/*
 * pil_anc.h
 *
 *  Created on: 23/10/2026
 *      Author: Mylo
 */

#ifndef PIL_ANC_H
#define PIL_ANC_H
#include <stdint.h>
#define NSCN  8
#define BLK   250
#define T_OFF 8000
#define T_ON  48000
#define NBLK  ((T_OFF+T_ON)/BLK)
void  pil_init(void);
void  pil_step(void);            /* se llama a 8 kHz */
void  pil_run_banco(void);       /* identificacion + los 8 escenarios */
void  pil_demo_start(void);      /* tono de 500 Hz continuo, ANC con el boton */
extern float pil_d_out, pil_e_out;
extern volatile uint8_t pil_anc_on;
extern float  res_rms_off[NSCN], res_rms_on[NSCN], res_att_dB[NSCN], res_tconv_ms[NSCN];
extern float  trace[NSCN][NBLK];
extern float  shat[32];
extern float  err_shat;
extern uint32_t cyc_max, cyc_avg;
#endif
