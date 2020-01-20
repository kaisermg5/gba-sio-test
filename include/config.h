
// Uncomment to print console to the game instead of  
#define F_GRAPHICAL_CONSOLE


// Uncomment for callback system
//#define USE_CALLBACK_SYSTEM

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



//#define U_NORMAL_MODE
#define U_MULTIPLAYER_MODE