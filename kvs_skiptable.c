


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include"kvstore.h"
// zset风格API实现
#define  SKIP_MAX_LEVEL 6

#define MAX_KEY_LEN 128
#define MAX_VALUE_LEN 128




SkipList global_skiptable;

static int randomLevel() {
    int level = 0;
    while ((rand() & 0xFFFF) < 0.5 * 0xFFFF && level < SKIP_MAX_LEVEL)
        level++;
    return level;
}

static SkipNode *createSkipNode(int level, const char *key, const char *value, double score) {
    SkipNode *node = (SkipNode *)malloc(sizeof(SkipNode));
    node->key = strdup(key);
    node->value = strdup(value);
    node->score = score;
    node->forward = (SkipNode **)malloc((level + 1) * sizeof(SkipNode *));
    for (int i = 0; i <= level; ++i) node->forward[i] = NULL;
    return node;
}

int kvs_skiptable_zadd(SkipList *list, const char *key, const char *value, double score) {
    pthread_rwlock_wrlock(&list->lock);
    SkipNode *update[SKIP_MAX_LEVEL + 1];
    SkipNode *current = list->header;
    for (int i = list->level; i >= 0; --i) {
        while (current->forward[i] && (current->forward[i]->score < score || (current->forward[i]->score == score && strcmp(current->forward[i]->key, key) < 0)))
            current = current->forward[i];
        update[i] = current;
    }
    current = current->forward[0];
    if (current && current->score == score && strcmp(current->key, key) == 0) {
        free(current->value);
        current->value = strdup(value);
        pthread_rwlock_unlock(&list->lock);
        return 1; // 已存在，更新
    }
    int level = randomLevel();
    if (level > list->level) {
        for (int i = list->level + 1; i <= level; ++i)
            update[i] = list->header;
        list->level = level;
    }
    SkipNode *newNode = createSkipNode(level, key, value, score);
    for (int i = 0; i <= level; ++i) {
        newNode->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = newNode;
    }
    list->length++;
    pthread_rwlock_unlock(&list->lock);
    return 0;
}

int kvs_skiptable_zrem(SkipList *list, const char *key) {
    pthread_rwlock_wrlock(&list->lock);
    SkipNode *update[SKIP_MAX_LEVEL + 1];
    SkipNode *current = list->header;
    for (int i = list->level; i >= 0; --i) {
        while (current->forward[i] && strcmp(current->forward[i]->key, key) < 0)
            current = current->forward[i];
        update[i] = current;
    }
    current = current->forward[0];
    if (current && strcmp(current->key, key) == 0) {
        for (int i = 0; i <= list->level; ++i) {
            if (update[i]->forward[i] != current)
                break;
            update[i]->forward[i] = current->forward[i];
        }
        free(current->key);
        free(current->value);
        free(current->forward);
        free(current);
        while (list->level > 0 && list->header->forward[list->level] == NULL)
            list->level--;
        list->length--;
        pthread_rwlock_unlock(&list->lock);
        return 0;
    }
    pthread_rwlock_unlock(&list->lock);
    return -1;
}



// 范围查询：返回[min, max]区间的有序成员集合（member value score\n...）
// int kvs_skiptable_zrange(SkipList *list, double min, double max, char *result, int max_result_len) {
//     pthread_rwlock_rdlock(&list->lock);
//     SkipNode *current = list->header->forward[0];
//     int len = 0;
//     while (current && current->score < min)
//         current = current->forward[0];
//     while (current && current->score <= max) {
//         int n = snprintf(result + len, max_result_len - len, "%s %s %f\n", current->key, current->value, current->score);
//         if (n < 0 || n >= max_result_len - len) break;
//         len += n;
//         current = current->forward[0];
//     }
//     pthread_rwlock_unlock(&list->lock);
//     return len;
// }
int kvs_skiptable_zrange_entries(SkipList *list, double min, double max, ZRangeEntry *results, int max_results) {
    pthread_rwlock_rdlock(&list->lock);

    SkipNode *current = list->header->forward[0];
    int count = 0;

    // 找到第一个满足条件的节点
    while (current && current->score < min)
        current = current->forward[0];

    // 遍历[min, max] 范围内的节点
    while (current && current->score <= max && count < max_results) {
        strncpy(results[count].key, current->key, MAX_KEY_LEN - 1);
        results[count].key[MAX_KEY_LEN - 1] = '\0';

        strncpy(results[count].value, current->value, MAX_VALUE_LEN - 1);
        results[count].value[MAX_VALUE_LEN - 1] = '\0';

        results[count].score = current->score;

        count++;
        current = current->forward[0];
    }

    pthread_rwlock_unlock(&list->lock);
    return count;  // 返回实际填充的 entry 数量
}



// 跳表初始化
int kvs_skiptable_create(SkipList *inst) {
    if (!inst) return -1;
    inst->level = 0;
    inst->header = createSkipNode(SKIP_MAX_LEVEL, "", "", 0);
    for (int i = 0; i <= SKIP_MAX_LEVEL; ++i) inst->header->forward[i] = NULL;
    pthread_rwlock_init(&inst->lock, NULL);
    inst->length = 0;
    return 0;
}

// 跳表销毁
void kvs_skiptable_destory(SkipList *inst) {
    if (!inst) return;
    SkipNode *cur = inst->header;
    while (cur) {
        SkipNode *nxt = cur->forward[0];
        free(cur->key);
        free(cur->value);
        free(cur->forward);
        free(cur);
        cur = nxt;
    }
    pthread_rwlock_destroy(&inst->lock);
    inst->header = NULL;
    inst->level = 0;
    inst->length = 0;
}

// 多集合跳表哈希表（与协议层一致）
#define ZSET_TABLE_SIZE 1024

typedef struct ZSetEntry {
    char *zset_name;
    SkipList *zset;
    struct ZSetEntry *next;
} ZSetEntry;

static ZSetEntry *zset_table[ZSET_TABLE_SIZE] = {0};

static uint32_t zset_hash(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash % ZSET_TABLE_SIZE;
}

SkipList *find_zset(const char *zset_name) {
    uint32_t idx = zset_hash(zset_name);
    ZSetEntry *entry = zset_table[idx];
    while (entry) {
        if (strcmp(entry->zset_name, zset_name) == 0)
            return entry->zset;
        entry = entry->next;
    }
    return NULL;
}

// SkipList *find_or_create_zset(const char *zset_name) {
//     uint32_t idx = zset_hash(zset_name);
//     ZSetEntry *entry = zset_table[idx];
//     while (entry) {
//         if (strcmp(entry->zset_name, zset_name) == 0)
//             return entry->zset;
//         entry = entry->next;
//     }
//     // 不存在则新建
//     ZSetEntry *new_entry = (ZSetEntry *)malloc(sizeof(ZSetEntry));
//     new_entry->zset_name = strdup(zset_name);
//     new_entry->zset = (SkipList *)malloc(sizeof(SkipList));
//     kvs_skiptable_create(new_entry->zset);
//     new_entry->next = zset_table[idx];
//     zset_table[idx] = new_entry;
//     return new_entry->zset;
// }




