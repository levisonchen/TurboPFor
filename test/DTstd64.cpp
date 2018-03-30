#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/time.h>
#include <sys/stat.h>
#include <climits>

#include <algorithm>
#include <array>
#include <random>

#include "conf.h"
#include "bitpack.h"
#include "vp4.h"

int main(int argc, char* argv[]){
    
    unsigned i = 0;
    size_t n = atol(argv[1]);
    size_t t = atol(argv[2]);
    //fprintf(stderr, "%zd, %zd\n", n, t);
    if(n == 0 || t == 0) return 0;

    size_t cs;
    uint64_t* in;
    unsigned char* cp;
    std::vector<unsigned> qry;

    fprintf(stderr, "Start!\n");
    in = (unsigned* )malloc(n * sizeof(uint64_t));
    cp = (unsigned char* )malloc(n * 2 * sizeof(uint64_t));
    
    for(i = 0; i < n; i++) fscanf(stdin, "%u", &in[i]);

    qry.resize(t); i = 0;
    for(auto iter = qry.begin(); iter != qry.end(); iter++) *iter = (i++) % n;
    shuffle(qry.begin(), qry.end(), std::mt19937_64());
    
    cs = p4ndenc256v32(in, n, cp) - cp;
    struct p4 p4;
    unsigned b, bx;
    p4ini(&p4, &cp, cs, &b);
    fprintf(stderr, "\nBase = %u\nCompressed Size = %u\nisx = %u\nRatio = %f\n", b, cs, p4.isx, 1.f * cs / n);

    struct timeval begin;  
    struct timeval end;
    long tc, sum = 0;
    
    gettimeofday(&begin, NULL);
    for(i = 0; i < t; i++) sum += in[qry[i]];
    gettimeofday(&end, NULL);
    
    tc = (long)(end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec;
    fprintf(stderr, "\nSum = %ld\n", sum);
    fprintf(stderr, "Mem Time cost: %ld us\n", tc);
    fprintf(stderr, "Speed: %3f integers/us\n", (double)t / tc);
    fprintf(stderr, "Speed: %3f ns\n", (double)tc / t * 1000);

    sum = 0;
    gettimeofday(&begin, NULL);
    for(i = 0; i < t; i++) sum += bitgetx32(cp, qry[i], b);
    gettimeofday(&end, NULL);

    tc = (long)(end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec;
    
    fprintf(stderr, "\nSum = %ld\n", sum);
    fprintf(stderr, "Time cost: %ld us\n", tc);
    fprintf(stderr, "Speed: %3f integers/us\n", (double)t / tc);
    fprintf(stderr, "Speed: %3f ns\n", (double)tc / t * 1000);

    fprintf(stderr, "\nEnd.\n");

    return 0;
}