#ifndef __KV_STORE_H__
#define __KV_STORE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "kv_mempool.h"

#define NETWORK_REACTOR 0
#define NETWORK_PROACTOR 1
#define NETWORK_NTYCO 2

#define NETWORK_SELECT NETWORK_NTYCO

#define KVS_MAX_TOKENS 128
#define ENABLE_ARRAY 1
#define ENABLE_RBTREE 1
#define ENABLE_HASH 1
#define ENABLE_SKIPTABLE 1
#define RDB_FILE "kvs.rdb"
#define AUTO_SAVE_INTERVAL 10 // 自动保存间隔时间，单位为秒

// RDB配置相关常量
#define RDB_CONFIG_FILE "kvs.conf"        // 默认配置文件路径
#define RDB_MAGIC 0x5244424B              // 魔数 "RDBK" (RDB Key-Value)
#define RDB_VERSION 1                     // RDB文件格式版本号
#define RDB_HEADER_SIZE 16                // 文件头大小（字节）
#define MAX_CONFIG_LINE 256               // 配置文件单行最大长度
#define MAX_PATH_LEN 512                  // 文件路径最大长度

// 数据对齐常量
#define RDB_ALIGNMENT 8                    // 8字节对齐
#define RDB_ALIGN_SIZE(size) (((size) + RDB_ALIGNMENT - 1) & ~(RDB_ALIGNMENT - 1))


#define POOL_CHUNK_SIZE 64
#define POOL_CHUNK_COUNT 100000

static mem_pool_t *global_pool = NULL;


// RDB配置结构体 - 用于存储RDB模块的所有配置参数
typedef struct {
    int save_interval;                    // 自动保存间隔（秒）
    char rdb_file[MAX_PATH_LEN];          // RDB 文件路径
    int rdb_enabled;                      // 是否启用 RDB (1=启用, 0=禁用)
    int rdb_compression;                  // 是否启用压缩 (1=启用, 0=禁用)
    int rdb_crc64;                        // 是否启用CRC64校验 (1=启用, 0=禁用)
    int rdb_backup;                       // 是否启用备份 (1=启用, 0=禁用)
    char backup_dir[MAX_PATH_LEN];        // 备份目录路径
} rdb_config_t;

// RDB文件头结构体 - 存储在RDB文件开头，包含文件元数据
typedef struct {
    uint32_t magic;                       // 魔数，用于验证文件格式 "RDBK"
    uint32_t version;                     // 文件格式版本号
    uint32_t flags;                       // 标志位 (压缩、校验和等)
    uint32_t data_size;                   // 数据部分的大小（字节）
    uint32_t record_count;                // 记录总数
    time_t save_time;                     // 文件保存时间戳
    uint32_t alignment_padding;           // 对齐填充（确保结构体8字节对齐）
} __attribute__((packed)) rdb_header_t;

// RDB文件尾结构体 - 存储在文件末尾，包含校验信息
typedef struct {
    uint8_t eof_marker[4];                // EOF标记 "EOF\0"
    uint64_t crc64;                       // CRC64校验和
    uint32_t footer_size;                 // 文件尾大小（用于验证）
    uint32_t alignment_padding;           // 对齐填充（确保结构体8字节对齐）
} __attribute__((packed)) rdb_footer_t;

// EOF标记常量
#define RDB_EOF_MARKER "EOF"
#define RDB_EOF_MARKER_SIZE 4
#define RDB_FOOTER_SIZE (4 + 8 + 4 + 4)  // eof_marker(4) + crc64(8) + footer_size(4) + alignment_padding(4)

static time_t last_save_time = 0; // 上次保存的时间

// 全局RDB配置
extern rdb_config_t g_rdb_config;

typedef int (*msg_handler)(char *msg, int length, char *response);

extern int reactor_start(unsigned short port, msg_handler handler);
extern int proactor_start(unsigned short port, msg_handler handler);
extern int ntyco_start(unsigned short port, msg_handler handler);

/* RDB持久化功能 */
int kvs_rdb_init(void);
int kvs_rdb_save(const char *filename);
int kvs_rdb_load(const char *filename);
void *check_auto_save(void *arg);

