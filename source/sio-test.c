
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

#include "config.h"
#include "caveman-sio.h"


void normalModeTest(void);
void multiplayerModeTest(void);
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
 	bool flag = true;
	while (1) {
		scanKeys();
		if (flag && keysDown() & KEY_A) {
			flag = false;
			caveManSioOpen();
		}
#ifdef U_NORMAL_MODE
		normalModeTest();
#else
#ifdef U_MULTIPLAYER_MODE
		//multiplayerModeTest();
		handleCaveManSio();
#endif
#endif
		VBlankIntrWait();
	}
}


