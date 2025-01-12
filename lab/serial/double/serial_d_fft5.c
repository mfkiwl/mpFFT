
/*
  SPDX-FileCopyrightText: Copyright � 2021 Michael Frewer <frewer.science@gmail.com>
  SPDX-License-Identifier: Apache-2.0
*/

/* ****************************************************************************************************************************** */

static double *LUTr,*LUTi;

static double *inr,*ini,*outr,*outi;

static double w8,
              w16_1,w16_2,w16_3,w16_4;

unsigned long cmul=0, cadd=0, // complexity counter for multiplication and addition
              cfc=0, crb=0;   // complexity counter for recursive-function calls and its recursion breaks

time_t seed;

/* ****************************************************************************************************************************** */

unsigned long long int get_time_int(void)
// time in nanoseconds as an integer
{
 struct timespec time;
 clock_gettime(CLOCK_MONOTONIC,&time);
 return time.tv_sec*1000000000+time.tv_nsec;
}

/* ****************************************************************************************************************************** */

void code_info()
{
 printf("\nSerial, double-precision, CODE=%d, N=%lu, IN=%d\n\
 \bFFT-algorithm: DIT, recursive SR-2/4, conjugate-pair, 4m2a, 3-butterfly, recursion-break: n=4, twiddle-index: mul(*)\n",
 CODE,N,IN);
}

/* ****************************************************************************************************************************** */

void create_memory()
{
 LUTr=malloc(N/4*sizeof(double)); LUTi=malloc(N/4*sizeof(double));

 inr =malloc(N*sizeof(double)); ini =malloc(N*sizeof(double));
 outr=malloc(N*sizeof(double)); outi=malloc(N*sizeof(double));
}

/* ****************************************************************************************************************************** */

void lut()
{
 for(unsigned long k=0;k<N/4;k++)
 {
  LUTr[k]=cos(2.*M_PI*k/N); LUTi[k]=sin(2.*M_PI*k/N); // NOTE: w=wr-Iwi; w*=wr+Iwi
 }

 w8=cos(2.*M_PI*1/8);
 w16_1=cos(2.*M_PI*1/16); w16_2=cos(2.*M_PI*1/16)-cos(2.*M_PI*3/16);
 w16_3=cos(2.*M_PI*3/16); w16_4=cos(2.*M_PI*1/16)+cos(2.*M_PI*3/16);
}

/* ****************************************************************************************************************************** */

void ic()
{
 switch(IN)
 {
  case 1: // complex random input between 0 and 1
  {
   srand(seed);
   for(unsigned long k=0;k<N;k++)
   {
    inr[k]=(double)rand()/RAND_MAX; //printf("inr[%lu]=%.15lf\n",k,inr[k]);
    ini[k]=(double)rand()/RAND_MAX; //printf("ini[%lu]=%.15lf\n",k,ini[k]);
   }
  }
  break;

  case 2: // fixed complex input: inr[k]=1/(1+k^2); ini[k]=1/(1+k^4)
  for(unsigned long k=0;k<N;k++)
  {
   double k2=k*k;
   inr[k]=1./(1+k2); ini[k]=1./(1+k2*k2);
  }
  break;
 }
}

/* ****************************************************************************************************************************** */

