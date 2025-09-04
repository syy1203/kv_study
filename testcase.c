// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <arpa/inet.h>
// #include <sys/time.h>
// #include <pthread.h>
// #include <unistd.h>

// #define MAX_MSG_LENGTH 1024
// #define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

// typedef struct {
//     char ip[64];
//     int port;
//     int mode;
//     int id;
// } thread_arg_t;

// int send_msg(int connfd, char *msg, int length) {
//     int res = send(connfd, msg, length, 0);
//     if (res < 0) {
//         perror("send");
//         exit(1);
//     }
//     return res;
// }

// int recv_msg(int connfd, char *msg, int length) {
//     int res = recv(connfd, msg, length, 0);
//     if (res < 0) {
//         perror("recv");
//         exit(1);
//     }
//     return res;
// }

// void testcase(int connfd, char *msg, char *pattern, char *casename) {
//     if (!msg || !pattern || !casename) return;

//     send_msg(connfd, msg, strlen(msg));

//     char result[MAX_MSG_LENGTH] = {0};
//     recv_msg(connfd, result, MAX_MSG_LENGTH);

//     if (strcmp(result, pattern) == 0) {
//         // printf("[Thread] PASS ->  %s\n", casename);
//     } else {
//         // printf("[Thread] FAILED -> %s, '%s' != '%s' \n", casename, result, pattern);
//         exit(1);
//     }
// }

// int connect_tcpserver(const char *ip, unsigned short port) {
//     int connfd = socket(AF_INET, SOCK_STREAM, 0);
//     struct sockaddr_in server_addr;
//     memset(&server_addr, 0, sizeof(struct sockaddr_in));

//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = inet_addr(ip);
//     server_addr.sin_port = htons(port);

//     if (0 != connect(connfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in))) {
//         perror("connect");
//         return -1;
//     }

//     return connfd;
// }

// void array_testcase_1w(int connfd) {
//     int count = 10000;

//     struct timeval tv_begin;
//     gettimeofday(&tv_begin, NULL);

//     for (int i = 0; i < count; i++) {
//         testcase(connfd, "SET Teacher King", "OK\r\n", "SET-Teacher");
//         testcase(connfd, "GET Teacher", "King\r\n", "GET-Teacher");
//         testcase(connfd, "MOD Teacher Darren", "OK\r\n", "MOD-Teacher");
//         testcase(connfd, "GET Teacher", "Darren\r\n", "GET-Teacher");
//         testcase(connfd, "EXIST Teacher", "EXIST\r\n", "GET-Teacher");
//         testcase(connfd, "DEL Teacher", "OK\r\n", "DEL-Teacher");
//         testcase(connfd, "GET Teacher", "NO EXIST\r\n", "GET-Teacher");
//         testcase(connfd, "MOD Teacher KING", "NO EXIST\r\n", "MOD-Teacher");
//         testcase(connfd, "EXIST Teacher", "NO EXIST\r\n", "GET-Teacher");
//     }

//     struct timeval tv_end;
//     gettimeofday(&tv_end, NULL);

//     int time_used = TIME_SUB_MS(tv_end, tv_begin);
//     printf("[array testcase] time_used: %d ms, qps: %d\n", time_used, 90000 * 1000 / time_used);
// }

// void rbtree_testcase_1w(int connfd) {
//     int count = 10000;

//     struct timeval tv_begin;
//     gettimeofday(&tv_begin, NULL);

//     for (int i = 0; i < count; i++) {
//         testcase(connfd, "RSET Teacher King", "OK\r\n", "RSET-Teacher");
//         testcase(connfd, "RGET Teacher", "King\r\n", "RGET-King-Teacher");
//         testcase(connfd, "RMOD Teacher Darren", "OK\r\n", "RMOD-D-Teacher");
//         testcase(connfd, "RGET Teacher", "Darren\r\n", "RGET-Darren-Teacher");
//         testcase(connfd, "REXIST Teacher", "EXIST\r\n", "REXIST-Teacher");
//         testcase(connfd, "RDEL Teacher", "OK\r\n", "RDEL-Teacher");
//         testcase(connfd, "RGET Teacher", "NO EXIST\r\n", "RGET-K-Teacher");
//         testcase(connfd, "RMOD Teacher KING", "NO EXIST\r\n", "RMOD-K-Teacher");
//         testcase(connfd, "REXIST Teacher", "NO EXIST\r\n", "REXIST-Teacher");
//     }

