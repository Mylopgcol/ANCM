/*
 * pil_anc.c
 *
 *  Created on: 23/10/2026
 *      Author: Mylo
 */


#include "pil_anc.h"
#include <math.h>
#include <string.h>

#define FS     8000.0f
#define TWO_PI 6.2831853f
#define L      32           /* orden de W(z) */
#define LH     32           /* longitud de S^(z) */
#define LS     16           /* longitud de S(z) real */
#define LP     32           /* longitud de P(z) real */
#define MU     0.05f        /* paso FxNLMS */
#define MU_ID  0.2f         /* paso NLMS de identificacion */
#define AMB    0.005f       /* ruido ambiente en el microfono de error */
#define T_ID   24000        /* 3 s de identificacion */

/* Rutas acusticas modeladas (la "planta" digital) */
static const float S_real[LS] = {0,0,0.6f,0.3f,-0.15f,0.05f,-0.02f,0,0,0,0,0,0,0,0,0};
static const float P_real[LP] = {0,0,0,0,0,0,0,0,0,0,0.8f,0.5f,-0.2f,0.15f,0.1f,-0.05f,0.02f,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

volatile uint8_t pil_anc_on = 0;
float w[L], xbuf[L], xfbuf[L], xs_hist[LP], yhist[LS], ubuf[LH];
float shat[LH], shat_use[LH];
float err_shat;
static uint32_t rng = 2463534242u;
static float ph1, ph2;
static int tipo, fase_id, cnt, delta_shat, scn_actual, demo;
static float off_acc, on_acc, blk_acc, rms_off_c, rms_on_c;
static volatile int done;

float res_rms_off[NSCN], res_rms_on[NSCN], res_att_dB[NSCN], res_tconv_ms[NSCN];
float trace[NSCN][NBLK];
uint32_t cyc_max, cyc_avg;
float pil_d_out, pil_e_out;

static float rnd(void){ rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5;
                        return (int32_t)rng * (1.0f/2147483648.0f); }
static float q12(float v){ if(v>0.9995f) v=0.9995f; if(v<-1.0f) v=-1.0f;
                           return roundf(v*2048.0f)/2048.0f; }          /* conversor de 12 bits */
static void  shift(float *a, int n, float v){ memmove(a+1, a, (n-1)*sizeof(float)); a[0]=v; }
static float dot(const float *a, const float *b, int n){ float s=0; for(int i=0;i<n;i++) s+=a[i]*b[i]; return s; }

/* Banco de ruidos: ruido primario emitido por la fuente */
static float fuente(void){
  switch(tipo){
    case 0: ph1 += TWO_PI*300.0f/FS; if(ph1>TWO_PI) ph1-=TWO_PI; return 0.5f*sinf(ph1);
    case 1: case 6: case 7:
            ph1 += TWO_PI*500.0f/FS; if(ph1>TWO_PI) ph1-=TWO_PI; return 0.5f*sinf(ph1);
    case 2: ph1 += TWO_PI*800.0f/FS; if(ph1>TWO_PI) ph1-=TWO_PI; return 0.5f*sinf(ph1);
    case 3: ph1 += TWO_PI*300.0f/FS; if(ph1>TWO_PI) ph1-=TWO_PI;
            ph2 += TWO_PI*500.0f/FS; if(ph2>TWO_PI) ph2-=TWO_PI;
            return 0.3f*(sinf(ph1)+sinf(ph2));
    case 4: { float f = 200.0f + 800.0f*((cnt % 16000)/16000.0f);       /* barrido 200-1000 Hz */
              ph1 += TWO_PI*f/FS; if(ph1>TWO_PI) ph1-=TWO_PI; return 0.5f*sinf(ph1); }
    default: return 0.5f*rnd();                                          /* ruido blanco */
  }
}

void pil_init(void){ memset(w,0,sizeof w); memset(shat,0,sizeof shat); }

static void reset_lazo(void){
  memset(w,0,sizeof w); memset(xbuf,0,sizeof xbuf); memset(xfbuf,0,sizeof xfbuf);
  memset(xs_hist,0,sizeof xs_hist); memset(yhist,0,sizeof yhist); memset(ubuf,0,sizeof ubuf);
  ph1 = ph2 = 0; cnt = 0; off_acc = on_acc = blk_acc = 0; pil_anc_on = 0; done = 0;
}

void pil_step(void){
  cnt++;
  if (fase_id) {                                          /* ---- identificacion de S^(z) ---- */
    float u   = q12(0.3f*rnd());                          /* ruido blanco por el DAC */
    shift(ubuf, LH, u); shift(yhist, LS, u);
    float e   = q12(dot(S_real, yhist, LS) + AMB*rnd());  /* microfono de error */
    float p   = 1e-4f + dot(ubuf, ubuf, LH);
    float eid = e - dot(shat, ubuf, LH);
    float g   = MU_ID*eid/p;
    for (int i=0;i<LH;i++) shat[i] += g*ubuf[i];          /* NLMS */
    if (cnt >= T_ID) done = 1;
    return;
  }
  /* ---- lazo ANC ---- */
  float xs = fuente();
  shift(xs_hist, LP, xs);
  float x  = q12(xs);                                     /* ADC 12 bits (referencia) */
  shift(xbuf, L, x);
  float d  = dot(P_real, xs_hist, LP);                    /* d(n) = P * fuente */
  float y  = pil_anc_on ? dot(w, xbuf, L) : 0.0f;         /* y(n) = W * x */
  y = q12(y);                                             /* DAC 12 bits */
  shift(yhist, LS, y);
  float yp = dot(S_real, yhist, LS);                      /* y'(n) = S * y */
  float e  = q12(d + yp + AMB*rnd());                     /* ADC 12 bits (error) */
  float xf = dot(shat_use, xbuf, LH);                     /* x'(n) = S^ * x */
  shift(xfbuf, L, xf);
  if (pil_anc_on) {
    float p = 1e-4f + dot(xfbuf, xfbuf, L);
    float g = MU*e/p;
    for (int i=0;i<L;i++) w[i] -= g*xfbuf[i];             /* FxNLMS */
  }
  pil_d_out = d; pil_e_out = e;
  if (demo) return;
  /* metricas */
  blk_acc += e*e;
  if (cnt % BLK == 0) { trace[scn_actual][cnt/BLK-1] = sqrtf(blk_acc/BLK); blk_acc = 0; }
  if (cnt <= T_OFF) off_acc += e*e;
  if (cnt == T_OFF) { rms_off_c = sqrtf(off_acc/T_OFF); pil_anc_on = 1; }
  if (cnt > T_OFF+T_ON-8000) on_acc += e*e;
  if (cnt == T_OFF+T_ON) { rms_on_c = sqrtf(on_acc/8000.0f); done = 1; }
}

void pil_demo_start(void){
  tipo = 1; delta_shat = 0;
  for(int i=0;i<LH;i++) shat_use[i] = shat[i];
  reset_lazo(); demo = 1;
}

extern void pil_wait_done(volatile int *flag);            /* se define en main.c */
void pil_run_banco(void){
  reset_lazo(); fase_id = 1; pil_wait_done(&done); fase_id = 0;      /* 1) identificacion */
  float n1=0, n2=0;
  for(int i=0;i<LH;i++){ float s=(i<LS)?S_real[i]:0; n1+=(s-shat[i])*(s-shat[i]); n2+=s*s; }
  err_shat = sqrtf(n1/n2);
  for (int k=0;k<NSCN;k++){                                           /* 2) escenarios */
    scn_actual = k; tipo = k; delta_shat = (k==6)?3:(k==7)?4:0;
    for (int i=0;i<LH;i++) shat_use[i] = (i>=delta_shat)? shat[i-delta_shat] : 0;
    reset_lazo(); pil_wait_done(&done);
    res_rms_off[k] = rms_off_c; res_rms_on[k] = rms_on_c;
    res_att_dB[k]  = 20.0f*log10f(rms_off_c/(rms_on_c+1e-12f));
    res_tconv_ms[k] = -1;
    for (int j=T_OFF/BLK; j<NBLK; j++)
      if (trace[k][j] < 0.1f*rms_off_c){ res_tconv_ms[k] = (j-T_OFF/BLK+1)*BLK/8.0f; break; }
  }
}
