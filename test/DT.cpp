#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <getopt.h>
#include <sys/stat.h>
#include <limits.h>
#include "conf.h"
#include "bitpack.h"
#include "vp4.h"
#include <algorithm>
#include <random>
#include <array>
/*
static uint32_t ret; 
size_t enc(unsigned char *_in, unsigned _n, unsigned char *out) {
  unsigned char sbuf[BLK_SIZE*2+1024];
  unsigned mdelta = 1;
  unsigned *in = (unsigned *)_in, n = (_n+3) / 4, i, *pa=(unsigned *)sbuf, x, b;
  x = *in++;
  --n;
  vbxput32(out, x);
  DELTR(in,n,x,mdelta,pa); 
	return p4encx32(pa,n,out) - out;
}

unsigned dec(unsigned char *in, unsigned _n, size_t i) {
  unsigned n = (outlen+3) / 4,x,b, out; 
  vbxget32(in, x);
  *out++ = x;
  --n;
  unsigned mdelta = 1;
  unsigned char* foo = mdelta
    ? p4f1decx32(       in, 1, &out, x)
    : p4fdecx32 (       in, 1, &out, x);
  return out;
}
*/

int dcmp(double *a, double *b) {
  if(*a < *b) return -1;
  if(*a > *b) return  1;
  return 0;
}

void zipu32(unsigned *a, unsigned n, double alpha, unsigned x1, unsigned x2) {
  int      i; 
  unsigned m = x2 - x1 + 1;
  double   prob, cum, *zmap;
  if(!(zmap = malloc(m*sizeof(zmap[0])))) die("mallo error %d\n", m); 

  // generate initial sample (slow)
  srand48(1); 
  if(alpha > 0) {
    for(cum = 0.0,i = 0; i < m; i++) 
      cum += 1.0 / pow(i+1, alpha); 
    cum = 1.0 / cum;    
    for(zmap[0] = prob = cum,i = 1; i < m; i++) zmap[i] = (prob += (cum / pow(i+1, alpha))); 
  } else for(i = 0; i < m; i++) zmap[i] = 1.0 / m;

  // use binary search to speed up zipfgen
  qsort(zmap, m, sizeof(zmap[0]), (int(*)(const void*,const void*))dcmp); 
  for(i = 0; i < n; i++) { 
    double r = drand48();
    int    l = 0, h = m-1;  
    while(l < h) { 
      int k = (l + h) >> 1; 
      if(r >= zmap[k]) l = k + 1; 
      else h = k; 
    }
    a[i] = x1 + l; 
  } 
  free(zmap); 
}

int main(){
    fprintf(stderr, "start\n");
    size_t n = 25000000, t = 100000000;
    size_t cs, ds;
    unsigned* in;
    uint32_t* out, tmp;
    unsigned char* cp;
    unsigned* ret;
    in = (unsigned* )malloc(n * sizeof(uint32_t));
    out = (uint32_t* )malloc(n * 5 / 4 * sizeof(uint32_t));
    tmp = (uint32_t* )malloc(1000 * sizeof(uint32_t));
    cp = (unsigned char* )malloc(n * 2 * sizeof(uint32_t));
    //qry = (unsigned* )malloc(t * sizeof(unsigned));
    std::vector<unsigned> qry;
    qry.resize(t);
    tmp = 0;
    for(auto iter = qry.begin(); iter != qry.end(); iter++) *iter = tmp++;
    shuffle(qry.begin(), qry.end(), std::mt19937_64());
    ret = (unsigned* )malloc(t * sizeof(uint32_t));
    int i;
    srand(0);
    //for(i = 0; i < n; i++) in[i] = (i << 2) + (rand() & 3);
    zipu32(in, (unsigned)n, 1.2, 0, 255);
    for(i = 1; i < n; i++) in[i] += in[i-1];
    //for(i = 0; i < n; i++) fprintf(stdout, "%u\n", in[i]);
    fprintf(stderr, "%u\n", in[n-1]);
    fprintf(stderr, "start\n");
    cs = p4encx32(in, n, cp) - cp;
    //cs = bitnpack128v32(in, n, cp);
    struct p4 p4;
    unsigned bx;
    unsigned b;// = p4bits(cp, &bx);
    fprintf(stderr, "start\n");
    p4ini(&p4, &cp, cs, &b);
    fprintf(stderr, "b=%u cs=%u isx=%u r=%f\n", b, cs, p4.isx, 1.f * cs / n);
    /*
    for(i = 0; i < n; i++){
      int idx = i;//rand() % n;
      if (p4.isx) 
        ret = p4getx32(&p4, cp, idx, b);
      else
        ret = bitgetx32(cp, idx, b);
      if (ret != in[idx]){
        fprintf(stderr, "%d: %u, %u\n", i, ret, in[idx]);
        break;
      }
    }
    */
    for(i = 0; i < t; i++)
      qry[i] = rand() % n;
    struct timeval begin;  
    struct timeval end;
    long tc, sum = 0;
    gettimeofday(&begin, NULL);  
    for(i = 0; i < t; i++){
      sum += in[qry[i]];
    }
    gettimeofday(&end, NULL);
    tc = (long)(end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec;
    fprintf(stderr, "Sum = %ld\n", sum);
    fprintf(stderr, "Mem Time cost: %ld us\n", tc);
    fprintf(stderr, "Speed: %3f integers/us\n", (double)t / tc);
    fprintf(stderr, "Speed: %3f ns\n", (double)tc / t * 1000);
    sum = 0;
    gettimeofday(&begin, NULL);
    for(i = 0; i < t; i++){
      //if (p4.isx) ret = p4getx32(&p4, cp, i, b); else
      sum += bitgetx32(cp, qry[i], b);
      //if (ret != in[qry[i]]) fprintf(stderr, "%d: %u, %u\n", i, ret, in[qry[i]]);
    }
    gettimeofday(&end, NULL);  
    tc = (long)(end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec;
    int flag = 0;
    /*
    for(i = 0; i < t; i++)
      if(ret[i] != in[qry[i]]){ flag = 1; break; }
    if(flag) fprintf(stderr,"failed!\n");
    else fprintf(stderr,"correct!\n");*/
    fprintf(stderr, "Sum = %ld\n", sum);
    fprintf(stderr,"Time cost: %ld us\n", tc);
    fprintf(stderr,"Speed: %3f integers/us\n", (double)t / tc);
    fprintf(stderr, "Speed: %3f ns\n", (double)tc / t * 1000);
    fprintf(stderr, "end\n");
    return 0;
}