//     struct timeval tv_end;
//     gettimeofday(&tv_end, NULL);

//     int time_used = TIME_SUB_MS(tv_end, tv_begin);
//     printf("[rbtree testcase] time_used: %d ms, qps: %d\n", time_used, 90000 * 1000 / time_used);
// }

// void rbtree_testcase_3w(int connfd) {
//     int count = 10000;

//     struct timeval tv_begin;
//     gettimeofday(&tv_begin, NULL);

//     for (int i = 0; i < count; i++) {
//         char cmd[128] = {0};
//         snprintf(cmd, 128, "RSET Teacher%d King%d", i, i);
//         testcase(connfd, cmd, "OK\r\n", "RSET-Teacher");
//     }

//     for (int i = 0; i < count; i++) {
//         char cmd[128] = {0};
//         snprintf(cmd, 128, "RGET Teacher%d", i);
//         char result[128] = {0};
//         snprintf(result, 128, "King%d\r\n", i);
//         testcase(connfd, cmd, result, "RGET-King-Teacher");
//     }

//     for (int i = 0; i < count; i++) {
//         char cmd[128] = {0};
//         snprintf(cmd, 128, "RMOD Teacher%d King%d", i, i);
//         testcase(connfd, cmd, "OK\r\n", "RMOD-K-Teacher");
//     }

//     struct timeval tv_end;
//     gettimeofday(&tv_end, NULL);

//     int time_used = TIME_SUB_MS(tv_end, tv_begin);
//     printf("[rbtree testcase 3w] time_used: %d ms, qps: %d\n", time_used, 30000 * 1000 / time_used);
// }

// void hash_testcase(int connfd) {
//     testcase(connfd, "HSET Teacher King", "OK\r\n", "HSET-Teacher");
//     testcase(connfd, "HGET Teacher", "King\r\n", "HGET-King-Teacher");
//     testcase(connfd, "HMOD Teacher Darren", "OK\r\n", "HMOD-D-Teacher");
//     testcase(connfd, "HGET Teacher", "Darren\r\n", "HGET-Darren-Teacher");
//     testcase(connfd, "HEXIST Teacher", "EXIST\r\n", "HEXIST-Teacher");
//     testcase(connfd, "HDEL Teacher", "OK\r\n", "HDEL-Teacher");
//     testcase(connfd, "HGET Teacher", "NO EXIST\r\n", "HGET-K-Teacher");
//     testcase(connfd, "HMOD Teacher KING", "NO EXIST\r\n", "HMOD-K-Teacher");
//     testcase(connfd, "HEXIST Teacher", "NO EXIST\r\n", "HEXIST-Teacher");
// }

// void *thread_worker(void *arg) {
//     thread_arg_t *targ = (thread_arg_t *)arg;
//     int connfd = connect_tcpserver(targ->ip, targ->port);
//     if (connfd < 0) {
//         fprintf(stderr, "Thread %d: failed to connect\n", targ->id);
//         pthread_exit(NULL);
//     }

//     switch (targ->mode) {
//         case 0: rbtree_testcase_1w(connfd); break;
//         case 1: rbtree_testcase_3w(connfd); break;
//         case 2: array_testcase_1w(connfd); break;
//         case 3: hash_testcase(connfd); break;
//         default: printf("Invalid mode\n");
//     }

//     close(connfd);
//     pthread_exit(NULL);
// 	free(targ); 
// }

// int main(int argc, char *argv[]) {
//     if (argc != 5) {
//         printf("Usage: %s <ip> <port> <mode> <thread_num>\n", argv[0]);
//         return -1;
//     }