// RDB配置管理函数
int rdb_config_init(void);                                    // 初始化RDB配置（加载配置文件或使用默认值）
int rdb_config_load(const char *config_file);                 // 从配置文件加载RDB配置
int rdb_config_save(const char *config_file);                 // 保存RDB配置到配置文件
void rdb_config_print(void);                                  // 打印当前RDB配置信息
int rdb_config_set_save_interval(int interval);               // 设置自动保存间隔（秒）
int rdb_config_set_rdb_file(const char *file);                // 设置RDB文件路径
int rdb_config_set_enabled(int enabled);                      // 启用/禁用RDB功能
int rdb_config_set_compression(int enabled);                  // 启用/禁用压缩功能
int rdb_config_set_crc64(int enabled);                     // 启用/禁用CRC64校验功能
int rdb_config_set_backup(int enabled, const char *backup_dir); // 启用/禁用备份功能并设置备份目录

// RDB校验和工具函数
uint64_t rdb_calculate_crc64(const void *data, size_t size);     // 计算数据的CRC64校验和
int rdb_verify_file_integrity(const char *filename);             // 验证RDB文件的完整性（魔数、版本、校验和）

// RDB备份功能
int rdb_create_backup(const char *filename);                   // 创建RDB文件的备份
int rdb_restore_from_backup(const char *backup_file);          // 从备份文件恢复RDB数据

#if ENABLE_ARRAY

typedef struct kvs_array_item_s
{
    char *key;
    char *value;
} kvs_array_item_t;

#define KVS_ARRAY_SIZE 1024

typedef struct kvs_array_s
{
    kvs_array_item_t *table;
    int idx;
    int total;
    pthread_mutex_t array_mutex; // 互斥锁，用于保护数组的并发访问
} kvs_array_t;

int kvs_array_create(kvs_array_t *inst);
void kvs_array_destory(kvs_array_t *inst);

int kvs_array_set(kvs_array_t *inst, char *key, char *value);
char *kvs_array_get(kvs_array_t *inst, char *key);
int kvs_array_del(kvs_array_t *inst, char *key);
int kvs_array_mod(kvs_array_t *inst, char *key, char *value);
int kvs_array_exist(kvs_array_t *inst, char *key);

#endif

#if ENABLE_RBTREE

#define RED 1
#define BLACK 2

#define ENABLE_KEY_CHAR 1

#if ENABLE_KEY_CHAR
typedef char *KEY_TYPE;
#else
typedef int KEY_TYPE; // key
#endif

typedef struct _rbtree_node
{
    unsigned char color;
    struct _rbtree_node *right;
    struct _rbtree_node *left;
    struct _rbtree_node *parent;
    KEY_TYPE key;
    void *value;
} rbtree_node;

typedef struct _rbtree
{
    rbtree_node *root;
    rbtree_node *nil;
} rbtree;

typedef struct _rbtree kvs_rbtree_t;

int kvs_rbtree_create(kvs_rbtree_t *inst);
void kvs_rbtree_destory(kvs_rbtree_t *inst);
int kvs_rbtree_set(kvs_rbtree_t *inst, char *key, char *value);
char *kvs_rbtree_get(kvs_rbtree_t *inst, char *key);
int kvs_rbtree_del(kvs_rbtree_t *inst, char *key);
int kvs_rbtree_mod(kvs_rbtree_t *inst, char *key, char *value);
int kvs_rbtree_exist(kvs_rbtree_t *inst, char *key);

#endif

#if ENABLE_HASH

#define MAX_KEY_LEN 128
#define MAX_VALUE_LEN 512
#define MAX_TABLE_SIZE 1024

#define ENABLE_KEY_POINTER 1

typedef struct hashnode_s
{
#if ENABLE_KEY_POINTER
    char *key;
    char *value;
#else
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
#endif
    struct hashnode_s *next;

} hashnode_t;

typedef struct hashtable_s
{

    hashnode_t **nodes; //* change **,

    int max_slots;
    int count;

} hashtable_t;

typedef struct hashtable_s kvs_hash_t;
#endif

#if ENABLE_SKIPTABLE

typedef struct SkipNode {
    char *key;         // 元素名
    char *value;       // 元素值（可选）
    double score;      // 分数
    struct SkipNode **forward;
} SkipNode;

typedef struct SkipList {
    int level;
    SkipNode *header;
    pthread_rwlock_t lock;
    int length; // 节点总数
} SkipList;

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    double score;
} ZRangeEntry;

