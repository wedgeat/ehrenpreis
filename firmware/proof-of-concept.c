/*
 * Copyright (c) 2013 - 2015,
 * Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * Copyright 2026 AW
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * PROOF OF CONCEPT CODE
 * Electronic chime using LPC802
 * Code should be portable to other ARM
 * microcontrollers with similar peripherals.
 * PWM period 320 @ 15 MHz
 * Sample timer period 640 @ 15 MHz
 *
 * AW 2026-10-25
 */

#include "fsl_device_registers.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "peripherals.h"

#include "board.h"
#include "app.h"

void MRT0_IRQHANDLER(void);
static inline uint_fast8_t decay(int32_t);
static inline int_fast16_t sine(uint32_t);
static const int_fast16_t sineAmpl = 0x400;
static const uint_fast8_t decayAmpl = 128;

struct chord {
	uint_fast8_t x1;
	uint_fast8_t x2;
	uint_fast8_t x3;
	uint_fast8_t x4;
	int32_t p1;
	int32_t p2;
	int32_t p3;
	int32_t p4;
	uint_fast16_t smax;
};

struct tune {
	struct chord chords[128];
	uint8_t len;
};

void chime(const struct chord *cc);
void melody(const struct tune *t);

/*!
 * @brief Flag to calculate next sample
 */
volatile uint_fast8_t nextSample = 0;

/*!
 * @brief Main function
 */
