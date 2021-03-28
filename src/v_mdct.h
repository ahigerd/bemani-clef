/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING.Xiph'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.  *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2009             *
 * by the Xiph.Org Foundation https://xiph.org/                     *
 *                                                                  *
 ********************************************************************

 function: modified discrete cosine transform prototypes

 Modified 1/15/2020 by Adam Higerd: adapt for C++ use

 ********************************************************************/

#ifndef _OGG_mdct_H_
#define _OGG_mdct_H_

#include <vector>


/*#define MDCT_INTEGERIZED  <- be warned there could be some hurt left here*/
#ifdef MDCT_INTEGERIZED

#define DATA_TYPE int
#define REG_TYPE  register int
#define TRIGBITS 14
#define cPI3_8 6270
#define cPI2_8 11585
#define cPI1_8 15137

#define FLOAT_CONV(x) ((int)((x)*(1<<TRIGBITS)+.5))
#define MULT_NORM(x) ((x)>>TRIGBITS)
#define HALVE(x) ((x)>>1)

#else

#define DATA_TYPE float
#define REG_TYPE  float
#define cPI3_8 .38268343236508977175F
#define cPI2_8 .70710678118654752441F
#define cPI1_8 .92387953251128675613F

#define FLOAT_CONV(x) (x)
#define MULT_NORM(x) (x)
#define HALVE(x) ((x)*.5f)

#endif


struct mdct_lookup {
  int n;
  int log2n;

  std::vector<DATA_TYPE> trig;
  std::vector<int> bitrev;

  DATA_TYPE scale;
};

extern void mdct_init(mdct_lookup *lookup,int n);
extern void mdct_forward(const mdct_lookup *init, DATA_TYPE *in, DATA_TYPE *out);
extern void mdct_backward(const mdct_lookup *init, DATA_TYPE *in, DATA_TYPE *out);

#endif
