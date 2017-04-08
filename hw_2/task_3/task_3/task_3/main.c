#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

long long int *res_array;
int res_array_pos = 0;
int res_array_len;

int expand_long_array(long long int *array, int *size) {
    long long int *tmp_ptr = realloc(array, *size + 100);
    if (tmp_ptr == NULL) {
        perror("Can't allocate memory to expand int array");
        return -1;
    } else {
        array = tmp_ptr;
        *size += 100;
    }
    return 0;
}

int expand_char_array(char *array, int *size) {
    char *tmp_ptr = realloc(array, *size + 100);
    if (tmp_ptr == NULL) {
        perror("Can't allocate memory to expand char array");
        return -1;
    } else {
        array = tmp_ptr;
        *size += 100;
    }
    return 0;
}

int add_num_to_res(long long int num) {
    // Write result
    res_array[res_array_pos] = num;
    // Check for bound
    if (res_array_pos + 1 >= res_array_len) {
        if (expand_long_array(res_array, &res_array_len) == -1) {
            return -1;
        }
    }
    res_array_pos++;
    return 0;
}

int read_numbers_from_file(const char *filename) {
    int fd = open(filename, O_RDONLY, 0666);
    if (fd < 0) {
        perror("Can't open file: ");
        return -1;
    }
    
    int current_num_len = 100;
    char *current_num = malloc(current_num_len * sizeof *current_num);
    int i = 0;
    
    // Read by one byte
    char ch;
    while(read(fd, &ch, 1) > 0) {
        if (ch >= '0' && ch <= '9') {
            current_num[i] = ch;
            // Check for bound
            if (i + 1 >= current_num_len) {
                if (expand_char_array(current_num, &current_num_len) == -1) {
                    return -1;
                }
            }
            i++;
        } else if (i != 0)  {
            // Try to transform current char array to digit
            current_num[i] = '\0';
            long res_int;
            // To check for overflow use strtol
            char *ptr; // for rest of the string
            errno = 0; // to catch overflow
            res_int = strtol(current_num, &ptr, 10);
            if (ptr == current_num) {
                perror("No digits found");
                return -1;
            }
            else if ((res_int == LONG_MIN || res_int == LONG_MAX) && errno == ERANGE) {
                perror("Value out of range");
                return -1;
            }
            
            // Add to result array
            if (add_num_to_res(res_int) == -1) {
                return -1;
            }
            // Reset 'pointer'
            i = 0;
        }
    }
    
    free(current_num);
    if (close(fd) < 0) {
        perror("Can't close file: ");
        return -1;
    }
    return 0;
}

int compare (const void *a, const void *b) {
    if (*(long long int*)a <  *(long long int*)b) return -1;
    if (*(long long int*)a == *(long long int*)b) return 0;
    if (*(long long int*)a >  *(long long int*)b) return 1;
    
    return 0;
}

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s in_1.file [in_2.file] [...] out.file \n", argv[0]);
        return -1;
    }

    int total_files_count = argc - 2;
    res_array_len = 10;
    res_array = malloc(res_array_len * sizeof *res_array);
    
    for (int file_id = 1; file_id < argc - 1; ++file_id) {
        if (read_numbers_from_file(argv[file_id]) == -1) {
            printf("Can't read data from current file :(");
            free(res_array);
            return -1;
        }
    }
    
    // Sorting
    // "If comp does not throw exceptions, this function (qsort) throws no exceptions (no-throw guarantee)."
    qsort(res_array, res_array_pos, sizeof(*res_array), compare);
    
    // Printing results
    FILE *f = fopen(argv[argc - 1], "w");
    if (f == NULL) {
        perror("Can't open file to write results: ");
    } else {
        for (int i = 0; i < res_array_pos; ++i) {
            if (fprintf(f, "%lld\n", res_array[i]) < 0) {
                perror("Can't write result to file");
            }
        }
        if (fclose(f) < 0) {
            perror("Can't close file with results: ");
        }
    }
    
    free(res_array);
    return 0;
}
