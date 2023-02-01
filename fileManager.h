


#ifndef FILEMANAGER_HEADER
#define FILEMANAGER_HEADER

#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>

typedef struct file_Metadata
{
    char* filename;
    time_t lastModified;
}fileData;














#endif