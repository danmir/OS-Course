#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

char* lck_suff = ".lck";
char* tmp_pref = "/tmp/";

//typedef int (*pfn)(int param1, void * param2);

void error(char *message) {
    fprintf(stderr, "%s.\n", message);
    exit(1);
}

char* get_lock_file_path(char* file_to_edit_path) {
    // /dir1/dir2/file_to_edit -> /tmp/file_to_edit.`lck_suff`
    // /dir1/dir2/file_to_edit.txt -> /tmp/file_to_edit.txt.`lck_suff`
    // file_to_edit -> /tmp/file_to_edit.`lck_suff`

    size_t file_to_edit_path_len = strlen(file_to_edit_path);
    char *file_to_edit_with_extension = malloc(file_to_edit_path_len + 1);

    for (int i = (int) (file_to_edit_path_len - 1), j = 0; i >= 0; --i, ++j) {
        if (file_to_edit_path[i] == '/') {
            file_to_edit_with_extension[j] = '\0';
            break;
        }
        file_to_edit_with_extension[j] = file_to_edit_path[i];
    }

    // Reverse string
    for (int i = 0, j = (int) strlen(file_to_edit_with_extension) - 1; i < j; ++i, --j) {
        char tmp;
        tmp = file_to_edit_with_extension[i];
        file_to_edit_with_extension[i] = file_to_edit_with_extension[j];
        file_to_edit_with_extension[j] = tmp;
    }

    strcat(file_to_edit_with_extension, lck_suff);

    char* final_file_to_edit_with_extension = malloc(strlen(file_to_edit_with_extension) + strlen(tmp_pref));
    for (int k = 0; k < strlen(tmp_pref); ++k) {
        final_file_to_edit_with_extension[k] = tmp_pref[k];
    }
    strcat(final_file_to_edit_with_extension, file_to_edit_with_extension);

    return final_file_to_edit_with_extension;
}

bool is_file_exists(char* path) {
    return access(path, F_OK) == 0 ? true : false;
}

void create_lock_file_by_path(char* path) {
    FILE *fp = fopen(path, "wa");
    if (fp < 0) {
        perror("Can't open lock file");
        exit(1);
    }
    fprintf(fp, "W%d\n", getpid());
    fclose(fp);
}

void remove_lock_file_by_path(char* path) {
    if (remove(path) < 0) {
        perror("Can't remove lock file");
        exit(1);
    }
}

void append_to_file(char* file_to_edit_path, int argc, char *argv[]) {
    sleep(10); // For test
    FILE *fp = fopen(file_to_edit_path, "a");
    fprintf(fp, "\n");
    for (int i = 1; i < argc - 2; i++) {
        fprintf(fp, "%s ", argv[i]);
    }
    fprintf(fp, "%s", argv[argc - 2]);
    fclose(fp);
}

void edit_file_with_lock(char* file_to_edit_path, int data_size, char *data[]) {
    char* lock_file = get_lock_file_path(file_to_edit_path);
    while (is_file_exists(lock_file)) {}

    create_lock_file_by_path(lock_file);

    append_to_file(file_to_edit_path, data_size, data);

    remove_lock_file_by_path(lock_file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./locker text_1 [text_2 ...] file_to_edit");
    };

    char* file_to_edit = argv[argc - 1];
    edit_file_with_lock(file_to_edit, argc, argv);

    return 0;
}