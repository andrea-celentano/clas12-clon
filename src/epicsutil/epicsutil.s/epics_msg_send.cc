/*
 ipc_epics_msg_send.cc - sends ipc epics message 
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

// for posix
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__


#define USE_ACTIVEMQ

#include "epicsutil.h"
#include "ipc_lib.h"


// misc
using namespace std;
#include <strstream>
#include <fstream>
#include <iostream>
#include <iomanip>

static IpcServer &server = IpcServer::Instance();

int
epics_msg_sender_init(char *application, char *unique_id)
{
  int status = 0;
  pthread_t t1;
  strstream temp;

  // synch with stdio
  ios::sync_with_stdio();

  //ipc_set_application(application);
  /*
  ipc_set_user_status_poll_callback(status_poll_callback);
  ipc_set_quit_callback(quit_callback);
  */

  if(strlen(unique_id)==0) unique_id = (char*)"epics_msg_sender";

  printf("epics_msg_sender_init: unique_id set to >%s<\n",unique_id);

  status = server.init(getenv("EXPID"), NULL, NULL, (char *)"epics_msg_send", NULL, "*");
  if(status<0)
  {
    cerr << "\n?Unable to connect to server...probably duplicate unique id\n"
	 << "   ...check for another epics_msg_sender  using ipc_info\n"
	 << "   ...only one connection allowed!" << endl << endl;
    return(EXIT_FAILURE);
  }
  
  // post startup message
  temp << "Process startup: ipc_epics_msg starting in " << application << ends;
  status = insert_msg("ipc_epics_msg","online",unique_id,"status",0,"START",0,temp.str());

  // flush output to log files, etc
  fflush(NULL);

  return(status);
}



int
epics_msg_send(const char *caname, const char *catype, int nelem, void *data)
{
  int ii;

  char   *domain   = (char*) "epics_msg_send";
  char   *user     = getenv("USER");
  int32_t msgtime = time(NULL);
  char   *host     = getenv("HOST");

  /* params check */
  if(strlen(host)==0)
  {
    printf("epics_msg_send: ERROR: host undefined\n");
    return(-1);
  }

  if(caname==NULL)
  {
    printf("epics_msg_send: ERROR: caname undefined\n");
    return(-1);
  }

  if(catype==NULL)
  {
    printf("epics_msg_send: ERROR: catype undefined\n");
    return(-1);
  }


  // get ref to server
  //TipcSrv &server = TipcSrv::Instance();
  // form and send message
  //TipcMsg msg((char*)"epics");
  //msg.Dest((char*)"epics_msg"); /* receiver will subscribe for that with '/' in front of it !!! */
  //msg.Sender((char*)caname); 


  /* clear message and send 'epics' keyword */
  server << clrm << "epics";

  /* standard section */
  server << domain << host << user << msgtime;

	/* epics section */
  server << caname << catype << nelem;

  if( !strcmp(catype,"int"))         for(ii=0; ii<nelem; ii++) server << (int32_t)((int *)data)[ii];
  else if( !strcmp(catype,"uint"))   for(ii=0; ii<nelem; ii++) server << (int32_t)((int *)data)[ii];
  else if( !strcmp(catype,"float"))  for(ii=0; ii<nelem; ii++) server << (float)((float *)data)[ii];
  else if( !strcmp(catype,"double")) for(ii=0; ii<nelem; ii++) server << (double)((double *)data)[ii];
  else if( !strcmp(catype,"uchar"))  for(ii=0; ii<nelem; ii++) server << (char)((char *)data)[ii];
  else if( !strcmp(catype,"string")) for(ii=0; ii<nelem; ii++) server << (char *)(((char **)data)[ii]);

  else
  {
    printf("epics_msg_send: ERROR: unknown catype >%s<\n",catype);
    return(-1);
  }

  server << endm;

  return(0);
}

int
epics_msg_close()
{
  int status;

  status = server.close();
}
