    #include <assert.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdio.h>
    #include <stdint.h>
    #include <stddef.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>

    #include "wordicus.h"
    #include "wordicus_error.h"

    struct wordicus_file_t {
        char *name;
        char * file_content;
        size_t file_size;
    };


    #define LOCAL_WORDICUS_FD_ERROR(error_type) \
        do {                                    \
            error_status = (error_type);        \
            goto file_descriptor_error;         \
        } while(0)

    wordicus_file_t *wordicus_file_new(const char *filename) {
        if(filename == NULL) return NULL;
        int data_fd = open(filename, O_RDONLY);
        if(data_fd == -1) THROW_WORDICUS_ERROR(FILE_NOT_FOUND);

        struct stat data_stats;
        if (fstat(data_fd, &data_stats) == -1) LOCAL_WORDICUS_FD_ERROR(FILE_ERROR);
        if (data_stats.st_size == 0) LOCAL_WORDICUS_FD_ERROR(FILE_EMPTY);

        size_t file_size = (size_t)data_stats.st_size;
        char *file_content = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, data_fd, 0);
        if (file_content == MAP_FAILED) LOCAL_WORDICUS_FD_ERROR(MMAP_FAILED);
        close(data_fd);
        madvise(file_content, file_size, MADV_SEQUENTIAL);

        wordicus_file_t * wordicus_file = malloc(sizeof(*wordicus_file));

        if (wordicus_file == NULL) {
            munmap(file_content, file_size);
            THROW_WORDICUS_ERROR(MALLOC_FAILED);
        };
        wordicus_file->file_content = file_content;
        wordicus_file->file_size = file_size;
        wordicus_file->name = strdup(filename);

        if (wordicus_file->name == NULL) {
            munmap(file_content, file_size);
            free(wordicus_file);
            THROW_WORDICUS_ERROR(MALLOC_FAILED);
        }

        return wordicus_file;

        file_descriptor_error:
            close(data_fd);
        error_out:
            fprintf(stderr, "[Fatal]: %s\n", Wordicus_errors[error_status]);
            return NULL;
    }

    #undef LOCAL_WORDICUS_FD_ERROR

    void wordicus_file_free(wordicus_file_t * wf) {
        if (wf == NULL) return;
        munmap(wf->file_content, wf->file_size);
        free(wf->name);
        free(wf);
    }

    const char *wordicus_file_get_name(const wordicus_file_t *wf) {
        if (wf == NULL) return NULL;
        return wf->name;
    }

    size_t wordicus_file_get_lines(const wordicus_file_t *wf) {
        if (wf == NULL) return (size_t) - 1;

        size_t line_count = 0, position = 0;
        VEC_TYPE newline = SET_NEWLINE();

        for (; position + CHUNK_SIZE <= wf->file_size; position += CHUNK_SIZE)
                PROCESS_CHUNK(wf->file_content + position, newline, line_count);

        for (; position < wf->file_size; position++)
            if (wf->file_content[position] == '\n') line_count++;

        return line_count;
    }


    int main(int argc, char * argv[]) {
        REGISTER_TYPE;

        if (argc != 2) return -1;

        wordicus_file_t * file = wordicus_file_new(argv[1]);
        if (file == NULL) return -1;

        size_t line_count = wordicus_file_get_lines(file);
        if (line_count  == ((size_t) - 1)) {
            wordicus_file_free(file);
            return -1;
        };
        printf("line count : %zu\n", line_count);

        wordicus_file_free(file);
        return 0;
    }
