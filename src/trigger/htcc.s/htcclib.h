#ifndef _HTCCLIB_

#include <ap_int.h>
#include <hls_stream.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* htcclib.h */

#define NSLOT 3
#define NSTRIP 48

#define NCHAN 48
#define NCLSTR 36
#define NHIT NCLSTR


#define NH_READS  8   /* the number of reads-write for streams, AND the number of 4ns intervals inside 32ns interval */
#define MAXTIMES 2
#define TIMECORR 7 /*TEMPORARY !!!!!!!!!!!!!!!! */

/* strip persistency */
#define NPER 8
typedef ap_uint<3> nframe_t;


typedef struct
{
  ap_uint<13>	e0;
  ap_uint<3>	t0;
  ap_uint<13>	e1;
  ap_uint<3>	t1;
} fadc_word_t;

typedef struct htccstrip
{
  ap_uint<13> energy;       /* strip energy */
} HTCCStrip;

#define N1 6 /* the number of elements in following struct */
#define NSTREAMS1 (NCHAN/N1)
typedef struct htccstrip_s
{
  ap_uint<13> energy00;
  ap_uint<13> energy01;
  ap_uint<13> energy02;
  ap_uint<13> energy03;
  ap_uint<13> energy04;
  ap_uint<13> energy05;
} HTCCStrip_s;



typedef struct htcchit
{
  ap_uint<NCLSTR> output;
} HTCCHit;





#define HTCCHIT_TAG 0x5

typedef struct
{
  ap_uint<11> t_start;
  ap_uint<11> t_stop;
} trig_t;

typedef struct
{
  ap_uint<32> data;
  ap_uint<1>  end;
} eventdata_t;


#define NBIT_HIT_ENERGY 16
#define NBIT_COORD_EB 10 /*in event builder coordinates always 10 bits*/

typedef struct
{
  ap_uint<NCLSTR> output;
} hit_ram_t;




int htcclib(int handler);
void htcchiteventreader(hls::stream<trig_t> &trig_stream, hls::stream<eventdata_t> &event_stream, HTCCHit &hit);

void htccstrips(ap_uint<16> strip_threshold, hls::stream<fadc_word_t> s_fadc_words[NSLOT], hls::stream<HTCCStrip_s> s_strip0[NSTREAMS1]);
void htccstripspersistence(nframe_t nframes, hls::stream<HTCCStrip_s> &s_stripin, hls::stream<HTCCStrip_s> &s_stripout, ap_uint<3> jj);
//void htccstripspersistence(nframe_t nframes, hls::stream<HTCCStrip_s> s_stripin[NSTREAMS1], hls::stream<HTCCStrip_s> s_stripout[NSTREAMS1]);
void htcchit(ap_uint<16> strip_threshold, ap_uint<16> mult_threshold, ap_uint<16> cluster_threshold, hls::stream<HTCCStrip_s> s_strip[NSTREAMS1], hls::stream<HTCCHit> &s_hit);
void htcchitfanout(hls::stream<HTCCHit> &s_hit, hls::stream<HTCCHit> &s_hit1, hls::stream<HTCCHit> &s_hit2, volatile ap_uint<1> &hit_scaler_inc);
void htcchiteventfiller(hls::stream<HTCCHit> &s_hitin, hit_ram_t buf_ram[256]);
void htcchiteventwriter(hls::stream<trig_t> &trig_stream, hls::stream<eventdata_t> &event_stream, hit_ram_t buf_ram_read[256]);

#ifdef	__cplusplus
}
#endif


#define _HTCCLIB_
#endif
