
#include <gba_sio.h>
#include "caveman-sio.h"

#include "config.h"
#include <stdio.h>

typedef void (*CMS_CONTROLLER_HANDLER)(void);

enum CAVE_MAN_SIO_STATES {
    CMSS_CLOSED              = 0,
    CMSS_STAND_BY            = 1,
    CMSS_SENDING             = 2,
    CMSS_RECEIVING           = 3,
    CMSS_PROCESSING_DATA     = 4,
    CMSS_WAITING_RESPONSE    = 5
    // Max value:  127
};
/**
 * Data Structure
 * 
 **/
struct CaveManSioController {
    s32 timer;
    s32 timeout;
    u16 state: 7;
    u16 msgClock: 3; // 10
    u16 newDataFlag: 1; // 11
    u16 lastSentReceviedDataFlag: 1; // 12
    u16 msgsWithoutResponse: 2; // 14
    u16 unusedData1: 2; // 16
    u16 lastError: 9;
    u16 unusedData2: 7;
    // Data
    u8 * recvBuffer;
    u16 recvBitsCount;
    u16 recvSize;
    u16 lastReceivedData[2];
    const u8 * sendBuffer;
    u16 sendBitsCount;
    u16 sendSize;
    u16 lastSentData[2];
    // Handlers
    CMS_DATA_HANDLER dataHandler;
    CMS_ERROR_HANDLER errorHandler;
    CMS_CONTROLLER_HANDLER controllerHandler;
};
static struct CaveManSioController cmsController = {
    .state = CMSS_CLOSED
};

/**
 * Static functions
 * 
 **/
static enum CAVE_MAN_SIO_ERRORS cmsCheckBasicErrors(void) {
    enum CAVE_MAN_SIO_ERRORS ret;
    if (cmsController.state == CMSE_STOPPED) {
        ret = CMSE_STOPPED;
    } else if (cmsController.state != CMSS_STAND_BY) {
        ret = CMSE_BUSY;
    } else {
        ret = CMSE_OK;
    }
    cmsController.lastError = ret;
    return ret;
}

static bool caveManSioThrowError(enum CAVE_MAN_SIO_ERRORS err) {
    mgba_printf(0, "err:%d\n", err);
    if (cmsController.errorHandler != NULL) {
        cmsController.errorHandler(err);
        return true;
    }
    return false;
}

static inline bool allConsolesReady(void) {
    return REG_SIOCNT & (1<<3);
}

static inline bool connectedAsParent(void) {
    return !(REG_SIOCNT & (1<<2));
}

#define CMS_DATA_MASK 0x3FFF

static inline void incrementMessageClock(void) {
    // msgClock should never be 0b11
    if (cmsController.msgClock < 0b10) {
        cmsController.msgClock++;
    } else {
        cmsController.msgClock = 0;
    }
}

static inline void setDataToSend(u16 data) {
    cmsController.lastSentData[cmsController.lastSentReceviedDataFlag] = (cmsController.msgClock) | (data & CMS_DATA_MASK);
}

static inline u16 getDataToSend(void) {
    return cmsController.lastSentData[cmsController.lastSentReceviedDataFlag];
}

static inline u16 getPreviouslySentData(void) {
    return cmsController.lastSentData[!cmsController.lastSentReceviedDataFlag];
}

static inline u16 getReceivedData(void) {
    return cmsController.lastReceivedData[cmsController.lastSentReceviedDataFlag];
}

static inline u16 getPreviouslyReceivedData(void) {
    return cmsController.lastReceivedData[!cmsController.lastSentReceviedDataFlag];
}


static void cmsDoDataTransfer(void) {
    bool asParent = connectedAsParent();
    u32 i = 0;
    while (i < 1) {
        setDataToSend(asParent ? 0x123 : 0x456);
        REG_SIOMLT_SEND = getDataToSend();
        u16 receivedData, sentData;
        bool syncedData;
        if (asParent) {
            REG_SIOCNT |= (1<<7);
            receivedData = REG_SIOMULTI1;
            sentData = getDataToSend();
            mgba_printf(0, "0x%x\n", receivedData);
            syncedData = (sentData >> 14) == (receivedData >> 14);
        } else {
            receivedData = REG_SIOMULTI0;
            sentData = REG_SIOMULTI1;
            syncedData = sentData == getDataToSend() && (sentData >> 14) == (receivedData >> 14);
        }
        if (syncedData) {
            mgba_printf(0, "%c: synced!\n", asParent? 'p' : 'c');
        } else {
            mgba_printf(0, "%c: not synced...\n", asParent? 'p' : 'c');
        }
        i++;
    }
}