int main(void) {

    /* Init board hardware. */
    BOARD_InitHardware();
    BOARD_InitPeripherals();
    BOARD_InitLEDsPins();

    CTIMER_EnableMatchChannelReload(CTIMER0_PERIPHERAL, CTIMER0_PWM_0_CHANNEL, 1);
    CTIMER_SetShadowValue(CTIMER0_PERIPHERAL, CTIMER0_PWM_0_CHANNEL, CTIMER0_PWM_PERIOD/2);
    CTIMER_StartTimer(CTIMER0_PERIPHERAL);

    EnableIRQ(MRT0_IRQN);

    static const struct chord chimes[17] = {
		/* 0: SAE800 3-tone */
		{ 24, 20, 16, 0, 0, 50, 100, INT32_MAX, 360 },
		/* 1: SAE800 2-tone */
		{ 24, 20, 0, 0, 0, 50, INT32_MAX, INT32_MAX, 310 },
		/* 2: SAE800 1-tone */
		{ 24, 0, 0, 0, 0, INT32_MAX, INT32_MAX, INT32_MAX, 260 },
		/* 3: Mono-beep */
		{ 18, 18, 18, 18, 0, INT32_MAX, INT32_MAX, INT32_MAX, 320 },
		/* 4: Duo-beep */
		{ 18, 18, 18, 18, 0, 80, INT32_MAX, INT32_MAX, 400 },
		/* 5: Tri-beep */
		{ 18, 18, 18, 18, 0, 80, 160, INT32_MAX, 480 },
		/* 6: Quadri-beep */
		{ 18, 18, 18, 18, 0, 80, 160, 240, 560 },
		/* 7: Chord */
		{ 32, 40, 48, 24, 10, 5, 20, 15, 320 },
		/* 8: SAE800 3-tone high */
		{ 48, 40, 32, 0, 0, 50, 100, INT32_MAX, 360 },
		/* 9: SAE800 2-tone high */
		{ 48, 40, 0, 0, 0, 50, INT32_MAX, INT32_MAX, 310 },
		/* 10: SAE800 1-tone high */
		{ 48, 0, 0, 0, 0, INT32_MAX, INT32_MAX, INT32_MAX, 260 },
		/* 11: Ready */
		{ 16, 20, 24, 32, 0, 33, 67, 100, 480 },
		/* 12: 2-tone */
		{ 40, 20, 32, 16, 4, 0, 64, 60, 320 },
		/* 13: The Jetsons */
		{ 32, 40, 46, 48, 0, 50, 100, 125, 400 },
		/* 14: Announcement */
		{ 16, 24, 20, 24, 0, 33, 67, 99, 300 },
		/* 15: Utopia */
		{ 30, 24, 20, 16, 0, 0, 40, 40, 320 },
		/* 16: 2-tone */
		{ 24, 24, 24, 24, 0, 8, 96, 104, 320 }
    };

    static const struct tune tunes[1] = {
    /* La Cumparsita -- slightly detuned due to rounding */
		{
			{
				{ .x1 = 14, .x2 = 27, .x3 = 41, .x4 = 54, .smax = 10 },
				{ .x1 = 12, .x2 = 24, .x3 = 41, .x4 = 48, .smax = 10 },
				{ .x1 = 11, .x2 = 22, .x3 = 36, .x4 = 43, .smax = 10 },
				{ .x1 = 10, .x2 = 20, .x3 = 31, .x4 = 41, .smax = 10 },
				{ .x1 = 72, .x2 = 20, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 61, .x2 = 13, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 51, .x2 = 10, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 10, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 41, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 43, .x2 = 20, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 41, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 38, .x2 = 13, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 20, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 32, .x3 = 14, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 32, .x3 = 82, .x4 = 27, .smax = 10 },
				{ .x1 = 41, .x2 = 32, .x3 = 64, .x4 = 14, .smax = 10 },
				{ .x1 = 41, .x2 = 32, .x3 = 54, .x4 = 20, .smax = 10 },
				{ .x1 = 14, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 41, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 27, .x2 = 43, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 41, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 14, .x2 = 38, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 20, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 31, .x2 = 41, .x3 = 10, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 31, .x3 = 72, .x4 = 20, .smax = 10 },
				{ .x1 = 41, .x2 = 31, .x3 = 61, .x4 = 13, .smax = 10 },
				{ .x1 = 41, .x2 = 31, .x3 = 51, .x4 = 10, .smax = 10 },
				{ .x1 = 10, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 41, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 20, .x2 = 43, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 41, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 13, .x2 = 38, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 10, .x2 = 41, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 14, .x3 = 32, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 27, .x3 = 32, .x4 = 82, .smax = 10 },
				{ .x1 = 41, .x2 = 14, .x3 = 32, .x4 = 64, .smax = 10 },
				{ .x1 = 41, .x2 = 32, .x3 = 54, .x4 = 20, .smax = 10 },
				{ .x1 = 14, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 41, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 43, .x2 = 27, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 41, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 38, .x2 = 14, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 20, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 36, .x2 =  9, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 36, .x2 = 54, .x3 = 18, .x4 =  0, .smax = 10 },
				{ .x1 = 36, .x2 = 51, .x3 =  9, .x4 =  0, .smax = 10 },
				{ .x1 = 36, .x2 = 54, .x3 = 18, .x4 =  0, .smax =  5 },
				{ .x1 = 14, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 18, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 36, .x2 = 51, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 36, .x2 = 54, .x3 = 18, .x4 =  0, .smax =  5 },
				{ .x1 = 36, .x2 = 51, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 36, .x2 = 54, .x3 = 18, .x4 =  0, .smax = 10 },
				{ .x1 = 51, .x2 = 36, .x3 = 18, .x4 =  0, .smax = 10 },
				{ .x1 = 32, .x2 = 14, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 32, .x2 = 54, .x3 = 27, .x4 =  0, .smax = 10 },
				{ .x1 = 32, .x2 = 51, .x3 = 14, .x4 =  0, .smax = 10 },
				{ .x1 = 32, .x2 = 54, .x3 = 20, .x4 =  0, .smax =  5 },
				{ .x1 = 10, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 14, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 32, .x2 = 51, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 14, .x2 = 32, .x3 = 54, .x4 =  0, .smax =  5 },
				{ .x1 = 32, .x2 = 51, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 14, .x2 = 32, .x3 = 54, .x4 =  0, .smax = 10 },
				{ .x1 = 14, .x2 = 32, .x3 = 51, .x4 =  0, .smax = 10 },
				{ .x1 = 10, .x2 = 31, .x3 = 26, .x4 =  0, .smax = 10 },
				{ .x1 = 41, .x2 = 31, .x3 = 26, .x4 = 20, .smax = 10 },
				{ .x1 = 36, .x2 = 31, .x3 = 26, .x4 = 13, .smax = 10 },
				{ .x1 = 41, .x2 = 31, .x3 = 26, .x4 = 10, .smax =  5 },
				{ .x1 = 15, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 10, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 36, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 32, .x2 = 10, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 31, .x2 =  0, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 27, .x2 = 10, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 26, .x2 = 10, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 27, .x2 = 14, .x3 = 20, .x4 =  0, .smax = 15 },
				{ .x1 = 22, .x2 = 11, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 20, .x2 = 10, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 = 18, .x2 =  9, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 =  8, .x2 = 16, .x3 =  0, .x4 =  0, .smax =  5 },
				{ .x1 =  8, .x2 = 15, .x3 =  0, .x4 =  0, .smax = 10 },
				{ .x1 = 14, .x2 = 10, .x3 = 20, .x4 = 27, .smax = 20 },
			},
			83
		}
    };

    while (1) {
    	for (uint_fast8_t c = 0; c < 17; c++) chime(&chimes[c]);
		melody(&tunes[0]);
    }
}

