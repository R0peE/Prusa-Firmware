//sm4.c - simple 4-axis stepper control

#include "sm4.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <math.h>
#include "Arduino.h"

#include "boards.h"
#define false 0
#define true 1
#include "Configuration_prusa.h"


#ifdef NEW_XYZCAL


// Signal pinouts

// direction signal - MiniRambo
//#define X_DIR_PIN    48 //PL1 (-)
//#define Y_DIR_PIN    49 //PL0 (-)
//#define Z_DIR_PIN    47 //PL2 (-)
//#define E0_DIR_PIN   43 //PL6 (+)

//direction signal - EinsyRambo
//#define X_DIR_PIN    49 //PL0 (+)
//#define Y_DIR_PIN    48 //PL1 (-)
//#define Z_DIR_PIN    47 //PL2 (+)
//#define E0_DIR_PIN   43 //PL6 (-)

//step signal pinout - common for all rambo boards
//#define X_STEP_PIN   37 //PC0 (+)
//#define Y_STEP_PIN   36 //PC1 (+)
//#define Z_STEP_PIN   35 //PC2 (+)
//#define E0_STEP_PIN  34 //PC3 (+)


sm4_stop_cb_t sm4_stop_cb = 0;

sm4_update_pos_cb_t sm4_update_pos_cb = 0;

sm4_calc_delay_cb_t sm4_calc_delay_cb = 0;

uint16_t sm4_cpu_time = 0;


uint8_t sm4_get_dir(uint8_t axis)
{
	switch (axis)
	{
#if ((MOTHERBOARD == BOARD_RAMBO_MINI_1_0) || (MOTHERBOARD == BOARD_RAMBO_MINI_1_3))
	case 0: return (PORTL & 2)?0:1;
	case 1: return (PORTL & 1)?0:1;
	case 2: return (PORTL & 4)?0:1;
	case 3: return (PORTL & 64)?1:0;
#elif (MOTHERBOARD == BOARD_EINSY_1_0a)
	case 0: return (PORTL & 1)?1:0;  // X
	case 1: return (PORTL & 2)?0:1;  // Y
	case 2: return (PORTL & 4)?1:0;  // Z
	case 3: return (PORTL & 64)?0:1; // E
#elif (MOTHERBOARD == BOARD_MKS_GENL_2_0)
	case 0: return (PORTA & 64)?1:0; // X
	case 1: return (PORTC & 8)?0:1;  // Y
	case 2: return (PORTF & 1)?1:0;  // Z
	case 3: return (PORTF & 64)?0:1; // E
#endif
	}
	return 0;
}

void sm4_set_dir(uint8_t axis, uint8_t dir)
{
	switch (axis)
	{
#if ((MOTHERBOARD == BOARD_RAMBO_MINI_1_0) || (MOTHERBOARD == BOARD_RAMBO_MINI_1_3))
	case 0: if (!dir) PORTL |= 2; else PORTL &= ~2; break;
	case 1: if (!dir) PORTL |= 1; else PORTL &= ~1; break;
	case 2: if (!dir) PORTL |= 4; else PORTL &= ~4; break;
	case 3: if (dir) PORTL |= 64; else PORTL &= ~64; break;
#elif (MOTHERBOARD == BOARD_EINSY_1_0a)
	case 0: if (dir) PORTL |= 1; else PORTL &= ~1; break;
	case 1: if (!dir) PORTL |= 2; else PORTL &= ~2; break;
	case 2: if (dir) PORTL |= 4; else PORTL &= ~4; break;
	case 3: if (!dir) PORTL |= 64; else PORTL &= ~64; break;
#elif (MOTHERBOARD == BOARD_MKS_GENL_2_0)
	case 0: if (dir) PORTA |= 64; else PORTA &= ~64; break;
	case 1: if (!dir) PORTC |= 8; else PORTC &= ~8; break;
	case 2: if (dir) PORTF |= 1; else PORTF &= ~1; break;
	case 3: if (!dir) PORTF |= 64; else PORTF &= ~64; break;
#endif
	}
	asm("nop");
}

uint8_t sm4_get_dir_bits(void)
{
    register uint8_t dir_bits = 0;
	//TODO -optimize in asm
#if ((MOTHERBOARD == BOARD_RAMBO_MINI_1_0) || (MOTHERBOARD == BOARD_RAMBO_MINI_1_3))
    register uint8_t portL = PORTL;
	if (portL & 2) dir_bits |= 1;
	if (portL & 1) dir_bits |= 2;
	if (portL & 4) dir_bits |= 4;
	if (portL & 64) dir_bits |= 8;
	dir_bits ^= 0x07; //invert XYZ, do not invert E
#elif (MOTHERBOARD == BOARD_EINSY_1_0a)
    register uint8_t portL = PORTL;
	if (portL & 1) dir_bits |= 1;
	if (portL & 2) dir_bits |= 2;
	if (portL & 4) dir_bits |= 4;
	if (portL & 64) dir_bits |= 8;
	dir_bits ^= 0x0a; //invert YE, do not invert XZ
#elif (MOTHERBOARD == BOARD_MKS_GENL_2_0)
    register uint8_t portF = PORTF;
	if (PORTA & 64) dir_bits |= 1;
	if (PORTC & 8) dir_bits |= 2;
	if (portF & 1) dir_bits |= 4;
	if (portF & 64) dir_bits |= 8;
	dir_bits ^= 0x0a; //invert YE, do not invert XZ
#endif
	return dir_bits;
}

