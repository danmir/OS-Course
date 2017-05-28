#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

struct point {
    unsigned is_live: 1;
};

/* Ширина игрового поля */
#define __WORLD_HEIGHT__ 5

/* Высота игрового поля */
#define __WORLD_WIDTH__ 5

#define __GAME_INIT_CONF__ "game.conf"
#define __GAME_TMP_CONF__ "game_tmp.conf"

struct point world[__WORLD_WIDTH__][__WORLD_HEIGHT__];
struct point prev_world[__WORLD_WIDTH__][__WORLD_HEIGHT__];

void error(char *message) {
    fprintf(stderr, "%s.\n", message);
    exit(1);
}

void rand_init_world(struct point world[][__WORLD_HEIGHT__]) {
    unsigned int i, j;
    for (i = 0; i < __WORLD_WIDTH__; i++) {
        for (j = 0; j < __WORLD_HEIGHT__; j++) {
            unsigned int num = rand() < RAND_MAX / 10 ? 1 : 0;
            if (num == 1) {
                world[i][j].is_live = 1;
            } else {
                world[i][j].is_live = 0;
            }
        }
    }
}

void file_init_world(struct point world[][__WORLD_HEIGHT__]) {

    FILE *fp = fopen(__GAME_INIT_CONF__, "r");
    if (fp == NULL) {
        error("Не удалось открыть файл");
    };

    unsigned int i, j;
    int c;
    for (i = 0; i < __WORLD_WIDTH__; i++) {
        for (j = 0; j < __WORLD_HEIGHT__; j++) {
            c = fgetc(fp);
            switch(c) {
                case EOF:
                    error("Неожиданный конец файла");
                    break;
                case '\n':
                    error("Неожиданный перевод строки");
                    break;
                case '1':
                    world[i][j].is_live = 1;
                    break;
                case '0':
                    world[i][j].is_live = 0;
                    break;
                default:
                    error("Конфиг должен быть из (0|1) без пробелов");
                    break;
            }
        }
        fgetc(fp);
    }
    fclose(fp);
}

void print_world(struct point world[][__WORLD_HEIGHT__]) {
    unsigned int i, j;
    for (i = 0; i < __WORLD_WIDTH__; i++) {
        for (j = 0; j < __WORLD_HEIGHT__; j++) {
            printf("%d", world[i][j].is_live);
        }
        printf("\n");
    }
}

void read_point_neighbors(signed int nb[][2], unsigned int x, unsigned int y) {
    unsigned int i, j;
    unsigned int k = 0;

    for (i = x - 1; i <= x + 1; i++) {
        for (j = y - 1; j <= y + 1; j++) {
            if (i == x && j == y) {
                continue;
            }
            nb[k][0] = i;
            nb[k][1] = j;
            k++;
        }
    }
}

unsigned int count_live_neighbors(struct point world[][__WORLD_HEIGHT__], unsigned int x, unsigned int y) {
    unsigned int count = 0;
    unsigned int i;
    signed int nb[8][2];
    signed int _x, _y;

    read_point_neighbors(nb, x, y);

    for (i = 0; i < 8; i++) {
        _x = nb[i][0];
        _y = nb[i][1];

        if (_x < 0 || _y < 0) {
            continue;
        }
        if (_x >= __WORLD_WIDTH__ || _y >= __WORLD_HEIGHT__) {
            continue;
        }

        if (world[_x][_y].is_live == 1) {
            count++;
        }
    }

    return count;
}

void copy_world(struct point src[][__WORLD_HEIGHT__], struct point dest[][__WORLD_HEIGHT__]) {
    unsigned int i, j;
    for (i = 0; i < __WORLD_WIDTH__; i++) {
        for (j = 0; j < __WORLD_HEIGHT__; j++) {
            dest[i][j] = src[i][j];
        }
    }
}

void next_generation(struct point world[][__WORLD_HEIGHT__]) {
    unsigned int i, j;
    unsigned int live_nb;
    struct point tmp_world[__WORLD_WIDTH__][__WORLD_HEIGHT__];
    struct point p;

    time_t start_time = time(NULL);

    for (i = 0; i < __WORLD_WIDTH__; i++) {
        for (j = 0; j < __WORLD_HEIGHT__; j++) {
            p = world[i][j];
            live_nb = count_live_neighbors(world, i, j);

            if (p.is_live == 0) {
                if (live_nb == 3) {
                    tmp_world[i][j].is_live = 1;
                }
            } else {
                if (live_nb < 2 || live_nb > 3) {
                    tmp_world[i][j].is_live = 0;
                }
            }
        }
        time_t end_time = time(NULL);
        double calc_time = difftime(end_time, start_time);
        //fprintf(stdout, "Пересчитали за %f \n", calc_time);
        if (calc_time >= 1.0) {
            fprintf(stderr, "Не успели пересчитать за одну секунду.\n");
            return;
        };
    }

    copy_world(tmp_world, world);
}

void timer_handler(int sig) {
    alarm(1);
    FILE *fp = fopen(__GAME_TMP_CONF__, "w");
    if (fp == NULL) {
        error("Не удалось открыть временный файл");
    };

    unsigned int i, j;
    for (i = 0; i < __WORLD_WIDTH__; i++) {
        for (j = 0; j < __WORLD_HEIGHT__; j++) {
            if (fputc(world[i][j].is_live + 48, fp) == EOF) {
                error("Не удалось записать в временный файл");
            }
        }
        if (fputc('\n', fp) == EOF) {
            error("Не удалось записать в временный файл");
        }
    }
    fclose(fp);
    next_generation(world);
}

void game() {
    file_init_world(world);
    signal(SIGALRM, timer_handler);
    alarm(1);
    wait(NULL);
}

void run_server() {
    struct sockaddr_in serv_addr, cli_addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        error("Не удалось создать сокет");
    };

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(7891);

    if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Can't bind");
    };

    listen(sock, 1);
    socklen_t addr_size = sizeof(cli_addr);

    printf("Listening...\n");

    while(true) {
        int newsock = accept(sock, (struct sockaddr *)&cli_addr, &addr_size);
        if (newsock < 0) {
            error("Не удалось принять соединение");
        }
        FILE *fp = fopen(__GAME_TMP_CONF__, "r");
        if (fp != NULL) {
            int symbol;
            while((symbol = getc(fp)) != EOF) {
                int n = write(newsock, &symbol, 1);
                if (n < 0) {
                    error("Не удалось записать в сокет");
                };
            }
            fclose(fp);
        }
        close(newsock);
    }
};


int main() {
//    FILE *fp = fopen(__GAME_TMP_CONF__, "a");
//    fclose(fp);

//    pid_t pid;
    switch(fork()) {
        case -1:
            error("Can't fork");
            break;
        case 0:
            run_server();
            break;
        default:
            game();
    }

//    file_init_world(world);
//    print_world(world);
    return 0;
}