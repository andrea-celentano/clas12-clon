
/* ftofhitfanout.cc */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

using namespace std;
#include <strstream>



#include "ftoflib.h"


//#define DEBUG


/* 0.0/5/4/0%/0%/(306)~0%/(231)~0% II=4 */

void
ftofhitfanout(FTOFHit hitin[NH_READS], hls::stream<FTOFHit_8slices> &s_hits, FTOFHit hitout[NH_READS], volatile ap_uint<1> &hit_scaler_inc)
{
//#pragma HLS INTERFACE ap_none port=hit_scaler_inc
//#pragma HLS DATA_PACK variable=s_hits
//#pragma HLS INTERFACE axis register both port=s_hits
#pragma HLS PIPELINE II=1

  FTOFHit_8slices hit;

  ap_uint<1> scaler[NH_READS];
  ap_uint<1> scaler_tmp = 0;

  for(int i=0; i<NH_READS; i++)
  {
    hit.output[i] = hitin[i].output;

    scaler[i] = 0;
    if(hit.output[i] != 0) scaler[i] = 1;
    //else                 scaler[i] = 0;

    hitout[i].output = hit.output[i];
  }

  s_hits.write(hit);

  for(int i=0; i<NH_READS; i++)
  {
    scaler_tmp |= scaler[i];
  }

  if(scaler_tmp) hit_scaler_inc = 1;
  else           hit_scaler_inc = 0;
}


