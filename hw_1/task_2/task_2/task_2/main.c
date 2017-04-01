#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

int make_sparse(int fdin, int fdout) {
    static char const skipbyte = '\0';
    
    // Read by one byte
    char ch;
    while(read(fdin, &ch, 1) > 0) {
        if (ch == skipbyte) {
            // If whence is SEEK_CUR, the offset is set to its current location plus offset bytes.
            if (lseek(fdout, 1, SEEK_CUR) == -1) {
                perror("Can't make 'hole' in output file");
                return -1;
            }
        } else {
            if (write(fdout, &ch, 1) != 1) {
                perror("Can't write to output file");
                return -1;
            }
        }
    }
    
    // Don't forget to write the last byte
    if (ch == skipbyte) {
        if (lseek(fdout, -1, SEEK_CUR) == -1) {
            perror("Can't return to last byte position");
            return -1;
        }
        if (write(fdout, &ch, 1) != 1) {
            perror("Can't write to output file");
            return -1;
        }
    }
    
    return 0;
}

int main(int argc, const char * argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: gzip -cd sparse-file.gz | %s newsparsefile\n", argv[0]);
        return -1;
    }
    
    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Can't open file: ");
        return -1;
    }
    
    if (make_sparse(STDIN_FILENO, fd) == -1) {
        perror(argv[0]);
        return 1;
    }
    
    int res = close(fd);
    if (res < 0) {
        perror("Can't close file: ");
        return -1;
    }
    return 0;
}
