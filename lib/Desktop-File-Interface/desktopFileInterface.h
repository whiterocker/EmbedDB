#if !defined(DESKTOP_FILE_INTERFACE)
#define DESKTOP_FILE_INTERFACE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#if defined(DIST)
#include "embedDB.h"
#else
#include "../../src/embedDB/embedDB.h"
#endif

/* File functions */
embedDBFileInterface *getFileInterface(void);
embedDBFileInterface *getMockEraseFileInterface(void);
void *setupFile(char *filename);
void tearDownFile(void *file);

#ifdef __cplusplus
}
#endif

#endif
