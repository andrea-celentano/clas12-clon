
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <iostream>
using namespace std;

#include "evio.h"
#include "evioBankUtil.h"

#include "ftoflib.h"
#include "ftoftrans.h"

#include "hls_fadc_sum.h"
#include "trigger.h"


#define DEBUG_0
#define DEBUG_3


#define MAX(a,b)    (a > b ? a : b)
#define MIN(a,b)    (a < b ? a : b)

static float ped[21][16]; /* pedestals */
static float tet[21][16]; /* threshold */
static float gain[21][16]; /* gain */
static int nsa[21]; /* NSA */
static int nsb[21]; /* NSB */


void
ftofhiteventreader(hls::stream<eventdata_t> &event_stream, FTOFHit &hit, uint32_t *bufout)
{
  eventdata_t eventdata;
  uint32_t data_end, word_first, tag, inst, view, data, *bufptr = bufout;
  int j;
  while(1)
  {
    if(event_stream.empty()) {printf("ftofhiteventreader: EMPTY STREAM ERROR1\n");break;}
    eventdata = event_stream.read();
    *bufptr++ = eventdata.data;

    if(eventdata.end == 1) break;

    data_end = eventdata.end;           /* 0 for all words except last one when it is 1 */
    word_first = eventdata.data(31,31); /* 1 for the first word in hit, 0 for followings */
    tag = eventdata.data(30,27); /* must be 'FTOFHIT_TAG' */
    j = eventdata.data(7,0);

    if(event_stream.empty()) {printf("ftofhiteventreader: EMPTY STREAM ERROR2\n");break;}
    eventdata = event_stream.read();
    *bufptr++ = eventdata.data;
    data_end = eventdata.end;
	hit.output[j](61,32) = eventdata.data(29,0);

    if(event_stream.empty()) {printf("ftofhiteventreader: EMPTY STREAM ERROR2\n");break;}
    eventdata = event_stream.read();
    *bufptr++ = eventdata.data;
    data_end = eventdata.end;
	hit.output[j](31,0) = eventdata.data(31,0);
  }

}






int
ftoflib(uint32_t *bufptr, uint16_t threshold_[3], uint16_t nframes_)
{
  int status, ret, sec;
  int iev;
  unsigned long long timestamp;

  ap_uint<16> threshold[3] = {threshold_[0], threshold_[1], threshold_[2]};
  nframe_t nframes = 1;

  hls::stream<fadc_16ch_t> s_fadc_words[NFADCS];
  hls::stream<FTOFStrip_s> s_strip;
  hls::stream<FTOFHit> s_hits;
  volatile ap_uint<1> hit_scaler_inc;

  hls::stream<trig_t> trig_stream;
  hit_ram_t buf_ram[256];
  hls::stream<eventdata_t> event_stream;
  FTOFHit hit;

  int detector = FTOF;
  int nslot = 12;




  for(sec=0; sec<NSECTOR; sec++)
  {
    ret = fadcs(bufptr, threshold[0], sec, detector, s_fadc_words, 0, 0, &iev, &timestamp);
    if(ret<=0) continue;


    for(int it=0; it<MAXTIMES; it++)
    {
      ftof(threshold, nframes, s_fadc_words, s_hits, buf_ram);

      ftofhiteventwriter(trig_stream, event_stream, buf_ram);
      ftofhiteventreader(event_stream, hit);

    }
  }



  return(0);
}