/**
 * Data handlers
 * 
 **/


static void caveManSioDataHandler_StandBy(void) {
    if (allConsolesReady()) {
        mgba_puts("connected!");
        cmsController.controllerHandler = NULL;
    }
}




/**
 * Public functions
 * 
 **/


enum CAVE_MAN_SIO_ERRORS caveManSioOpen(void) {
    if (cmsController.state == CMSS_CLOSED) {
        // Set controller values
        cmsController.state = CMSS_STAND_BY;
        cmsController.lastError = CMSE_OK;
        cmsController.timer = -1;
        cmsController.msgClock = 0;
        cmsController.newDataFlag = 0;
        cmsController.lastSentReceviedDataFlag = 0;
        cmsController.msgsWithoutResponse = 0;

        // Set registers
        // Enter multiplayer mode
        REG_RCNT = R_MULTI;
        REG_SIOCNT &= ~(1<<12);
        REG_SIOCNT |= (1<<13);
        // Set data as placeholder
        REG_SIOMLT_SEND = 0x1234;
        mgba_puts("wait for connection...");
        cmsController.controllerHandler = caveManSioDataHandler_StandBy;

        return CMSE_OK;
    }
    return CMSE_ALREADY_OPENED;
}

void caveManSioClose(void) {
    REG_RCNT = 0;
    REG_SIOCNT = 0;
    cmsController.state = CMSS_CLOSED;
}


enum CAVE_MAN_SIO_ERRORS caveMainSioSetTimeout(const s32 timeout) {
    enum CAVE_MAN_SIO_ERRORS ret = cmsCheckBasicErrors();
    if (ret == CMSE_OK) {
        cmsController.timeout = timeout;
    }
    return ret;
}

enum CAVE_MAN_SIO_ERRORS caveManSioSetErrorHandler(CMS_ERROR_HANDLER errorHandler) {
    enum CAVE_MAN_SIO_ERRORS ret = cmsCheckBasicErrors();
    if (ret == CMSE_OK) {
        cmsController.errorHandler = errorHandler;
    }
    return ret;
}
#define TOO_MANY_MESSAGES_WITHOUT_RESPONSE 0b11
bool caveManSioIsConnected(void) {
    return cmsController.state != CMSS_CLOSED && cmsController.msgsWithoutResponse < TOO_MANY_MESSAGES_WITHOUT_RESPONSE;
}

enum CAVE_MAN_SIO_ERRORS caveManSioRecv(u8* data, const u16 dataSize, CMS_DATA_HANDLER responseHandler) {
    enum CAVE_MAN_SIO_ERRORS ret = cmsCheckBasicErrors();
    if (ret == CMSE_OK) {
        cmsController.recvBuffer = data;
        cmsController.recvSize = dataSize;
        cmsController.recvBitsCount = 0;
        cmsController.dataHandler = responseHandler;
        cmsController.timer = 0;
        cmsController.state = CMSS_RECEIVING;
    }
    return ret;
}

enum CAVE_MAN_SIO_ERRORS caveManSioSend(const u8 * data, const u16 dataSize, u8 * responseBuffer, 
    const u16 responseSize, CMS_DATA_HANDLER responseHandler) 
{
    enum CAVE_MAN_SIO_ERRORS ret = cmsCheckBasicErrors();
    if (ret == CMSE_OK) {
        cmsController.sendBuffer = data;
        cmsController.sendSize = dataSize;
        cmsController.sendBitsCount = 0;
        cmsController.recvBuffer = responseBuffer;
        cmsController.recvSize = responseSize;
        cmsController.recvBitsCount = 0;
        cmsController.dataHandler = responseHandler;
        cmsController.timer = 0;
        cmsController.state = CMSS_SENDING;
    }
    return ret;
}

void handleCaveManSio(void) {
    if (cmsController.state != CMSS_CLOSED) {
        if (allConsolesReady()) {
            cmsDoDataTransfer();
            // if (cmsController.newDataFlag && cmsController.controllerHandler != NULL) {
            //     cmsController.newDataFlag = false;
            //     cmsController.controllerHandler();
            // } else {
            //     caveManSioThrowError(CMSE_NO_CONTROLLER_HANDLER);
            //     caveManSioClose();
            // }
        }
        // Handle timeout
        if (cmsController.timeout > 0) {
            if (cmsController.timer >= cmsController.timeout) {
                caveManSioThrowError(CMSE_TIMEOUT);
                caveManSioClose();
            } else if (cmsController.timer >= 0) {
                cmsController.timer++;
            }
        }
    }
}