/*!
 * @brief Chime functionality
 */
void chime(const struct chord *cc) {
	/* Counter */
	uint32_t i = 0;
	int32_t s = 0;

	/* Calculate maximum amplitude */
	int32_t amax = 0;
	for (uint_fast16_t j = 0; j <= cc->smax; j++) {
		amax = MAX(decay(j - cc->p1) + decay(j - cc->p2) + decay(j - cc->p3) + decay(j - cc->p4), amax);
	}

	static const int32_t ymax = CTIMER0_PWM_PERIOD + 1;

	/* Intermediate values */
	int32_t a1;
	int32_t a2;
	int32_t a3;
	int32_t a4;

	int32_t y1;
	int32_t y2;
	int32_t y3;
	int32_t y4;

	/* Final value */
	int32_t y = 0;

	while (1)
	{
		/* Step */
		s = 5 * i / 0x400;

		a1 = decay(s - cc->p1);
		a2 = decay(s - cc->p2);
		a3 = decay(s - cc->p3);
		a4 = decay(s - cc->p4);

		if (cc->smax <= s) {
			i = 0;
			break;
		}

		/* Calculate value */
		y1 = sine(cc->x1 * i) * a1;
		y2 = sine(cc->x2 * i) * a2;
		y3 = sine(cc->x3 * i) * a3;
		y4 = sine(cc->x4 * i) * a4;

		y = y1 + y2 + y3 + y4;

		/* Normalise sineAmpl */
		y *= ymax / 2;
		y /= sineAmpl * amax;

		/* Convert to unsigned */
		y += ymax / 2;

		/* Save value and set flag */
		CTIMER_SetShadowValue(
				CTIMER0_PERIPHERAL,
				CTIMER0_PWM_0_CHANNEL,
				(uint32_t) y);

		i++;

		/* Indicate underflow */
		if (nextSample) LED_RED_ON();
		else LED_RED_OFF();

		while (!nextSample);
		nextSample = 0;
	}
}

/*!
 * @brief Melody functionality
 */
void melody(const struct tune *t) {
	for (uint_fast8_t k = 0; k < t->len; k++){
		const struct chord *cc = &(t->chords[k]);

		/* Counter */
		uint32_t i = 0;
		int32_t s = 0;

		/* Calculate maximum amplitude */
		int32_t amax = 4 * decayAmpl;

		static const int32_t ymax = CTIMER0_PWM_PERIOD + 1;

		/* Intermediate values */
		int32_t y1;
		int32_t y2;
		int32_t y3;
		int32_t y4;

		/* Final value */
		int32_t y = 0;

		while (1)
		{
			/* Step */
			s = 11 * i / 0x400;

			if (cc->smax <= s / 8) {
				break;
			}

			/* Calculate value */
			y1 = sine(cc->x1 * i);
			y2 = sine(cc->x2 * i);
			y3 = sine(cc->x3 * i);
			y4 = sine(cc->x4 * i);

			y = y1 + y2 + y3 + y4;

			y *= decay(s);

			/* Normalise sineAmpl */
			y *= ymax / 2;
			y /= sineAmpl * amax;

			/* Convert to unsigned */
			y += ymax / 2;

			/* Save value and set flag */
			CTIMER_SetShadowValue(
					CTIMER0_PERIPHERAL,
					CTIMER0_PWM_0_CHANNEL,
					(uint32_t) y);

			i++;

			/* Indicate underflow */
			if (nextSample) LED_RED_ON();
			else LED_RED_OFF();

			while (!nextSample);
			nextSample = 0;
		}
	}
}

