    #define _GNU_SOURCE
    #include <time.h>
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


    #define THROW_LOCAL_WORDICUS_FD_ERROR(error_type) \
        do {                                    \
            error_status = (error_type);        \
            goto file_descriptor_error;         \
        } while(0)

    wordicus_file_t *wordicus_file_new(wordicus_file_t * wordicus_file, char *filename) {
        if(filename == NULL) return NULL;
        int data_fd = open(filename, O_RDONLY);
        if(data_fd == -1) THROW_WORDICUS_ERROR(FILE_NOT_FOUND);

        struct statx data_stats;
        if (statx(data_fd, "", AT_EMPTY_PATH, STATX_SIZE, &data_stats) == -1)
            THROW_LOCAL_WORDICUS_FD_ERROR(FILE_ERROR);

        if (data_stats.stx_size == 0) THROW_LOCAL_WORDICUS_FD_ERROR(FILE_EMPTY);

        size_t file_size = (size_t)data_stats.stx_size;
        char *file_content = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, data_fd, 0);
        if (file_content == MAP_FAILED) THROW_LOCAL_WORDICUS_FD_ERROR(MMAP_FAILED);
        close(data_fd); // close in both cases

        wordicus_file->file_content = file_content;
        wordicus_file->file_size = file_size;
        wordicus_file->name = strdup(filename);

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

    size_t wordicus_file_get_lines_v2(const wordicus_file_t *wf) {
        if (wf == NULL) return (size_t) - 1;

        size_t line_count = 0, position = 0;
        __m256i newline = _mm256_set1_epi8('\n');

        __m256i ch1,ch2,ch3,ch4;
        __m256i cmp1,cmp2,cmp3,cmp4;
        uint32_t mask1,mask2,mask3,mask4;
        size_t count1,count2,count3,count4;

        while (position + 128 <= wf->file_size) {
            const char * base_position = wf->file_content + position;

            ch1 = _mm256_loadu_si256((__m256i_u * )base_position);
            ch2 = _mm256_loadu_si256((__m256i_u * )(base_position + 32));
            ch3 = _mm256_loadu_si256((__m256i_u * )(base_position + 64));
            ch4 = _mm256_loadu_si256((__m256i_u * )(base_position + 96));

            cmp1 = _mm256_cmpeq_epi8(ch1, newline);
            cmp2 = _mm256_cmpeq_epi8(ch2, newline);
            cmp3 = _mm256_cmpeq_epi8(ch3, newline);
            cmp4 = _mm256_cmpeq_epi8(ch4, newline);

            mask1 = _mm256_movemask_epi8(cmp1);
            mask2 = _mm256_movemask_epi8(cmp2);
            mask3 = _mm256_movemask_epi8(cmp3);
            mask4 = _mm256_movemask_epi8(cmp4);

            count1 = __builtin_popcount(mask1);
            count2 = __builtin_popcount(mask2);
            count3 = __builtin_popcount(mask3);
            count4 = __builtin_popcount(mask4);

            line_count += (count1 + count2 + count3 + count4);
            position += 128;
        }

        for (; position < wf->file_size; position++)
            if (wf->file_content[position] == '\n') line_count++;

        return line_count;
    }

    double begin_profiling_scope() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
    }

    double end_profiling_scope(double initial_profile_scope) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ((double)ts.tv_sec + (double)ts.tv_nsec / 1e9) - initial_profile_scope;
    }


    int main(int argc, char * argv[]) {
        REGISTER_TYPE;

        if (argc != 2) return -1;
        wordicus_file_t file_given;

        double start_time_file = begin_profiling_scope();
        wordicus_file_t * file = wordicus_file_new(&file_given, argv[1]);
        if (file == NULL) return -1;

        double elapsed_time_file = end_profiling_scope(start_time_file);
        printf("%f seconds for file\n", elapsed_time_file);

        double start_time_lines = begin_profiling_scope();
        size_t line_count = wordicus_file_get_lines_v2(file);
        double elapsed_time_lines = end_profiling_scope(start_time_lines);

        printf("%f seconds for computation\n", elapsed_time_lines);
        printf("line count : %zu\n", line_count);

        double total_elapsed_time  = end_profiling_scope(start_time_file);
        printf("Total is : %f \n", total_elapsed_time);

        wordicus_file_free(file);
        return 0;
    }
