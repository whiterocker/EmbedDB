#include "desktopFileInterface.h"

typedef struct {
    char *filename;
    FILE *file;
} FILE_INFO;

void * setupFile(char *filename) {
  FILE_INFO *fileInfo = malloc(sizeof(FILE_INFO));
  int nameLen = strlen(filename);
  fileInfo->filename = calloc(1, nameLen + 1);
  memcpy(fileInfo->filename, filename, nameLen);
  fileInfo->file = NULL;
  return fileInfo;
}

void tearDownFile(void *file) {
  FILE_INFO *fileInfo = (FILE_INFO *)file;
  free(fileInfo->filename);
  if (fileInfo->file != NULL)
    fclose(fileInfo->file);
  free(file);
}

static bool FILE_READ(void *buffer, uint32_t pageNum, uint32_t pageSize, void *file) {
  FILE_INFO *fileInfo = (FILE_INFO *)file;
  fseek(fileInfo->file, pageSize * pageNum, SEEK_SET);
  return (1 == fread(buffer, pageSize, 1, fileInfo->file));
}

static bool FILE_WRITE(void *buffer, uint32_t pageNum, uint32_t pageSize, void *file) {
  FILE_INFO *fileInfo = (FILE_INFO *)file;
  fseek(fileInfo->file, pageNum * pageSize, SEEK_SET);
  return (1 == fwrite(buffer, pageSize, 1, fileInfo->file));
}

static bool FILE_ERASE(uint32_t startPage, uint32_t endPage, uint32_t pageSize, void *file) {
    return true;
}

static bool MOCK_FILE_ERASE(uint32_t startPage, uint32_t endPage, uint32_t pageSize, void *file) {
  /* Seek to position in file */
  FILE_INFO *fileInfo = (FILE_INFO *)file;
  int seekResult = fseek(fileInfo->file, startPage * pageSize, SEEK_SET);
  
  if (seekResult != 0) {
#ifdef PRINT_ERRORS
    printf("Error seeking in file.\n");
#endif
    return seekResult;
  }
  
  /* Setup data to insert */
  size_t memorySize = (endPage - startPage) * pageSize;
  char *buffer = malloc(memorySize);
  memset(buffer, 1, memorySize);
  
  /* Erase data */
  size_t writeResult = fwrite(buffer, memorySize, 1, fileInfo->file);
  free(buffer);
  return (1 == writeResult);
}

static bool FILE_CLOSE(void *file) {
  FILE_INFO *fileInfo = (FILE_INFO *)file;
  fclose(fileInfo->file);
  fileInfo->file = NULL;
  return true;
}

static bool FILE_FLUSH(void *file) {
  FILE_INFO *fileInfo = (FILE_INFO *)file;
  return (fflush(fileInfo->file) == 0);
}

static bool FILE_OPEN(void *file, uint8_t mode) {
  FILE_INFO *fileInfo = (FILE_INFO *)file;
  
  if (mode == EMBEDDB_FILE_MODE_W_PLUS_B) {
    fileInfo->file = fopen(fileInfo->filename, "w+b");
  } else if (mode == EMBEDDB_FILE_MODE_R_PLUS_B) {
    fileInfo->file = fopen(fileInfo->filename, "r+b");
  } else {
    return false;
  }
  
  if (fileInfo->file == NULL) {
    return false;
  }
  
  return true;
}

embedDBFileInterface *getFileInterface(void) {
    embedDBFileInterface *fileInterface = malloc(sizeof(embedDBFileInterface));
    fileInterface->close = FILE_CLOSE;
    fileInterface->read  = FILE_READ;
    fileInterface->write = FILE_WRITE;
    fileInterface->erase = FILE_ERASE;
    fileInterface->open  = FILE_OPEN;
    fileInterface->flush = FILE_FLUSH;
    return fileInterface;
}

embedDBFileInterface *getMockEraseFileInterface(void) {
    embedDBFileInterface *fileInterface = malloc(sizeof(embedDBFileInterface));
    fileInterface->close = FILE_CLOSE;
    fileInterface->read  = FILE_READ;
    fileInterface->write = FILE_WRITE;
    fileInterface->erase = MOCK_FILE_ERASE;
    fileInterface->open  = FILE_OPEN;
    fileInterface->flush = FILE_FLUSH;
    return fileInterface;
}
