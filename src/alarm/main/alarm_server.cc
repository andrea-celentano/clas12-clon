// to do:   need mysql_autocommit
// 
//  alarm_server
//
//  monitors ipc alarm system and updates mysql database
//
//  still to do:
//     reenable alh alarm processing (probably never will...jun-2006)
//
//  ejw, 18-may-99
//  ejw, 20jun-2006 switched to mysql from ingres


// CC -c alarm_server.cc -I$RTHOME/include -I/usr/include/mysql/ 
//         -I/usr/local/clas/devel_new/include -I/usr/local/coda/2.2.1/common/include
//rtlink -cxx -o alarm_server alarm_server.o -L/usr/lib/mysql/ -lmysqlclient \
//         -L/usr/local/clas/devel_new/SunOS_sun4u/lib -lipc -lutil -L/usr/local/coda/2.2.1/SunOS/lib -ltcl

// for posix
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

// for smartsockets
#include <rtworks/cxxipc.hxx>


// for mysql
#include <mysql/mysql.h>


// system

using namespace std;
#include <strstream>
#include <fstream>
#include <iostream>
#include <iomanip>




// local variables used by mysql
static char *dbhost                = (char*)"clondb1";
static char *dbuser                = (char*)"clasrun";
static char *database              = (char*)"clasprod";
static MYSQL *dbhandle             = NULL;


// for CLAS ipc
#include <clas_ipc_prototypes.h>


// misc variables
static char *application      = (char*)"clastest";
static char *unique_id        = (char*)"alarm_server";
static time_t start           = time(NULL);
static int connected          = 0;
static time_t last_db_entry   = 0;
static int disconnect_timeout = 30;  // seconds
static int nolog              = 0;
static int done               = 0;
static int debug              = 0;
static int ncmlog             = 0;
static int nalarm             = 0;
static int nclear             = 0;
static int db_bad_count       = 0;
char temp[1024];


// other prototypes
void decode_command_line(int argc, char **argv);
void cmlog_callback(T_IPC_CONN,
		    T_IPC_CONN_PROCESS_CB_DATA,
		    T_CB_ARG);
extern "C" {
void quit_callback(int sig);
void status_poll_callback(T_IPC_MSG msg);
int insert_msg(char *name, char *facility, char *process, char *msgclass, 
	       int severity, char *status, int code, char *text);
}	


// ref to IPC server (connection created later)
TipcSrv &server=TipcSrv::Instance();


//--------------------------------------------------------------------------

int
main(int argc,char **argv)
{
  int status;


  // synch with c i/o
  ios::sync_with_stdio();


  // decode command line flags
  decode_command_line(argc,argv);



  // set ipc parameters and connect to ipc system
  ipc_set_application(application);
  ipc_set_user_status_poll_callback(status_poll_callback);
  ipc_set_quit_callback(quit_callback);
  status=ipc_init(unique_id,(char*)"alarm server");
  if(status<0)
  {
    cerr << "\n?Unable to connect to server...probably duplicate unique id\n"
	 << "   ...check for another alarm_server  using ipc_info\n"
	 << "   ...only one connection allowed!" << endl << endl;
    exit(EXIT_FAILURE);
  }
  TipcMt mt((char*)"cmlog");
  server.ProcessCbCreate(mt,cmlog_callback,0);
  server.SubjectSubscribe((char*)"cmlog",TRUE);



  // post startup message
  sprintf(temp,"Process startup:   %15s  in application:  %s",unique_id,application);
  if(debug) printf("%s\n",temp);
  status = insert_msg((char*)"alarm_server",(char*)"alarm_server",unique_id,(char*)"status",
		    0,(char*)"START",0,temp);
  cout << "\n\nalarm_server startup at " << ctime(&start) << flush;


  // process ipc alarm messages
  while (done==0)
  {
    server.MainLoop(1.0);

    // disconnect from database if connected and timeout reached
    if((connected==1)&&(time(NULL)-last_db_entry)>disconnect_timeout)
    {
      if(debug) printf("After server.MainLoop(1.0), disconnect from database\n");
      mysql_close(dbhandle);
      dbhandle=NULL;
      connected=0;
    }
  }


  // done
  sprintf(temp,"Process shutdown:  %15s",unique_id);
  status=insert_msg((char*)"alarm_server",(char*)"alarm_server",unique_id,(char*)"status",
		    0,(char*)"STOP",0,temp);
  time_t now = time(NULL);
  cout << "\n    alarm_server processed " << nalarm << " alarms, " 
       << nclear << " clears" << endl;
  cout << "\nalarm_server stopping at " << ctime(&now) << endl;
  ipc_close();

  exit(EXIT_SUCCESS);
}
       

//--------------------------------------------------------------------------