//     char *ip = argv[1];
//     int port = atoi(argv[2]);
//     int mode = atoi(argv[3]);
//     int thread_num = atoi(argv[4]);

//     pthread_t tids[thread_num];
//     thread_arg_t args[thread_num];

//     for (int i = 0; i < thread_num; ++i) {
// 		thread_arg_t *arg = malloc(sizeof(thread_arg_t));
// 		snprintf(arg->ip, sizeof(arg->ip), "%s", ip);
// 		arg->port = port;
// 		arg->mode = mode;
// 		arg->id = i;
	
// 		pthread_create(&tids[i], NULL, thread_worker, arg);
// 	}
	

//     for (int i = 0; i < thread_num; ++i) {
//         pthread_join(tids[i], NULL);
//     }

//     return 0;
// }
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_MSG_LENGTH 1024
#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

typedef struct {
    char ip[64];
    int port;
    int mode;
    int id;
} thread_arg_t;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

int send_msg(int connfd, char *msg, int length) {
    int res = send(connfd, msg, length, 0);
    if (res < 0) {
        perror("send");
        return -1;
    }
    return res;
}

int recv_msg(int connfd, char *msg, int length) {
    int total = 0;
    while (total < length) {
        int res = recv(connfd, msg + total, length - total, 0);
        if (res <= 0) {
            perror("recv");
            return -1;
        }
        total += res;
        if (strstr(msg, "\r\n")) break;  // 假设返回结尾是 \r\n
    }
    return total;
}

void testcase(int connfd, char *msg, char *pattern, char *casename, int thread_id) {
    if (!msg || !pattern || !casename) return;

    send_msg(connfd, msg, strlen(msg));

    char result[MAX_MSG_LENGTH] = {0};
    if (recv_msg(connfd, result, MAX_MSG_LENGTH) < 0) {
        pthread_exit((void *)1);
    }

    if (strcmp(result, pattern) == 0) {
        pthread_mutex_lock(&print_mutex);
        // printf("[Thread %d] PASS ->  %s\n", thread_id, casename);
        pthread_mutex_unlock(&print_mutex);
    } else {
        pthread_mutex_lock(&print_mutex);
        // printf("[Thread %d] FAILED -> %s, '%s' != '%s'\n", thread_id, casename, result, pattern);
        pthread_mutex_unlock(&print_mutex);
        pthread_exit((void *)1);
    }
}

