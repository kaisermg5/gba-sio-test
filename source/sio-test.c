
// Uncomment to print console to the game instead of  
//#define F_GRAPHICAL_CONSOLE


// Uncomment for callback system
//#define USE_CALLBACK_SYSTEM

#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_sio.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef F_GRAPHICAL_CONSOLE
#include <gba_console.h>
#define mgba_puts(x) puts(x)
#define mgba_printf(par1, par2, par3) printf(par2, (unsigned int) (par3))
#else
#include "mgba.h"
static inline void mgba_puts(const char * string) {
	mgba_printf(MGBA_LOG_DEBUG, string);
}
#endif


typedef void (*CALLBACK_FUNC)(void);

CALLBACK_FUNC sioCallback = NULL;

void SIO_irq(void) {
	mgba_printf(MGBA_LOG_DEBUG, "%d\n", (int)REG_SIODATA32);
	sioCallback = NULL;
	REG_IF |= IRQ_SERIAL;
}

void SIO_waitIntr(void) {
	mgba_puts("wait irq");
	IntrWait(IRQ_SERIAL, IRQ_SERIAL);
	mgba_puts("after wait");
	sioCallback = NULL;
}


// Master
void MasterWaitSiLowAndStart(void) {
	//printf("0x%x\n", (int)(REG_SIOCNT & SIO_RDY));
	if (!(REG_SIOCNT & SIO_RDY)) {
		mgba_puts("slave ready");
		REG_SIOCNT |= SIO_START;
		sioCallback = SIO_waitIntr;
	}
}

void InitializeMasterData(void) {
	REG_SIODATA32 = 123456;
	sioCallback = MasterWaitSiLowAndStart;
	mgba_puts("wait for slave");
}

void InitializeMaster(void) {
	mgba_puts("setup");
	REG_RCNT = R_NORMAL;
	REG_SIOCNT = SIO_32BIT | SIO_IRQ | SIO_CLK_INT;
	sioCallback = InitializeMasterData;

	irqSet(IRQ_SERIAL, SIO_irq);
	irqEnable(IRQ_SERIAL);
}


// Slave
void Slave_soLow(void) {
	mgba_puts("low");
	REG_SIOCNT &= ~SIO_SO_HIGH;
	sioCallback = SIO_waitIntr;
}

void Slave_start1_so_high(void) {
	REG_SIOCNT |= SIO_START | SIO_SO_HIGH;
	sioCallback = Slave_soLow;
}
void Slave_start0_soLow(void) {
	REG_SIOCNT &= ~(SIO_START | SIO_SO_HIGH);
	sioCallback = Slave_start1_so_high;
}

void InitializeSlaveData(void) {
	REG_SIODATA32 = 987654;
	sioCallback = Slave_start0_soLow;
}

void InitializeSlave(void) {
	mgba_puts("setup");
	REG_RCNT = R_NORMAL;
	REG_SIOCNT = SIO_32BIT | SIO_IRQ;
	sioCallback = InitializeSlaveData;

	irqSet(IRQ_SERIAL, SIO_irq);
	irqEnable(IRQ_SERIAL);
}

// All in one
void allInOneSIO(bool isMaster) {
	if (REG_SIOCNT & (1<<7)) {
		mgba_puts("error: already started");
		return;
	}
	// 0?
	REG_SIOCNT &= ~(1<<13);
	// 1
	REG_RCNT &= ~(1<<15);
	// 2
	REG_SIOCNT &= ~(1<<5);
	REG_SIOCNT |= (1<<4);
	// 3
	REG_SIODATA32 = isMaster ? 0x1234 : 0x9876;
	// 4
	REG_SIOCNT |= (1<<3);

	s32 timer = 0;
	if (isMaster) {
		// m1
		REG_SIOCNT |= 1;
		REG_SIOCNT &= ~(1<<1);
		// m2
		mgba_puts("wait for slave");
		while (REG_SIOCNT & (1<<2)) {
			timer++;
		}
		mgba_puts("slave ready");
		// m3
		REG_SIOCNT |= (1<<7);
	} else {
		// s1
		REG_SIOCNT &= ~1;
		// s2
		REG_SIOCNT &= ~(1<<3);
		// m3
		REG_SIOCNT |= (1<<7);
	}

	mgba_puts("wait...");
	while (REG_SIOCNT & (1<<7)) {
		timer++;
	}
	mgba_puts("done?");
	mgba_printf(MGBA_LOG_DEBUG, "val: 0x%x\n", (s32)REG_SIODATA32);
	mgba_printf(MGBA_LOG_DEBUG, "timer: 0x%x\n", timer);
}



// Delay between calls to SIO func callbacks
const u16 FRAME_DELAY = 0;//60;

int main(void) {
//---------------------------------------------------------------------------------


	// the vblank interrupt must be enabled for VBlankIntrWait() to work
	// since the default dispatcher handles the bios flags no vblank handler
	// is required
	irqInit();
	irqEnable(IRQ_VBLANK);

#ifdef F_GRAPHICAL_CONSOLE
	consoleDemoInit();
#else
	mgba_console_open();
#endif
	
	u16 currKeys;
	bool initializeFlag = true;
	u16 i = 0;
	while (1) {
		// Read keys
		scanKeys();
		currKeys = keysDown();

		// Initialize 
		// If A is pressed: initialize master
		// If B is pressed: initialize slave
		if (initializeFlag) {
			if (currKeys & KEY_A) {
				mgba_puts("master");
				#ifdef USE_CALLBACK_SYSTEM
					sioCallback = InitializeMaster;
				#else
					allInOneSIO(true);
				#endif
				
				initializeFlag = false;
			} else if (currKeys & KEY_B) {
				mgba_puts("slave");
				#ifdef USE_CALLBACK_SYSTEM
					sioCallback = InitializeSlave;
				#else
					allInOneSIO(false);
				#endif
				initializeFlag = false;
			}
		}
		
		// Call sio callback
		if (sioCallback != NULL) {
			if (i == 0) {
				sioCallback();
			}
			if (i < FRAME_DELAY) {
				i++;
			} else {
				i = 0;
			}
		}
		VBlankIntrWait();
	}
}


