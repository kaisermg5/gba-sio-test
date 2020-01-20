
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_sio.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"

typedef void (*callbackFn)(void);

static callbackFn callback = NULL;

static u16 dataToSend;

void parentCallback(void) {
    if (!(REG_SIOCNT & (1<<7))) {
        mgba_puts("transfer");
        REG_SIOMLT_SEND = dataToSend;
        REG_SIOCNT |= (1<<7);
        mgba_printf(MGBA_LOG_DEBUG, "0x%x\n", REG_SIOMULTI0);
        mgba_printf(MGBA_LOG_DEBUG, "0x%x\n", REG_SIOMULTI1);
        mgba_printf(MGBA_LOG_DEBUG, "0x%x\n", REG_SIOMULTI2);
        mgba_printf(MGBA_LOG_DEBUG, "0x%x\n", REG_SIOMULTI3);
    }
}

void childCallback(void) {
    REG_SIOMLT_SEND = dataToSend;
    mgba_printf(MGBA_LOG_DEBUG, "0x%x\n", ((REG_SIOCNT) >> 4) & 0b11);
}

void setupMultiplayer(void) {
    // Set (0) in (d15) of Register RCNT
    REG_RCNT  &= ~(1<<15);

    // Set (0,1) in (d12,d13) of Control Register SIOCNT
    REG_SIOCNT &= ~(1<<12);
    REG_SIOCNT |= (1<<13);

    // Set transfer data
    REG_SIOMLT_SEND = 0x1234;

    // Is (d03) of Register SIOCNT, (1)?
    // Yes -> continue
    // No  -> wait
    mgba_puts("wait for others");
    while (!(REG_SIOCNT & (1<<3))) ;
    mgba_puts("connected!");

    // Is (d02) of Register SIOCNT, (0)?
    if (REG_SIOCNT & (1<<2)) {
        // child
        mgba_puts("child");
        dataToSend = 0x9876;
        callback = childCallback;
    } else {
        // parent
        mgba_puts("parent");
        dataToSend = 0x1234;
        callback = parentCallback;
    }
}
static bool flag = true;

u32 timer = 0;
s32 counter = 0;
void multiplayerModeTest(void) {
    u16 currKeys = keysDown();
    if (flag && currKeys & KEY_A) {
        mgba_puts("test");
        setupMultiplayer();
    }
    if (callback != NULL) {
        if (timer == 0) {
            mgba_printf(MGBA_LOG_DEBUG, "time: %d\n", counter);
            callback();
            counter++;
            dataToSend--;
        }
        if (timer < 180) {
            timer++;
        } else {
            timer = 0;
        }
    }
}