int connect_tcpserver(const char *ip, unsigned short port) {
    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (connect(connfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        perror("connect");
        return -1;
    }

    return connfd;
}

void array_testcase_1w(int connfd, int thread_id) {
    int count = 10000;
    struct timeval tv_begin, tv_end;
    gettimeofday(&tv_begin, NULL);

    for (int i = 0; i < count; i++) {
        testcase(connfd, "SET Teacher King", "OK\r\n", "SET-Teacher", thread_id);
        testcase(connfd, "GET Teacher", "King\r\n", "GET-Teacher", thread_id);
        testcase(connfd, "MOD Teacher Darren", "OK\r\n", "MOD-Teacher", thread_id);
        testcase(connfd, "GET Teacher", "Darren\r\n", "GET-Teacher", thread_id);
        testcase(connfd, "EXIST Teacher", "EXIST\r\n", "EXIST-Teacher", thread_id);
        // testcase(connfd, "DEL Teacher", "OK\r\n", "DEL-Teacher", thread_id);
        // testcase(connfd, "GET Teacher", "NO EXIST\r\n", "GET-Teacher", thread_id);
        // testcase(connfd, "MOD Teacher KING", "NO EXIST\r\n", "MOD-Teacher", thread_id);
        // testcase(connfd, "EXIST Teacher", "NO EXIST\r\n", "EXIST-Teacher", thread_id);
    }

    gettimeofday(&tv_end, NULL);
    int time_used = TIME_SUB_MS(tv_end, tv_begin);

    pthread_mutex_lock(&print_mutex);
    printf("[Thread %d] [array testcase] time_used: %d ms, qps: %d\n", thread_id, time_used, 90000 * 1000 / time_used);
    pthread_mutex_unlock(&print_mutex);
}

void rbtree_testcase_1w(int connfd, int thread_id) {
    int count = 10000;
    struct timeval tv_begin, tv_end;
    gettimeofday(&tv_begin, NULL);

    for (int i = 0; i < count; i++) {
        testcase(connfd, "RSET Teacher King", "OK\r\n", "RSET-Teacher", thread_id);
        testcase(connfd, "RGET Teacher", "King\r\n", "RGET-Teacher", thread_id);
        testcase(connfd, "RMOD Teacher Darren", "OK\r\n", "RMOD-Teacher", thread_id);
        testcase(connfd, "RGET Teacher", "Darren\r\n", "RGET-Teacher", thread_id);
        testcase(connfd, "REXIST Teacher", "EXIST\r\n", "REXIST-Teacher", thread_id);
        testcase(connfd, "RDEL Teacher", "OK\r\n", "RDEL-Teacher", thread_id);
        testcase(connfd, "RGET Teacher", "NO EXIST\r\n", "RGET-Teacher", thread_id);
        testcase(connfd, "RMOD Teacher KING", "NO EXIST\r\n", "RMOD-Teacher", thread_id);
        testcase(connfd, "REXIST Teacher", "NO EXIST\r\n", "REXIST-Teacher", thread_id);
    }

    gettimeofday(&tv_end, NULL);
    int time_used = TIME_SUB_MS(tv_end, tv_begin);

    pthread_mutex_lock(&print_mutex);
    printf("[Thread %d] [rbtree testcase] time_used: %d ms, qps: %d\n", thread_id, time_used, 90000 * 1000 / time_used);
    pthread_mutex_unlock(&print_mutex);
}

void rbtree_testcase_3w(int connfd, int thread_id) {
    int count = 50000;
    struct timeval tv_begin, tv_end;
    gettimeofday(&tv_begin, NULL);

    for (int i = 0; i < count; i++) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "RSET Teacher%d King%d", i, i);
        testcase(connfd, cmd, "OK\r\n", "RSET-Teacher", thread_id);
    }

    for (int i = 0; i < count; i++) {
        char cmd[128], expected[128];
        snprintf(cmd, sizeof(cmd), "RGET Teacher%d", i);
        snprintf(expected, sizeof(expected), "King%d\r\n", i);
        testcase(connfd, cmd, expected, "RGET-Teacher", thread_id);
    }

    // for (int i = 0; i < count; i++) {
    //     char cmd[128];
    //     snprintf(cmd, sizeof(cmd), "RMOD Teacher%d King%d", i, i);
    //     testcase(connfd, cmd, "OK\r\n", "RMOD-Teacher", thread_id);
    // }

    gettimeofday(&tv_end, NULL);
    int time_used = TIME_SUB_MS(tv_end, tv_begin);

    pthread_mutex_lock(&print_mutex);
    printf("[Thread %d] [rbtree testcase 3w] time_used: %d ms, qps: %d\n", thread_id, time_used, 100000 * 1000 / time_used);
    pthread_mutex_unlock(&print_mutex);
}
void hash_testcase_1w(int connfd, int thread_id) {
    int count = 10000;
    struct timeval tv_begin, tv_end;
    gettimeofday(&tv_begin, NULL);

    for (int i = 0; i < count; i++) {
            testcase(connfd, "HSET Teacher King", "OK\r\n", "HSET-Teacher", thread_id);
            testcase(connfd, "HGET Teacher", "King\r\n", "HGET-King-Teacher", thread_id);
            testcase(connfd, "HMOD Teacher Darren", "OK\r\n", "HMOD-D-Teacher", thread_id);
            testcase(connfd, "HGET Teacher", "Darren\r\n", "HGET-Darren-Teacher", thread_id);
            testcase(connfd, "HEXIST Teacher", "EXIST\r\n", "HEXIST-Teacher", thread_id);
            testcase(connfd, "HDEL Teacher", "OK\r\n", "HDEL-Teacher", thread_id);
            testcase(connfd, "HGET Teacher", "NO EXIST\r\n", "HGET-K-Teacher", thread_id);
            testcase(connfd, "HMOD Teacher KING", "NO EXIST\r\n", "HMOD-K-Teacher", thread_id);
            testcase(connfd, "HEXIST Teacher", "NO EXIST\r\n", "HEXIST-Teacher", thread_id);
    }

    gettimeofday(&tv_end, NULL);
    int time_used = TIME_SUB_MS(tv_end, tv_begin);

    pthread_mutex_lock(&print_mutex);
    printf("[Thread %d] [hash testcase] time_used: %d ms, qps: %d\n", thread_id, time_used,90000 * 1000 / time_used);
    pthread_mutex_unlock(&print_mutex);
}

