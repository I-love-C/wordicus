#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <immintrin.h>

int main(int argc, char * argv[]) {
    if (argc != 2) return -1;

    int data_fd = open(argv[1], O_RDONLY);
    assert(data_fd != -1);

    struct stat data_stats;
    assert(fstat(data_fd, &data_stats) != -1);

    size_t file_size = (size_t)data_stats.st_size;
    assert(file_size > 0);

    char *file_content = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, data_fd, 0);
    assert(file_content != MAP_FAILED);
    assert(close(data_fd) != -1);
    madvise(file_content, file_size, MADV_SEQUENTIAL);

    size_t line_count = 0, position = 0;
    __m256i newline  = _mm256_set1_epi8 ('\n');

    for (; position + 32 <= file_size; position+= 32) {
        __m256i chunk; memcpy(&chunk, file_content + position, 32);
        __m256i cmp_result = _mm256_cmpeq_epi8(chunk, newline);
        uint32_t mask = _mm256_movemask_epi8(cmp_result);
        line_count += __builtin_popcountl(mask);
    }
    for (; position < file_size; position++)
        if (file_content[position] == '\n') line_count++;

    printf("line count : %zu\n", line_count);
    assert(munmap(file_content, file_size) != -1);
    return 0;
}