int kvs_skiptable_create(SkipList *inst);
void kvs_skiptable_destory(SkipList *inst);
// 插入或更新元素，返回0成功，1已存在（分数被更新），-1失败
int kvs_skiptable_zadd(SkipList *list, const char *key, const char *value, double score);
// 删除元素，返回0成功，-1不存在
int kvs_skiptable_zrem(SkipList *list, const char *key);
// 查询key对应的score，返回分数，未找到返回-1
int kvs_skiptable_zrange(SkipList *list, double min, double max, char *result, int max_result_len);

SkipList *find_zset(const char *zset_name) ;
// find_or_create_zset(const char *zset_name);
#endif

#if ENABLE_RBTREE
// 红黑树遍历回调函数
static void rbtree_save_callback(rbtree_node *node, FILE *fp);

// 封装红黑树遍历函数
void kvs_rbtree_traverse(kvs_rbtree_t *rbtree, void (*callback)(rbtree_node *, FILE *), FILE *fp);
#endif

#if ENABLE_HASH
// 哈希表遍历回调函数
static void hash_save_callback(hashnode_t *node, FILE *fp);
// 封装哈希表遍历函数
void kvs_hash_traverse(kvs_hash_t *hash, void (*callback)(hashnode_t *, FILE *), FILE *fp);

int kvs_hash_create(kvs_hash_t *hash);
void kvs_hash_destory(kvs_hash_t *hash);
int kvs_hash_set(hashtable_t *hash, char *key, char *value);
char *kvs_hash_get(kvs_hash_t *hash, char *key);
int kvs_hash_mod(kvs_hash_t *hash, char *key, char *value);
int kvs_hash_del(kvs_hash_t *hash, char *key);
int kvs_hash_exist(kvs_hash_t *hash, char *key);

#endif

// 基础内存管理函数
void *kvs_malloc(size_t size);
void kvs_free(void *ptr);

// 内存池类型声明
typedef struct mem_pool mem_pool_t;

// 内存池管理函数
mem_pool_t *kvs_mempool_create(size_t block_size, size_t block_count);
void *kvs_mempool_alloc(mem_pool_t *mp);
void kvs_mempool_free(mem_pool_t *mp, void *ptr);
void kvs_mempool_destroy(mem_pool_t *mp);

typedef struct
{
    uint32_t magic;    // 魔数，用于验证协议 "KVST"
    uint32_t version;  // 协议版本
    uint32_t cmd_type; // 命令类型
    uint32_t data_len; // 数据长度
    uint32_t seq_id;   // 序列号
    uint32_t flags;    // 标志位
} __attribute__((packed)) tcp_header_t;

#define TCP_MAGIC 0x5453564B // "KVST"
#define TCP_VERSION 1
#define TCP_HEADER_SIZE 24


// 协议头解析函数
int parse_tcp_header(const char *buffer, tcp_header_t *header); // header是协议头，buffer是协议头+数据

int build_tcp_response(char *response, uint32_t cmd_type, uint32_t seq_id, const char *data, int data_len);
// response是响应数据，cmd_type是命令类型，seq_id是序列号，data是数据，data_len是数据长度

// TCP协议处理函数声明
int kvs_tcp_protocol(char *buffer, int buffer_len, char *response);

// RDB配置相关函数
int kvs_rdb_set_path(const char *path);
int kvs_rdb_set_directory(const char *dir);
int kvs_rdb_set_filename(const char *filename);
int kvs_rdb_set_auto_save(int enabled, int interval);
int kvs_rdb_get_full_path(char *full_path, size_t max_len);
void kvs_rdb_print_config(void);
int kvs_rdb_init_config(void);

// 命令类型定义
enum
{
    CMD_SET = 1,
    CMD_GET = 2,
    CMD_DEL = 3,
    CMD_MOD = 4,
    CMD_EXIST = 5,
    CMD_SAVE = 6,
    // 红黑树命令
    CMD_RSET = 11,
    CMD_RGET = 12,
    CMD_RDEL = 13,
    CMD_RMOD = 14,
    CMD_REXIST = 15,
    // 哈希表命令
    CMD_HSET = 21,
    CMD_HGET = 22,
    CMD_HDEL = 23,
    CMD_HMOD = 24,
    CMD_HEXIST = 25,
    // 跳表zset命令
    CMD_ZADD = 31,
    CMD_ZREM = 32,
    CMD_ZRANGE = 33,
   
};

#endif // __KV_STORE_H__