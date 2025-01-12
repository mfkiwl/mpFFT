
/*
  SPDX-FileCopyrightText: Copyright � 2021 Michael Frewer <frewer.science@gmail.com>
  SPDX-License-Identifier: Apache-2.0
*/

#include "mpFFT.h"

/* ********************************************************** Set Values ******************************************************** */

// set switch for parallel computation: 1 (on), 0 (off)
#define PARALLEL 1

// set binary computation precision P=digits/log10(2) as an integer
#define P 1024

// set vector-length as power of 2: N=2^n, n>=0
#define N (1UL<<10)

// set number of FFT repetitions M>=1 for time statistics
#define M 1
//#define M 1000

// set initial input (see OPTIONS.md for possible choices)
#define IN 2

// set rouding mode: round to nearest neighbour
#define R MPFR_RNDN

// set complexity count for fft-algorithm: 1 (on), 0 (off)
// turn off, when speed is needed (depending on compiler optimizations)
#define CC 1

// set first-orientation check by comparing to DFT (slow): 1 (on), 0 (off)
#define CH 1

// set binary printing precision Q<=P and choose file printing options: 1 (on), 0 (off)
#define Q 150
#define PRA 1 // print fft-result in double precision
#define PRB 1 // print fft-result in Q<=P precision
#define PRC 1 // print fft-test via ifft

/* ****************************************************************************************************************************** */

// read-in code
#if PARALLEL==0
#define FFT mp_sr24_cp_fft(N,1,0,inr,ini,outr,outi);
#include "lab/serial/mp/serial_mp_fft3.c" // currently most efficient serial implementation: CODE 3
#else
#define FFT mp_sr24_cp_fft(N,1,0,inr,ini,outr,outi);
#include "lab/parallel/mp/parallel_mp_fft2.c" // currently most efficient parallel implementation: CODE 2
#endif

int main()
{
 // disable the buffer for immediate print-out on screen
 setbuf(stdout,NULL);

 // declare time measurement variables
 unsigned long long int start_time,end_time,
                        start_time_init,end_time_init,
                        start_time_fft,end_time_fft,
                        elapsed;

 // set clock for overall time
 start_time=get_time_int();

 // set seed for random number generator
 seed=time(NULL);

 // print code info
 code_info();

 // initialize fft
 start_time_init=get_time_int();
 create_memory();
 lut();
 ic();
 end_time_init=get_time_int(); elapsed=end_time_init-start_time_init;
 printf("\nInitialization time: \t%.2f ms  ( %8.2f \xc2\xb5s)\n",(1e-6)*elapsed,(1e-3)*elapsed);

 // run fft
 // no parallel environment in main()
 if(PAR==0)
 {
  start_time_fft=get_time_int();
  for(unsigned long m=1;m<=M;m++) FFT;
  end_time_fft=get_time_int(); elapsed=end_time_fft-start_time_fft;
  printf("\nAverage time per FFT: \t%.2f ms  ( %8.2f \xc2\xb5s),\
  Performance (CTGs): %.2f MFLOP/s\n",(1e-6)*elapsed/M,(1e-3)*elapsed/M,(1e3)*5*N*log2(N)/(elapsed/M));
 }
 // create parallel environment in main()
 if(PAR==1)
 {
  #pragma omp parallel num_threads(4)
  {
   #pragma omp single
   {
    start_time_fft=get_time_int();
    for(unsigned long m=1;m<=M;m++) FFT;
    end_time_fft=get_time_int(); elapsed=end_time_fft-start_time_fft;
    printf("\nAverage time per FFT: \t%.2f ms  ( %8.2f \xc2\xb5s),\
    Performance (CTGs): %.2f MFLOP/s\n",(1e-6)*elapsed/M,(1e-3)*elapsed/M,(1e3)*5*N*log2(N)/(elapsed/M));
   }
  }
 }

 // show the performed fft computation complexity
 fft_complexity();

 // aftermath
 check();
 print_fft_result_d();
 print_fft_result_q();
 test();

 // exit
 clear_memory();

 end_time=get_time_int(); elapsed=end_time-start_time;
 printf("\nOverall time: \t%.2f s\n\n",(1e-9)*elapsed);

 return 0;
}