void d_sr24_cp_fft(unsigned long n, unsigned long s, long int sn, double *inr, double *ini, double *outr, double *outi)
{
 if(CC){cmul+=0; cadd+=0; cfc+=1; crb+=0;}

 if(n==1)
 {
  unsigned long s0;
  if(sn<0) s0=N; else s0=0;

  outr[0]=inr[s0];

  outi[0]=ini[s0];

  if(CC){cmul+=0; cadd+=0; cfc+=0; crb+=1;}
 }
 else if(n==2)
 {
  unsigned long s0;
  if(sn<0) s0=N; else s0=0;

  outr[0]=inr[s0]+inr[1*s];
  outr[1]=inr[s0]-inr[1*s];

  outi[0]=ini[s0]+ini[1*s];
  outi[1]=ini[s0]-ini[1*s];

  if(CC){cmul+=0; cadd+=4; cfc+=0; crb+=1;}
 }
 else if(n==4)
 {
  unsigned long s0;
  if(sn<0) s0=N; else s0=0;

  double a1r=inr[s0]+inr[2*s],
         a2r=inr[s0]-inr[2*s],
         a3r=inr[1*s]+inr[3*s],
         a4r=inr[1*s]-inr[3*s],

         a1i=ini[s0]+ini[2*s],
         a2i=ini[s0]-ini[2*s],
         a3i=ini[1*s]+ini[3*s],
         a4i=ini[1*s]-ini[3*s];

  outr[0]=a1r+a3r;
  outr[1]=a2r+a4i;
  outr[2]=a1r-a3r;
  outr[3]=a2r-a4i;

  outi[0]=a1i+a3i;
  outi[1]=a2i-a4r;
  outi[2]=a1i-a3i;
  outi[3]=a2i+a4r;

  if(CC){cmul+=0; cadd+=16; cfc+=0; crb+=1;}
 }
 else
 {
  d_sr24_cp_fft(n/2,2*s,sn,inr,ini,outr,outi);
  d_sr24_cp_fft(n/4,4*s,+1*s+sn,inr+s,ini+s,outr+2*n/4,outi+2*n/4);
  d_sr24_cp_fft(n/4,4*s,-1*s+sn,inr-s,ini-s,outr+3*n/4,outi+3*n/4);

  // special butterfly k=0
  double U0r=outr[0*n/4],
         U1r=outr[1*n/4],
         Zpr=outr[2*n/4],
         Zmr=outr[3*n/4],

         U0i=outi[0*n/4],
         U1i=outi[1*n/4],
         Zpi=outi[2*n/4],
         Zmi=outi[3*n/4],

         a1r=Zpr+Zmr, a1i=Zpi+Zmi,
         a2r=Zpr-Zmr, a2i=Zpi-Zmi;

  outr[0*n/4]=U0r+a1r;
  outr[2*n/4]=U0r-a1r;
  outr[1*n/4]=U1r+a2i;
  outr[3*n/4]=U1r-a2i;

  outi[0*n/4]=U0i+a1i;
  outi[2*n/4]=U0i-a1i;
  outi[1*n/4]=U1i-a2r;
  outi[3*n/4]=U1i+a2r;

  if(CC){cmul+=0; cadd+=12; cfc+=0; crb+=0;}

  // general butterfly for all k, extracting k=n/8 as special butterfly
  unsigned long k,kn,ks=N/n;
  for(k=1;k<n/4;k++)
  {
   U0r=outr[k+0*n/4],
   U1r=outr[k+1*n/4],
   Zpr=outr[k+2*n/4],
   Zmr=outr[k+3*n/4],

   U0i=outi[k+0*n/4],
   U1i=outi[k+1*n/4],
   Zpi=outi[k+2*n/4],
   Zmi=outi[k+3*n/4];

   kn=k*ks;
   double wr=LUTr[kn], wi=LUTi[kn];

   double m1r,m1i,m2r,m2i;
   if(k==n/8)
   {
    m1r=wr*(Zpr+Zpi); m1i=wr*(Zpi-Zpr);
    m2r=wr*(Zmr-Zmi); m2i=wr*(Zmi+Zmr);

    if(CC){cmul+=4; cadd+=4; cfc+=0; crb+=0;}
   }
   else
   {
    m1r=wr*Zpr+wi*Zpi; m1i=wr*Zpi-wi*Zpr;
    m2r=wr*Zmr-wi*Zmi; m2i=wr*Zmi+wi*Zmr;

    if(CC){cmul+=8; cadd+=4; cfc+=0; crb+=0;}
   }

   a1r=m1r+m2r; a1i=m1i+m2i;
   a2r=m1r-m2r; a2i=m1i-m2i;

   outr[k+0*n/4]=U0r+a1r;
   outr[k+2*n/4]=U0r-a1r;
   outr[k+1*n/4]=U1r+a2i;
   outr[k+3*n/4]=U1r-a2i;

   outi[k+0*n/4]=U0i+a1i;
   outi[k+2*n/4]=U0i-a1i;
   outi[k+1*n/4]=U1i-a2r;
   outi[k+3*n/4]=U1i+a2r;

   if(CC){cmul+=0; cadd+=12; cfc+=0; crb+=0;}
  }
 }
}

/* ****************************************************************************************************************************** */

void fft_complexity()
{
 if(CC)
 {
  unsigned long lgN=round(log2(N)),c1FLOPS,c2FLOPS;

  if(N>1)
  {
   c1FLOPS=round(4.*N*lgN-6.*N+8.);
   if((lgN%2)==0) c2FLOPS=round((34./9)*N*lgN-(124./27)*N-2.*lgN-(2./9)*lgN+(16./27)+8.);
   else c2FLOPS=round((34./9)*N*lgN-(124./27)*N-2.*lgN+(2./9)*lgN-(16./27)+8.);
  }
  else {c1FLOPS=0; c2FLOPS=0;}

  printf("\nFFT Computation Complexity: cmul=%lu, cadd=%lu, cfc=%lu, crb=%lu\
  \n\nExact total FLOPs performed: cmul + cadd =\x1b[1m %lu\x1b[0m\
  \nSplit-radix theoretical value:\
  \b\b\x1b[1m %lu\x1b[0m (scales as 4Nlog2(N) for N>>1: Yavne, 1968)\
  \nSplit-radix theoretical (current) lowest value:\
  \b\b\x1b[1m %lu\x1b[0m (scales as (34/9)Nlog2(N) for N>>1: van Buskirk, 2004; Johnson & Frigo, 2007)\n",
  cmul/M,cadd/M,cfc/M,crb/M,(cmul+cadd)/M,c1FLOPS,c2FLOPS);
 }
}

