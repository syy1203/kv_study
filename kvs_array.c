#include "kvstore.h"


// singleton

kvs_array_t global_array = {0};

int kvs_array_create(kvs_array_t *inst) {

	if (!inst) return -1;
	if (inst->table) {
		printf("table has alloc\n");
		return -1;
	}	
	inst->table = kvs_malloc(KVS_ARRAY_SIZE * sizeof(kvs_array_item_t));
	if (!inst->table) {
		return -1;
	}

	inst->total = 0;
    pthread_mutex_init(&inst->array_mutex, NULL);// 初始化互斥锁
	return 0;
}

void kvs_array_destory(kvs_array_t *inst) {

	if (!inst) return ;

	if (inst->table) {
		kvs_free(inst->table);
	}
	pthread_mutex_destroy(&inst->array_mutex);// 销毁互斥锁

}


/*
 * @return: <0, error; =0, success; >0, exist
 */

int kvs_array_set(kvs_array_t *inst, char *key, char *value) {
    pthread_mutex_lock(&inst->array_mutex);
    if (inst == NULL || key == NULL || value == NULL) {
        pthread_mutex_unlock(&inst->array_mutex);
        return -1;
    }
    if (inst->total == KVS_ARRAY_SIZE) {
        pthread_mutex_unlock(&inst->array_mutex);
        return -1;
    }
    // 直接在加锁区间内查找key，避免调用kvs_array_get导致重复加锁
    for (int i = 0; i < inst->total; i++) {
        if (inst->table[i].key && strcmp(inst->table[i].key, key) == 0) {
            pthread_mutex_unlock(&inst->array_mutex);
            return 1; // 已存在
        }
    }
    printf("kvs_array_set: %s\n", key);
    char *kcopy = kvs_malloc(strlen(key) + 1);
    if (kcopy == NULL) {
        pthread_mutex_unlock(&inst->array_mutex);
        return -2; // memory alloc error
    }
    memset(kcopy, 0, strlen(key) + 1);
    strncpy(kcopy, key, strlen(key));
    char *kvalue = kvs_malloc(strlen(value) + 1);
    if (kvalue == NULL){
        pthread_mutex_unlock(&inst->array_mutex);
        return -2;
    }
    memset(kvalue, 0, strlen(value) + 1);
    strncpy(kvalue, value, strlen(value));
    int i = 0;
    for (i = 0; i < inst->total; i++) {
        if (inst->table[i].key == NULL) {
            inst->table[i].key = kcopy;
            inst->table[i].value = kvalue;
            inst->total++;
            pthread_mutex_unlock(&inst->array_mutex);
            return 0;
        }
    }
    if (i == inst->total && i < KVS_ARRAY_SIZE) {
        inst->table[i].key = kcopy;
        inst->table[i].value = kvalue;
        inst->total++;
    }
    pthread_mutex_unlock(&inst->array_mutex);
    return 0;
}


char* kvs_array_get(kvs_array_t *inst, char *key) {
    
	pthread_mutex_lock(&inst->array_mutex);

	if (inst == NULL || key == NULL) return NULL;
printf("kvs_array_get: %s\n", key);
	int i = 0;
	for (i = 0;i < inst->total;i ++) {
		if (inst->table[i].key == NULL) {
			continue;
		}

		if (strcmp(inst->table[i].key, key) == 0) {
			pthread_mutex_unlock(&inst->array_mutex);
			return inst->table[i].value;
		}
	}
    pthread_mutex_unlock(&inst->array_mutex);
	return NULL;
}


/*
 * @return < 0, error;  =0,  success; >0, no exist
 */

int kvs_array_del(kvs_array_t *inst, char *key) {
    pthread_mutex_lock(&inst->array_mutex);

	if (inst == NULL || key == NULL) return -1;

	int i = 0;
	for (i = 0;i < inst->total;i ++) {

		if (strcmp(inst->table[i].key, key) == 0) {

			kvs_free(inst->table[i].key);
			inst->table[i].key = NULL;

			kvs_free(inst->table[i].value);
			inst->table[i].value = NULL;
// error: > 1024
			if (inst->total-1 == i) {
				inst->total --;
			}
			
            pthread_mutex_unlock(&inst->array_mutex);
			return 0;
		}
	}
    pthread_mutex_unlock(&inst->array_mutex);
	return i;
}


/*
 * @return : < 0, error; =0, success; >0, no exist 
 */

int kvs_array_mod(kvs_array_t *inst, char *key, char *value) {
    pthread_mutex_lock(&inst->array_mutex);

	if (inst == NULL || key == NULL || value == NULL) return -1;
// error: > 1024
	if (inst->total == 0) {
		pthread_mutex_unlock(&inst->array_mutex);// 
		return KVS_ARRAY_SIZE;
	}
	

	int i = 0;
	for (i = 0;i < inst->total;i ++) {

		if (inst->table[i].key == NULL) {
			continue;
		}

		if (strcmp(inst->table[i].key, key) == 0) {

			kvs_free(inst->table[i].value);

			char *kvalue = kvs_malloc(strlen(value) + 1);
			if (kvalue == NULL) {
				pthread_mutex_unlock(&inst->array_mutex);return -2;
			}
			memset(kvalue, 0, strlen(value) + 1);
			strncpy(kvalue, value, strlen(value));

			inst->table[i].value = kvalue;
          pthread_mutex_unlock(&inst->array_mutex);
			return 0;
		}

	}
    pthread_mutex_unlock(&inst->array_mutex);
	return i;
}


/*
 * @return 0: exist, 1: no exist
 */

// int kvs_array_exist(kvs_array_t *inst, char *key) {
//     pthread_mutex_lock(&inst->array_mutex);
// 	if (!inst || !key) return -1;
	
// 	char *str = kvs_array_get(inst, key);
// 	if (!str) {
// 		return 1; // 
// 	}
// 	return 0;
// }

int kvs_array_exist(kvs_array_t *inst, char *key) {
    pthread_mutex_lock(&inst->array_mutex);
    if (!inst || !key) {
        pthread_mutex_unlock(&inst->array_mutex);
        return -1;
    }

    int i = 0;
    for (i = 0; i < inst->total; i++) {
        if (inst->table[i].key && strcmp(inst->table[i].key, key) == 0) {
            pthread_mutex_unlock(&inst->array_mutex);
            return 0; // 存在
        }
    }

    pthread_mutex_unlock(&inst->array_mutex);
    return 1; // 不存在
}
// void kvs_arrat_range_query(kvs_array_t *inst,const char *key_start,const *key_end)

