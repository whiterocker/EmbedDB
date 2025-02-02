/******************************************************************************/
/**
 * @file        SDFileInterface.c
 * @author      EmbedDB Team (See Authors.md)
 * @brief       Source code file for the SD file interface.
 * @copyright   Copyright 2024
 *              EmbedDB Team
 * @par Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 * @par 1.Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * @par 2.Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * @par 3.Neither the name of the copyright holder nor the names of its contributors
 *  may be used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 * @par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/

#include "SDFileInterface.h"

typedef struct {
  char *filename;
  SD_FILE *sdFile;
} SD_FILE_INFO;

void *setupSDFile(char *filename) {
  SD_FILE_INFO *fileInfo = malloc(sizeof(SD_FILE_INFO));
  int nameLen = strlen(filename);
  fileInfo->filename = calloc(1, nameLen + 1);
  memcpy(fileInfo->filename, filename, nameLen);
  fileInfo->sdFile = NULL;
  return fileInfo;
}

void tearDownSDFile(void *file) {
  SD_FILE_INFO *fileInfo = (SD_FILE_INFO *)file;
  free(fileInfo->filename);
  if (fileInfo != NULL)
    sd_fclose(fileInfo->sdFile);
  free(file);
}

static bool FILE_READ(void *buffer, uint32_t pageNum, uint32_t pageSize, void *file) {
  SD_FILE_INFO *fileInfo = (SD_FILE_INFO *)file;
  sd_fseek(fileInfo->sdFile, pageSize * pageNum, SEEK_SET);
  return (1 == sd_fread(buffer, pageSize, 1, fileInfo->sdFile));
}

static bool FILE_WRITE(void *buffer, uint32_t pageNum, uint32_t pageSize, void *file) {
  bool retval = false;
  SD_FILE_INFO *fileInfo = (SD_FILE_INFO *)file;
  size_t fileSize = sd_length(fileInfo->sdFile);
  size_t requiredSize = pageNum * pageSize;
  if (fileSize < pageNum * pageSize) {
    int seekResult = sd_fseek(fileInfo->sdFile, fileSize, SEEK_SET);
    if (seekResult == 0) {
      size_t currentSize = fileSize;
      uint32_t max = UINT32_MAX;
      size_t   wresult = 1;
      while ((currentSize < requiredSize) && (wresult == 1)) {
	wresult = sd_fwrite(&max, sizeof(uint32_t), 1, fileInfo->sdFile);
	currentSize += (4*wresult);
      }
      if (currentSize >= requiredSize) {
	if (0 == sd_fseek(fileInfo->sdFile, pageNum * pageSize, SEEK_SET)) {
	  retval = (1 == sd_fwrite(buffer, pageSize, 1, fileInfo->sdFile));
	}
      }
    }
  }
  return retval;
}

static bool FILE_ERASE(uint32_t startPage, uint32_t endPage, uint32_t pageSize, void *file) {
  return true;
}

static bool FILE_CLOSE(void *file) {
  SD_FILE_INFO *fileInfo = (SD_FILE_INFO *)file;
  sd_fclose(fileInfo->sdFile);
  fileInfo->sdFile = NULL;
  return true;
}

static bool FILE_FLUSH(void *file) {
  SD_FILE_INFO *fileInfo = (SD_FILE_INFO *)file;
  return (sd_fflush(fileInfo->sdFile) == 0);
}

static bool FILE_OPEN(void *file, uint8_t mode) {
  SD_FILE_INFO *fileInfo = (SD_FILE_INFO *)file;
  
  if (mode == EMBEDDB_FILE_MODE_W_PLUS_B) {
    fileInfo->sdFile = sd_fopen(fileInfo->filename, "w+");
  } else if (mode == EMBEDDB_FILE_MODE_R_PLUS_B) {
    fileInfo->sdFile = sd_fopen(fileInfo->filename, "r+");
  } 
  
  return (bool) fileInfo->sdFile;
}

embedDBFileInterface *getSDInterface(void) {
  embedDBFileInterface *fileInterface = malloc(sizeof(embedDBFileInterface));
  fileInterface->close = FILE_CLOSE;
  fileInterface->read  = FILE_READ;
  fileInterface->write = FILE_WRITE;
  fileInterface->erase = FILE_ERASE;
  fileInterface->open  = FILE_OPEN;
  fileInterface->flush = FILE_FLUSH;
  return fileInterface;
}