/* ****************************************************************************************************************************** */

void check()
// first orientation-check if overall code is okay at least to within 1e-10, by comparing to a directly computed dft (slow)
{
 if(CH)
 {
  unsigned long j,k,
                c_bad=0, c_ok=0;
  double sumr,sumi,u=2.*M_PI/N;;
  for(k=0;k<N;k++)
  {
   sumr=0.;sumi=0.;
   for(j=0;j<N;j++)
   {
    sumr+=inr[j]*cos(u*j*k)+ini[j]*sin(u*j*k);
    sumi+=ini[j]*cos(u*j*k)-inr[j]*sin(u*j*k);
   }
   if(fabs(outr[k]-sumr)>(1e-10)) c_bad+=1; else c_ok+=1;
   if(fabs(outi[k]-sumi)>(1e-10)) c_bad+=1; else c_ok+=1;
  }
  if(c_bad>0) printf("\n***___BAD___***: %lu\n",c_bad); else printf("\nOK: %lu\n",c_ok);
 }
}

/* ****************************************************************************************************************************** */

void print_fft_result()
{
 if(PRA)
 {
  FILE *f=fopen("data_serial_d_fft5.dat","w");

  for(unsigned long k=0;k<N;k++)
  fprintf(f,"% -.16e  % -.16ei  |  % -.16e  % -.16ei\n",inr[k],ini[k],outr[k],outi[k]);

  fclose(f);
 }
}

/* ****************************************************************************************************************************** */

void test()
{
 if(PRB)
 {
  FILE *f1=fopen("data_serial_d_fft5_test.dat","w"),
       *f2=fopen("data_serial_d_fft5_test_error.dat","w");

  unsigned long k;
  double sumr=0.0, sumi=0.0,
         sumdr=0.0,sumdi=0.0;

  // intializing ifft by swapping inr and ini with the current out-values
  for(k=0;k<N;k++)
  {
   inr[k]=outi[k]; ini[k]=outr[k];
  }

  // run ifft (up to trivial re-ordering in the real & imaginary part and normalization in the resulting out-values)
  FFT;
  // restoring the orignal in-values
  ic();

  // print the new out-values, normalized by N, which then, when swapped, should equal the original in-values
  for(k=0;k<N;k++)
  {
   fprintf(f1,"% -.16e  % -.16ei  |  % -.16e  % -.16ei\n",inr[k],ini[k],outi[k]/N,outr[k]/N);

   // get the data for the relative root-mean-square-error (rRMSE)
   sumr+=inr[k]*inr[k]; sumi+=ini[k]*ini[k];
   sumdr+=(inr[k]-outi[k]/N)*(inr[k]-outi[k]/N); sumdi+=(ini[k]-outr[k]/N)*(ini[k]-outr[k]/N);
  }

  // print the error measures between the true (in) and computed (out) values:
  // 1st column-pair: absolute error, which is a measure of the precision used
  // 2nd column-pair: relative error, as a measure of accuracy achieved
  //                  --- NOTE: this measure is ill-defined when the true-value is close to zero
  // 3rd column-pair: relative root-mean-square error (rRMSE), as a better/reliable measure of accuracy achieved
  for(k=0;k<N;k++)
  {
   fprintf(f2,"% -.16e  % -.16ei  |  % -.16e  % -.16ei  |  % -.16e  % -.16ei\n",
               inr[k]-outi[k]/N,ini[k]-outr[k]/N,
               (inr[k]-outi[k]/N)/inr[k],(ini[k]-outr[k]/N)/ini[k],
               sqrt(sumdr)/sqrt(sumr),sqrt(sumdi)/sqrt(sumi));
  }
  printf("\nRelative root-mean-square error (rRMSE): % -.5e (real part), % -.5e (imaginary part)\n",
          sqrt(sumdr)/sqrt(sumr),sqrt(sumdi)/sqrt(sumi));

  fclose(f1);
  fclose(f2);
 }
}

/* ****************************************************************************************************************************** */

void clear_memory()
{
 free(inr); free(ini); free(outr); free(outi);
 free(LUTr); free(LUTi);
}

/* ****************************************************************************************************************************** */