void sm4_set_dir_bits(uint8_t dir_bits)
{
	//TODO -optimize in asm
#if ((MOTHERBOARD == BOARD_RAMBO_MINI_1_0) || (MOTHERBOARD == BOARD_RAMBO_MINI_1_3))
    register uint8_t portL = PORTL;
	portL &= 0xb8; //set direction bits to zero
	dir_bits ^= 0x07; //invert XYZ, do not invert E
	if (dir_bits & 1) portL |= 2;  //set X direction bit
	if (dir_bits & 2) portL |= 1;  //set Y direction bit
	if (dir_bits & 4) portL |= 4;  //set Z direction bit
	if (dir_bits & 8) portL |= 64; //set E direction bit
	PORTL = portL;
#elif (MOTHERBOARD == BOARD_EINSY_1_0a)
    register uint8_t portL = PORTL;
	portL &= 0xb8; //set direction bits to zero
	dir_bits ^= 0x0a; //invert YE, do not invert XZ
	if (dir_bits & 1) portL |= 1;  //set X direction bit
	if (dir_bits & 2) portL |= 2;  //set Y direction bit
	if (dir_bits & 4) portL |= 4;  //set Z direction bit
	if (dir_bits & 8) portL |= 64; //set E direction bit
	PORTL = portL;
#elif (MOTHERBOARD == BOARD_MKS_GENL_2_0)
    register uint8_t portF = PORTF & 0xbe;
	dir_bits ^= 0x0a; //invert YE, do not invert XZ
	if (dir_bits & 1) PORTA |= 64; else PORTA &= ~64;  //set X direction bit
	if (dir_bits & 2) PORTC |= 8; else PORTC &= ~8;    //set Y direction bit
	if (dir_bits & 4) portF |= 1;  //set Z direction bit
	if (dir_bits & 8) portF |= 64; //set E direction bit
	PORTF = portF;
#endif
	asm("nop");
}

void sm4_do_step(uint8_t axes_mask)
{
#if ((MOTHERBOARD == BOARD_RAMBO_MINI_1_0) || (MOTHERBOARD == BOARD_RAMBO_MINI_1_3) || (MOTHERBOARD == BOARD_EINSY_1_0a))
    register uint8_t portC = PORTC & 0xf0;
	PORTC = portC | (axes_mask & 0x0f); //set step signals by mask
	asm("nop");
	PORTC = portC; //set step signals to zero
	asm("nop");
#elif (MOTHERBOARD == BOARD_MKS_GENL_2_0)
	register uint8_t portA = PORTA & 0xbf;
	register uint8_t portC = PORTC & 0xf7;
	register uint8_t portF = PORTF & 0xbe;
    if (axes_mask & 1) portA |= 64;
	if (axes_mask & 2) portC |= 8;
	if (axes_mask & 4) portF |= 1;
	if (axes_mask & 8) portF |= 64;
	PORTA = portA;
	PORTC = portC;
	PORTF = portF;
	PORTA = portA & 0xbf;
	PORTC = portC & 0xf7;
	PORTF = portF & 0xbe;
#endif //((MOTHERBOARD == BOARD_RAMBO_MINI_1_0) || (MOTHERBOARD == BOARD_RAMBO_MINI_1_3) || (MOTHERBOARD == BOARD_EINSY_1_0a))
}

uint16_t sm4_line_xyze_ui(uint16_t dx, uint16_t dy, uint16_t dz, uint16_t de)
{
	uint16_t dd = (uint16_t)(sqrt((float)(((uint32_t)dx)*dx + ((uint32_t)dy*dy) + ((uint32_t)dz*dz) + ((uint32_t)de*de))) + 0.5);
	uint16_t nd = dd;
	uint16_t cx = dd;
	uint16_t cy = dd;
	uint16_t cz = dd;
	uint16_t ce = dd;
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t z = 0;
	uint16_t e = 0;
	while (nd)
	{
		if (sm4_stop_cb && (*sm4_stop_cb)()) break;
		uint8_t sm = 0; //step mask
		if (cx <= dx)
		{
			sm |= 1;
			cx += dd;
			x++;
		}
		if (cy <= dy)
		{
			sm |= 2;
			cy += dd;
			y++;
		}
		if (cz <= dz)
		{
			sm |= 4;
			cz += dd;
			z++;
		}
		if (ce <= de)
		{
			sm |= 4;
			ce += dd;
			e++;
		}
		cx -= dx;
		cy -= dy;
		cz -= dz;
		ce -= de;
		sm4_do_step(sm);
		uint16_t delay = SM4_DEFDELAY;
		if (sm4_calc_delay_cb) delay = (*sm4_calc_delay_cb)(nd, dd);
		if (delay) delayMicroseconds(delay);
		nd--;
	}
	if (sm4_update_pos_cb) (*sm4_update_pos_cb)(x, y, z, e);
	return nd;
}


#endif //NEW_XYZCAL