void hash_testcase_3w(int connfd, int thread_id) {
    int count = 50000;  // 测试次数，您可以根据需要调整
    struct timeval tv_begin, tv_end;
    gettimeofday(&tv_begin, NULL);

    // 1. HSET：批量插入数据
    for (int i = 0; i < count; i++) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "HSET Teacher%d King%d", i, i);
        testcase(connfd, cmd, "OK\r\n", "HSET-Teacher", thread_id);
    }

    // 2. HGET：获取并验证数据
    for (int i = 0; i < count; i++) {
        char cmd[128], expected[128];
        snprintf(cmd, sizeof(cmd), "HGET Teacher%d", i);
        snprintf(expected, sizeof(expected), "King%d\r\n", i);
        testcase(connfd, cmd, expected, "HGET-King-Teacher", thread_id);
    }

    gettimeofday(&tv_end, NULL);
    int time_used = TIME_SUB_MS(tv_end, tv_begin);

    pthread_mutex_lock(&print_mutex);
    printf("[Thread %d] [hash testcase 3w] time_used: %d ms, qps: %d\n",
           thread_id, time_used, 100000 * 1000 / time_used);
    pthread_mutex_unlock(&print_mutex);
}


void *thread_worker(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    int connfd = connect_tcpserver(targ->ip, targ->port);
    if (connfd < 0) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Thread %d: failed to connect\n", targ->id);
        pthread_mutex_unlock(&print_mutex);
        free(targ);
        pthread_exit((void *)1);
    }

    switch (targ->mode) {
        case 0: rbtree_testcase_1w(connfd, targ->id); break;
        case 1: rbtree_testcase_3w(connfd, targ->id); break;
        case 2: array_testcase_1w(connfd, targ->id); break;
        case 3: hash_testcase_1w(connfd, targ->id); break;
        case 4:hash_testcase_3w(connfd, targ->id); break;
        default:
            pthread_mutex_lock(&print_mutex);
            printf("Thread %d: Invalid mode\n", targ->id);
            pthread_mutex_unlock(&print_mutex);
    }

    close(connfd);
    free(targ);
    pthread_exit((void *)0);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <ip> <port> <mode> <thread_num>\n", argv[0]);
        return -1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int mode = atoi(argv[3]);
    int thread_num = atoi(argv[4]);

    pthread_t tids[thread_num];
    for (int i = 0; i < thread_num; ++i) {
        thread_arg_t *arg = malloc(sizeof(thread_arg_t));
        snprintf(arg->ip, sizeof(arg->ip), "%s", ip);
        arg->port = port;
        arg->mode = mode;
        arg->id = i;
        pthread_create(&tids[i], NULL, thread_worker, arg);
    }

    int all_passed = 1;
    for (int i = 0; i < thread_num; ++i) {
        void *retval;
        pthread_join(tids[i], &retval);
        if ((long)retval != 0) {
            all_passed = 0;
        }
    }

    if (all_passed) {
        printf("✅ All test threads passed.\n");
    } else {
        printf("❌ Some test threads failed.\n");
    }

    return all_passed ? 0 : 1;
}