void
cmlog_callback(T_IPC_CONN conn,
		    T_IPC_CONN_PROCESS_CB_DATA data,
		    T_CB_ARG arg)
{
  T_STR domain;
  T_STR host;
  T_STR user;
  T_INT4 msgtime;
  T_STR name;
  T_STR facility;
  T_STR process;
  T_STR msgclass;
  T_INT4 severity;
  T_STR msgstatus;
  T_INT4 code;
  T_STR text;

  int alarm_status,code_db;
  char alarm_time[30];
  char msgclass_db[30];
  char atime[30];
  double interval;

  tm *tstruct;
  char *comma=(char*)",", *prime=(char*)"'";

  ncmlog++;

  if(debug) printf("ncmlog=%d\n",ncmlog);

  // unpack message
  TipcMsg msg(data->msg);
  msg >> domain >> host >> user >> msgtime
      >> name >> facility >> process >> msgclass 
      >> severity >> msgstatus >> code >> text;


  // only process clonalarm messages
  if(strcasecmp(facility,"clonalarm")!=0)return;
  if(code<=0) return;
  
  
  // ignore alh alarms for the moment...28-oct-98, EJW
  if(strcasecmp(process,"alh")==0)return;
  
  
  // connect to database if not connected
  if(connected==0)
  {
    if(dbhandle==NULL)dbhandle=mysql_init(NULL);
    if(mysql_real_connect(dbhandle,dbhost,dbuser,NULL,database,0,NULL,0))
    {
      connected=1;
      // <wait for mysql5.0> mysql_autocommit(dbhandle,1);
    }
    else
    {
      if((db_bad_count%50)==1)
      {
        if(1==1)
        {
	      time_t now = time(NULL);
	      cerr << "Unable to connect to database (error: " << mysql_errno(dbhandle) 
	           << ", " << mysql_error(dbhandle) << ") on " 
	           << ctime(&now) << endl;
	      insert_msg((char*)"alarm_server",(char*)"alarm_server",unique_id,(char*)"status",
		         3,(char*)"SEVERE",0,(char*)"Unable to connect to database");
        }
        return;
      }
    }
  }

  // save time of last db access
  last_db_entry=time(NULL);
  
  
// get most recent status,time since last update for this alarm id
//    tstruct = localtime(&msgtime);
//    strftime(atime,sizeof(atime),"%d-%b-%Y %H:%M",tstruct);
//    code_db=code;
//    strcpy(msgclass_db,msgclass);
//    exec sql select alarm_status,interval('secs',date(:atime)-alarm_time)
//      into :alarm_status,:interval from ingres.clonalarm 
//      where system=:msgclass_db and alarm_id=:code_db;
//    if(sqlca.sqlcode!=0) {
//      cerr << "?bad class,code,severity: " << msgclass << "," << code << "," 
//  	 << severity << endl;
//      return;
//    }
  
  
  // get alarm time
  tstruct = localtime(&msgtime);
  strftime(atime,sizeof(atime),"%Y-%m-%d %H:%M",tstruct);
  
  
  // form sql string
  strstream sql;
  sql << "update clonalarm set "
      << "alarm_status=" << severity << comma
      << "alarm_time=" << prime << atime << prime
      << " where alarm_id=" << code 
      << " and system=" << prime << msgclass << prime 
      << " and alarm_status!=" << severity
      << " and alarm_time<=" << prime << atime << prime  
      << ends;
    
    
    // update database...only happens if timestamp more recent and status changed
    // also count alarms, clears, etc.
  if(debug==0)
  {
    mysql_query(dbhandle,sql.str());
    unsigned int error = mysql_errno(dbhandle);
    if(error!=0)
    {
      cerr << "Error: " << error << "for: " << endl << sql.str() << endl 
	   << mysql_error(dbhandle) << endl;
    }
    
    int cnt;
    if((cnt=mysql_affected_rows(dbhandle))>0)
    {
      //	cout << cnt << " rows affected" << endl;
      if(severity>0) nalarm++; else nclear++;
    }
    else
    {
      //	cout << "no rows affected" << endl;
    }
    
  }
  else
  {
    cout << "sql is: " << sql.str() << endl;
  }
  

  // update log file
  if((debug==0)&&(nolog==0))
  {
    cout << atime << ":   " << setw(15) << msgclass << "   " << setw(5) << code 
	 << "   " << severity << "   " << text << endl;
  }
  
  return;
}


//--------------------------------------------------------------------------


extern "C" {
void status_poll_callback(T_IPC_MSG msg){

  TipcMsgAppendStr(msg,(char*)"connected");
  TipcMsgAppendInt4(msg,connected);

  TipcMsgAppendStr(msg,(char*)"ncmlog");
  TipcMsgAppendInt4(msg,ncmlog);

  TipcMsgAppendStr(msg,(char*)"nalarm");
  TipcMsgAppendInt4(msg,nalarm);

  TipcMsgAppendStr(msg,(char*)"nclear");
  TipcMsgAppendInt4(msg,nclear);

  TipcMsgAppendStr(msg,(char*)"db_bad_count");
  TipcMsgAppendInt4(msg,db_bad_count);

  return;
}
}


//-------------------------------------------------------------------


extern "C" {
void quit_callback(int sig){

  done=1;

  return;
}
}


//-------------------------------------------------------------------



void decode_command_line(int argc, char**argv){

  const char *help = "\nusage:\n\n  alarm_server [-a application] [-u unique_id]\n"
    "               [-host dbhost] [-user dbuser] [-db database]\n"
    "               [-dtime disconect_timeout] [-nolog] [-debug]\n";


  // loop over all arguments, except the 1st (which is program name)
  int i=1;
  while(i<argc) {
    if(strncasecmp(argv[i],"-h",2)==0){
      cout << help << endl;
      exit(EXIT_SUCCESS);
    }
    else if (strncasecmp(argv[i],"-nolog",6)==0){
      nolog=1;
      i=i+1;
    }
    else if (strncasecmp(argv[i],"-host",5)==0){
      dbhost=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-db",3)==0){
      database=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-user",5)==0){
      dbuser=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-debug",6)==0){
      debug=1;
      i=i+1;
    }
    else if (strncasecmp(argv[i],"-dtime",6)==0){
      disconnect_timeout=atoi(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-a",2)==0){
      application=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-u",2)==0){
      unique_id=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-",1)==0) {
      cout << "Unknown command line arg: " << argv[i] << argv[i+1] << endl << endl;
      i=i+2;
    }
  }

  return;
}

  


//---------------------------------------------------------------------

