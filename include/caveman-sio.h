
#include <gba_types.h>

#define CAVE_MAN_SIO_VERSION 1

enum CAVE_MAN_SIO_ERRORS {
    CMSE_OK                         = 0,
    CMSE_STOPPED                    = 1,
    CMSE_TIMEOUT                    = 2,
    CMSE_BUSY                       = 3,
    CMSE_CONNECTION_REFFUSED        = 4,
    CMSE_LOST_CONNECTION            = 5,
    CMSE_UNKNOWN                    = 6,
    CMSE_ALREADY_OPENED             = 7,
    CMSE_NO_CONTROLLER_HANDLER      = 8,
    CMSE_UNSYNCED_PARTNER           = 9,
    CMSE_NONMATCHING_VERSIONS       = 10
    // Max value: 511
};

typedef void (*CMS_DATA_HANDLER)(u16 dataSize);
typedef void (*CMS_ERROR_HANDLER)(enum CAVE_MAN_SIO_ERRORS errorCode);

enum CAVE_MAN_SIO_ERRORS caveManSioRecv(u8 * data, const u16 dataSize, CMS_DATA_HANDLER responseHandler);

enum CAVE_MAN_SIO_ERRORS caveManSioSend(const u8 * data, const u16 dataSize, u8 * responseBuffer, 
    const u16 responseSize, CMS_DATA_HANDLER responseHandler);

#define CMS_DEFAULT_TIMEOUT (60*5)

enum CAVE_MAN_SIO_ERRORS caveMainSioSetTimeout(const s32 timeout);

enum CAVE_MAN_SIO_ERRORS caveManSioSetErrorHandler(CMS_ERROR_HANDLER errorHandler);

enum CAVE_MAN_SIO_ERRORS caveManSioOpen(void);

void caveManSioClose(void);

bool caveManSioIsConnected(void);

void handleCaveManSio(void);