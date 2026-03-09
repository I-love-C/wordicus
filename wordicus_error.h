#ifndef _WORDICUS_ERROR_H_
#define _WORDICUS_ERROR_H_

typedef enum {
    WORDICUS_OK = 0,
    FILE_NOT_FOUND,
    FILE_ERROR,
    FILE_EMPTY,
    MMAP_FAILED,
    MALLOC_FAILED,
    UNKNOWN_ERROR
} Wordicus_error ;

static const char * Wordicus_errors[] = {
    [FILE_NOT_FOUND]     = "Given file not found",
    [FILE_ERROR]         = "Failed to read file attributes",
    [FILE_EMPTY]         = "File is empty",
    [MMAP_FAILED]        = "Failed to map file to memory",
    [MALLOC_FAILED]      = "Malloc for wordicus struct failed",
    [UNKNOWN_ERROR]      = "Unknown error occurred"
};

static int error_status = WORDICUS_OK;

#define THROW_WORDICUS_ERROR(error_type) \
    do {                                 \
        error_status = (error_type);     \
        goto error_out;                  \
    } while(0)


#endif // _WORDICUS_ERROR_H_
