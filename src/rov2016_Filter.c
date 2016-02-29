/**
 **************************************************************************************
 * @file    rov2016_Filter.c
 * @author  Sivert Sliper, Stian Soerensen
 * @version V1.0
 * @date    3-February-2016
 * @brief   This file contains functions for a 64-coefficient FIR decimation filter.
 **************************************************************************************
 */

/* Include------------------------------------------------------------------------------*/
#include "stm32f30x.h"
#include "rov2016_Filter.h"

/* Private function prototypes ---------------------------------------------------------*/

/* Private variables -------------------------------------------------------------------*/

/* Function definitions ----------------------------------------------------------------*/
float sum0; // Accumulator.
float x0,c0; /* Temporary vars for state- and coeficcient vals. */

/* Pointers */
float *pCoeff;
// float *pTilstand;

/* Temporary pointers fro state- and coefficient buffers.*/
float *px, *pb, *pStateCurnt;
/* Coefficient vector in reversed-time order.
 * i.e. b[NUM_TAPS-1] + b[NUM_TAPS-2] + ... b[1] + b[0]
 * Coefficients are generated in Matlab and are suitable for
 * down-sampling from 100 Hz to 10 Hz sampling rate with a
 * cut-off frequency of 5 Hz.
 * */
float firCoeff[NUM_TAPS] = {
	-0.0003665379f, -0.0001341685f, 0.0001502386f, 0.0005086115f,
0.0009491952f, 0.0014538658f, 0.0019692588f, 0.0024044991f,
0.0026373938f, 0.0025295878f, 0.0019495776f, 0.0008008911f,
-0.0009485649f, -0.0032407700f, -0.0059103402f, -0.0086799543f,
-0.0111712385f, -0.0129319206f, -0.0134777697f, -0.0123455552f,
-0.0091513153f, -0.0036469225f, 0.0042324653f, 0.0143369287f,
0.0262915958f, 0.0395106971f, 0.0532359704f, 0.0665964857f,
0.0786841586f, 0.0886371359f, 0.0957218558f, 0.0994046479f,
0.0994046479f, 0.0957218558f, 0.0886371359f, 0.0786841586f,
0.0665964857f, 0.0532359704f, 0.0395106971f, 0.0262915958f,
0.0143369287f, 0.0042324653f, -0.0036469225f, -0.0091513153f,
-0.0123455552f, -0.0134777697f, -0.0129319206f, -0.0111712385f,
-0.0086799543f, -0.0059103402f, -0.0032407700f, -0.0009485649f,
0.0008008911f, 0.0019495776f, 0.0025295878f, 0.0026373938f,
0.0024044991f, 0.0019692588f, 0.0014538658f, 0.0009491952f,
0.0005086115f, 0.0001502386f, -0.0001341685f, -0.0003665379f,
};

/* Function definitions ----------------------------------------------------------------*/

/**
 * @brief  	Receives a pointer to an array of 10 values sampled at 100 Hz.
 * 			The received array is filtered such that it no longer contains
 * 			components above 5 Hz. The function will then store 1 value in
 * 			the address given by *pDst.
 * @param  	*pSrx - Pointer to the input-buffer holding 10 32-bit floating point values.
 * 			*pDst - Pointer to the address where the output should be stored.
 * @retval 	None
 */
void FIR_decimate(float *pSrc, float *pDst, float *pState){


	uint32_t i, taps_teller; // Counter var.

	pCoeff = &firCoeff[0]; /* Peker p� starten av koeffisientvektoren */
	//pTilstand = &firTilstand[0]; /* Peker p� starten av tilstandsbufferet */

	/* pTilstand peker p� starten av de forrige 54 sampler.
	   pStateCurnt peker der nye inngangsverdier skal skrives.
	*/
	 pStateCurnt = pState + (NUM_TAPS - 10u);

	 /* Kopier 10 nye inngangsverdier til tilstandsbufferet*/
	 i = 10;
	 while(i > 0u) {
	 	*pStateCurnt++ = *pSrc++;
	 	i--;
	 } // end while

	 /* Initialiser akkumulator */
	 sum0 = 0.0f;

	 /* Initialiser tilstandspeker*/
	 px = pState;

	 /* Initialiser koeffisientpeker */
	 pb = pCoeff;

	 /* For � minimere l�kke-"overhead" utf�res 4 iterasjoner i hver l�kke.
	 	Deler da NUM_TAPS p� 4 for � f� antall iterasjoner av l�kken.
	 */
	 taps_teller = NUM_TAPS >> 2;

	 while(taps_teller > 0u){
	 	/* Les koeffisient og sample*/
	 	c0 = *(pb++);
	 	x0 = *(px++);

	 	/* Utf�r MAC (Multiply-Accumulate)*/
	 	sum0 += x0 * c0;

	 	/* Gjenta 3 ganger til */
		c0 = *(pb++);
	 	x0 = *(px++);
	 	sum0 += x0 * c0;

		c0 = *(pb++);
	 	x0 = *(px++);
	 	sum0 += x0 * c0;

	 	c0 = *(pb++);
	 	x0 = *(px++);
	 	sum0 += x0 * c0;

	 	taps_teller--;
	 }//end while

	 /* Resultatet ligger i akkumulatoren, lagre i utgangsvariabelen*/
	 *pDst = sum0;

	 /* Kopierer de siste 54 sampler til starten av tilstandsbufferet for
	  * � v�re klar for neste funksjonskall. */
	 pStateCurnt = pState;
	 pState += 10;

	 /* L�kke utrulling. Sparer ca 2*(54-9) = 90 klokkesykler (1.26 mikrosekund). */
	 i = 9;

	 while(i > 0u){
	 	*pStateCurnt++ = *pState++;
	 	*pStateCurnt++ = *pState++;
	 	*pStateCurnt++ = *pState++;
	 	*pStateCurnt++ = *pState++;
	 	*pStateCurnt++ = *pState++;
	 	*pStateCurnt++ = *pState++;
	 	i--;
	 } //end while

//	 /* Resterende kopier*/
//	 //i = (NUM_TAPS - 10u) % 0x04u;
//	 i = (NUM_TAPS - 10u) & 0x03;
//	 while(i > 0u){
//	 	*pTilstandCurnt++ = *pTilstand++;
//	 	i--;
//	 }// end while
}//end nedsamplingsfilter(float *pSrc, float *pDst)
