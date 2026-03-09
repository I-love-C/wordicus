#ifndef _WORDICUS_H_
#define _WORDICUS_H_

#include <stddef.h>

typedef struct wordicus_file_t wordicus_file_t;

wordicus_file_t *wordicus_file_new(wordicus_file_t * wordicus_file, char *filename);
void wordicus_file_free(wordicus_file_t * wf);
const char *wordicus_file_get_name(const wordicus_file_t *wf);
size_t wordicus_file_get_lines(const wordicus_file_t *wf);

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
    #include <immintrin.h>
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
    #define CHUNK_SIZE 1
    #define VEC_TYPE char
    #define SET_NEWLINE() '\n'
    #define REGISTER_TYPE printf("Normal\n");
    #define PROCESS_CHUNK(ptr, newline_vec, count)                          \
        do {                                                                \
            if (*(ptr) == (newline_vec)) (count)++;                         \
        } while(0)

#endif

#endif // _WORDICUS_H_
