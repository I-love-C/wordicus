    #include <assert.h>
    #include <string.h>
    #include <stdio.h>
    #include <stdint.h>
    #include <stddef.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>

    #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
        #include <immintrin.h>
    #endif

    #if UINTPTR_MAX == 0xffffffff
        #warning "Compiling on a 32-bit system. File sizes over 4GB not supported."
    #endif

    #if defined(__AVX2__)
        #define REGISTER_TYPE printf("__AVX2__\n");
        #define CHUNK_SIZE 32
        #define VEC_TYPE __m256i
        #define SET_NEWLINE() _mm256_set1_epi8('\n')
        #define PROCESS_CHUNK(ptr, newline_vec, count)                          \
            do {                                                                \
                VEC_TYPE chunk; memcpy(&chunk, (ptr), CHUNK_SIZE);              \
                VEC_TYPE cmp_result = _mm256_cmpeq_epi8(chunk, (newline_vec));  \
                uint32_t mask = _mm256_movemask_epi8(cmp_result);               \
                (count) += __builtin_popcount(mask);                            \
            } while(0)

    #elif defined(__SSE2__)
        #define REGISTER_TYPE printf("__SSE2__\n");
        #define CHUNK_SIZE 16
        #define VEC_TYPE __m128i
        #define SET_NEWLINE() _mm_set1_epi8('\n')
        #define PROCESS_CHUNK(ptr, newline_vec, count)                          \
            do {                                                                \
                VEC_TYPE chunk; memcpy(&chunk, (ptr), CHUNK_SIZE);              \
                VEC_TYPE cmp_result = _mm_cmpeq_epi8(chunk, (newline_vec));     \
                uint32_t mask = _mm_movemask_epi8(cmp_result);                  \
                (count) += __builtin_popcount(mask);                            \
            } while(0)

    #else
    #endif


    int main(int argc, char * argv[]) {
        REGISTER_TYPE;
        if (argc != 2) return -1;

        int data_fd = open(argv[1], O_RDONLY);
        assert(data_fd != -1);

        struct stat data_stats;
        assert(fstat(data_fd, &data_stats) != -1);

        assert(data_stats.st_size > 0);
        assert((uintmax_t)data_stats.st_size <= (uintmax_t)SIZE_MAX);

        size_t file_size = (size_t)data_stats.st_size;
        char *file_content = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, data_fd, 0);
        assert(file_content != MAP_FAILED);
        assert(close(data_fd) != -1);
        madvise(file_content, file_size, MADV_SEQUENTIAL);

        size_t line_count = 0, position = 0;
        VEC_TYPE newline = SET_NEWLINE();

        for (; position + CHUNK_SIZE <= file_size; position += CHUNK_SIZE)
                PROCESS_CHUNK(file_content + position, newline, line_count);

        for (; position < file_size; position++)
            if (file_content[position] == '\n') line_count++;

        printf("line count : %zu\n", line_count);
        assert(munmap(file_content, file_size) != -1);
        return 0;
    }