/*!
 * @brief Sine lookup table
 */
static const int_fast16_t sineTable[256] = { 0, 6, 13, 19, 25, 31, 38, 44, 50, 57, 63, 69, 75, 82, 88, 94, 100, 107, 113, 119, 125, 132, 138, 144, 150, 156, 163, 169, 175, 181, 187, 194, 200, 206, 212, 218, 224, 230, 237, 243, 249, 255, 261, 267, 273, 279, 285, 291, 297, 303, 309, 315, 321, 327, 333, 339, 345, 351, 357, 363, 369, 374, 380, 386, 392, 398, 403, 409, 415, 421, 426, 432, 438, 443, 449, 455, 460, 466, 472, 477, 483, 488, 494, 499, 505, 510, 516, 521, 526, 532, 537, 543, 548, 553, 558, 564, 569, 574, 579, 584, 590, 595, 600, 605, 610, 615, 620, 625, 630, 635, 640, 645, 650, 654, 659, 664, 669, 674, 678, 683, 688, 692, 697, 702, 706, 711, 715, 720, 724, 729, 733, 737, 742, 746, 750, 755, 759, 763, 767, 771, 775, 779, 784, 788, 792, 796, 799, 803, 807, 811, 815, 819, 822, 826, 830, 834, 837, 841, 844, 848, 851, 855, 858, 862, 865, 868, 872, 875, 878, 882, 885, 888, 891, 894, 897, 900, 903, 906, 909, 912, 915, 917, 920, 923, 926, 928, 931, 934, 936, 939, 941, 944, 946, 948, 951, 953, 955, 958, 960, 962, 964, 966, 968, 970, 972, 974, 976, 978, 980, 982, 983, 985, 987, 989, 990, 992, 993, 995, 996, 998, 999, 1000, 1002, 1003, 1004, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1016, 1017, 1018, 1018, 1019, 1020, 1020, 1021, 1021, 1022, 1022, 1022, 1023, 1023, 1023, 1024, 1024, 1024, 1024, 1024, };

/*!
 * @brief Decay lookup table
 */
static const uint_fast8_t decayTable[256] = { 0, 4, 16, 64, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 127, 124, 121, 117, 114, 111, 108, 106, 103, 100, 98, 95, 92, 90, 88, 85, 83, 81, 79, 77, 75, 73, 71, 69, 67, 65, 64, 62, 60, 59, 57, 56, 54, 53, 51, 50, 49, 47, 46, 45, 44, 43, 42, 40, 39, 38, 37, 36, 35, 34, 34, 33, 32, 31, 30, 29, 29, 28, 27, 26, 26, 25, 24, 24, 23, 23, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16, 16, 16, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10, 10, 9, 9, 9, 9, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, };

/*!
 * @brief Decay helper function
 */
static inline uint_fast8_t decay(int32_t step) {
	return decayTable[(size_t)MIN(255, MAX(0, step))];
}

/*!
 * @brief Sine helper function
 */
static inline int_fast16_t sine(uint32_t i) {
	/* Q4 */
	if ((i % 1024) > 767)
		return -1 * sineTable[255 - (i % 256)];
	/* Q3 */
	else if ((i % 1024) > 511)
		return -1 * sineTable[      (i % 256)];
	/* Q2 */
	else if ((i % 1024) > 255)
		return      sineTable[255 - (i % 256)];
	/* Q1 */
	else
	    return      sineTable[      (i % 256)];
}

/*!
 * @brief MRT callback
 */
void MRT0_IRQHANDLER(void) {
  MRT_ClearStatusFlags(MRT0_PERIPHERAL, MRT0_CHANNEL_0, kMRT_TimerInterruptFlag);
  nextSample = 1;

  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
     Store immediate overlapping exception return operation might vector to incorrect interrupt. */
  #if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
  #endif
}
