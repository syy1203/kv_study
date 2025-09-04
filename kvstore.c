#include "kvstore.h"


#if ENABLE_ARRAY
extern kvs_array_t global_array;
#endif

#if ENABLE_RBTREE
extern kvs_rbtree_t global_rbtree;
#endif

#if ENABLE_HASH
extern kvs_hash_t global_hash;
#endif

#if ENABLE_SKIPTABLE
// 如果有单独头文件，否则用kvstore.h
extern SkipList global_skiptable;
#endif

void *kvs_malloc(size_t size) {
	
	if(size<=POOL_CHUNK_SIZE)
	{
		void *p=kvs_mempool_alloc(global_pool);
		if(p) 
		return p;
	}
	return malloc(size);
}

void kvs_free(void *ptr) {
	if(!ptr) return ;
	if(global_pool){
		 return kvs_mempool_free(global_pool,ptr);
	}
	else{
		return free(ptr);
	}
	
}


const char *command[] = {
	"SET", "GET", "DEL", "MOD", "EXIST",
	"RSET", "RGET", "RDEL", "RMOD", "REXIST",
	"HSET", "HGET", "HDEL", "HMOD", "HEXIST",
	"SAVE","ZADD", "ZREM", "ZSCORE"
};

enum {
	KVS_CMD_START = 0,
	// array
	KVS_CMD_SET = KVS_CMD_START,
	KVS_CMD_GET,
	KVS_CMD_DEL,
	KVS_CMD_MOD,
	KVS_CMD_EXIST,
	

	// rbtree
	KVS_CMD_RSET,
	KVS_CMD_RGET,
	KVS_CMD_RDEL,
	KVS_CMD_RMOD,
	KVS_CMD_REXIST,
	// hash
	KVS_CMD_HSET,
	KVS_CMD_HGET,
	KVS_CMD_HDEL,
	KVS_CMD_HMOD,
	KVS_CMD_HEXIST,
    KVS_CMD_SAVE,	// save  ---------------新增-------------


    KVS_CMD_ZADD,
	KVS_CMD_ZREM,
    KVS_CMD_ZSCORE,

	KVS_CMD_COUNT,
};


const char *response[] = {

};


// int kvs_split_token(char *msg, char *tokens[]) {

// 	if (msg == NULL || tokens == NULL) return -1;

// 	int idx = 0;
// 	char *token = strtok(msg, " ");
	
// 	while (token != NULL) {
// 		//printf("idx: %d, %s\n", idx, token);
		
// 		tokens[idx ++] = token;
// 		token = strtok(NULL, " ");
// 	}

// 	return idx;
// }


// SET Key Value
// tokens[0] : SET
// tokens[1] : Key
// tokens[2] : Value

// int kvs_filter_protocol(char **tokens, int count, char *response) {

// 	if (tokens[0] == NULL || count == 0 || response == NULL) return -1;

// 	int cmd = KVS_CMD_START;
// 	for (cmd = KVS_CMD_START;cmd < KVS_CMD_COUNT;cmd ++) {
// 		if (strcmp(tokens[0], command[cmd]) == 0) {
// 			break;
// 		} 
// 	}

// 	int length = 0;
// 	int ret = 0;
// 	char *key = tokens[1];
// 	char *value = tokens[2];
//     printf("cmd: %d\n", cmd);
//     printf("cmd: %s\n", command[cmd]); 
// 	switch(cmd) {
// #if ENABLE_ARRAY
// 	case KVS_CMD_SET:
// 		ret = kvs_array_set(&global_array ,key, value);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
// 		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "EXIST\r\n");
// 		} 
		
// 		break;
// 	case KVS_CMD_GET: {
// 		char *result = kvs_array_get(&global_array, key);
// 		if (result == NULL) {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		} else {
// 			length = sprintf(response, "%s\r\n", result);
// 		}
// 		break;
// 	}
// 	case KVS_CMD_DEL:
// 		ret = kvs_array_del(&global_array ,key);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
//  		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
// 	case KVS_CMD_MOD:
// 		ret = kvs_array_mod(&global_array ,key, value);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
//  		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
// 	case KVS_CMD_EXIST:
// 		ret = kvs_array_exist(&global_array ,key);
// 		if (ret == 0) {
// 			length = sprintf(response, "EXIST\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
	
// #endif
// 	// rbtree

    
// #if ENABLE_RBTREE
// 	case KVS_CMD_RSET:
// 		ret = kvs_rbtree_set(&global_rbtree ,key, value);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
// 		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "EXIST\r\n");
// 		} 
		
// 		break;
// 	case KVS_CMD_RGET: {
// 		char *result = kvs_rbtree_get(&global_rbtree, key);
// 		if (result == NULL) {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		} else {
// 			length = sprintf(response, "%s\r\n", result);
// 		}
// 		break;
// 	}
// 	case KVS_CMD_RDEL:
// 		ret = kvs_rbtree_del(&global_rbtree ,key);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
//  		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
// 	case KVS_CMD_RMOD:
// 		ret = kvs_rbtree_mod(&global_rbtree ,key, value);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
//  		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
// 	case KVS_CMD_REXIST:
// 		ret = kvs_rbtree_exist(&global_rbtree ,key);
// 		if (ret == 0) {
// 			length = sprintf(response, "EXIST\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
// #endif
// #if ENABLE_HASH
// 	case KVS_CMD_HSET:
// 		ret = kvs_hash_set(&global_hash ,key, value);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
// 		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "EXIST\r\n");
// 		} 
		
// 		break;
// 	case KVS_CMD_HGET: {
// 		char *result = kvs_hash_get(&global_hash, key);
// 		if (result == NULL) {
// 			length = sprintf(response, "NO EXIST HASH_get\r\n");
// 		} else {
// 			length = sprintf(response, "%s\r\n", result);
// 		}
// 		break;
// 	}
// 	case KVS_CMD_HDEL:
// 		ret = kvs_hash_del(&global_hash ,key);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
//  		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
// 	case KVS_CMD_HMOD:
// 		ret = kvs_hash_mod(&global_hash ,key, value);
// 		if (ret < 0) {
// 			length = sprintf(response, "ERROR\r\n");
//  		} else if (ret == 0) {
// 			length = sprintf(response, "OK\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
// 	case KVS_CMD_HEXIST:
// 		ret = kvs_hash_exist(&global_hash ,key);
// 		if (ret == 0) {
// 			length = sprintf(response, "EXIST\r\n");
// 		} else {
// 			length = sprintf(response, "NO EXIST\r\n");
// 		}
// 		break;
// 	case KVS_CMD_SAVE:
// 		ret = kvs_rdb_save(g_rdb_config.rdb_file);
// 		if (ret < 0)
// 		{
// 			length = sprintf(response, "SAVE ERROR\r\n");
// 		}
// 		else
// 		{
// 			length = sprintf(response, "SAVED\r\n");
// 		}
// 		break;
// #endif


// 	default: 
// 		assert(0);
// 	}

// 	return length;
// }


/*
 * msg: request message
 * length: length of request message
 * response: need to send
 * @return : length of response
 */

// int kvs_protocol(char *msg, int length, char *response) {  //
	
// // SET Key Value
// // GET Key
// // DEL Key
// 	if (msg == NULL || length <= 0 || response == NULL) return -1;

// 	//printf("recv %d : %s\n", length, msg);

// 	char *tokens[KVS_MAX_TOKENS] = {0};

// 	int count = kvs_split_token(msg, tokens);
// 	if (count == -1) return -1;

// 	//memcpy(response, msg, length);

// 	return kvs_filter_protocol(tokens, count, response);
// }


int init_kvengine(void) {
	if (!global_pool) {
        global_pool = kvs_mempool_create(POOL_CHUNK_SIZE, POOL_CHUNK_COUNT);
    }
	
#if ENABLE_ARRAY
	memset(&global_array, 0, sizeof(kvs_array_t));
	kvs_array_create(&global_array);
#endif

#if ENABLE_RBTREE
	memset(&global_rbtree, 0, sizeof(kvs_rbtree_t));
	kvs_rbtree_create(&global_rbtree);
#endif

#if ENABLE_HASH
	memset(&global_hash, 0, sizeof(kvs_hash_t));
	kvs_hash_create(&global_hash);
#endif

#if ENABLE_SKIPTABLE
	memset(&global_skiptable, 0, sizeof(SkipList));
	kvs_skiptable_create(&global_skiptable);
#endif

	// 初始化RDB模块
	if (kvs_rdb_init() != 0) {
		printf("[ERROR] RDB初始化失败\n");
		return -1;
	}

	return 0;
}

void dest_kvengine(void) {
#if ENABLE_ARRAY
	kvs_array_destory(&global_array);
#endif
#if ENABLE_RBTREE
	kvs_rbtree_destory(&global_rbtree);
#endif
#if ENABLE_HASH
	kvs_hash_destory(&global_hash);
#endif
#if ENABLE_SKIPTABLE
	kvs_skiptable_destory(&global_skiptable);
#endif


   kvs_mempool_destory(global_pool);

}




// 解析TCP协议头
// int parse_tcp_header(const char *buffer, tcp_header_t *header) {
//     if (!buffer || !header) return -1;
    
//     memcpy(header, buffer, TCP_HEADER_SIZE);
    
//     // 检查魔数
//     if (header->magic != TCP_MAGIC) {
//         printf("Invalid magic: %x\n", header->magic);
//         return -2;
//     }
    
//     // 检查版本
//     if (header->version != TCP_VERSION) {
//         printf("Unsupported version: %d\n", header->version);
//         return -3;
//     }
    
//     // 检查数据长度
//     if (header->data_len > 1024) {
//         printf("Data too long: %d\n", header->data_len);
//         return -4;
//     }
    
//     return 0;
// }

int parse_tcp_header(const char *buffer, tcp_header_t *header) {
	if (!buffer || !header) return -1;

	uint32_t val;
	memcpy(&val, buffer + 0,  4); header->magic    = ntohl(val);
	memcpy(&val, buffer + 4,  4); header->version  = ntohl(val);
	memcpy(&val, buffer + 8,  4); header->cmd_type = ntohl(val);
	memcpy(&val, buffer + 12, 4); header->data_len = ntohl(val);
	memcpy(&val, buffer + 16, 4); header->seq_id   = ntohl(val);
	memcpy(&val, buffer + 20, 4); header->flags    = ntohl(val);

	printf("解析后的 header：magic=%x version=%d cmd_type=%d data_len=%d seq_id=%d flags=%d\n",
		header->magic, header->version, header->cmd_type, header->data_len, header->seq_id, header->flags);

	// 校验
	if (header->magic != TCP_MAGIC) {
		printf("Invalid magic: %x\n", header->magic);
		return -2;
	}
	if (header->version != TCP_VERSION) {
		printf("Unsupported version: %d\n", header->version);
		return -3;
	}
	if (header->data_len > 1024) {
		printf("Data too long: %d\n", header->data_len);
		return -4;
	}
	return 0;
}



// 构建TCP响应
int build_tcp_response(char *response, uint32_t cmd_type, uint32_t seq_id, const char *data, int data_len) {
    if (!response) return -1;

    uint32_t net_val;
    net_val = htonl(TCP_MAGIC);      memcpy(response + 0,  &net_val, 4);
    net_val = htonl(TCP_VERSION);    memcpy(response + 4,  &net_val, 4);
    net_val = htonl(cmd_type);       memcpy(response + 8,  &net_val, 4);
    net_val = htonl(data_len);       memcpy(response + 12, &net_val, 4);
    net_val = htonl(seq_id);         memcpy(response + 16, &net_val, 4);
    net_val = htonl(0);              memcpy(response + 20, &net_val, 4);

    // 写入数据
    if (data && data_len > 0) {
        memcpy(response + TCP_HEADER_SIZE, data, data_len);
    }

    return TCP_HEADER_SIZE + data_len;
}

// 新的协议处理函数
int kvs_tcp_protocol(char *buffer, int buffer_len, char *response) {
    if (buffer_len < TCP_HEADER_SIZE) {
        printf("数据包太短: %d < %d\n", buffer_len, TCP_HEADER_SIZE);
        return -1;
    }
    
    tcp_header_t header;
    int ret = parse_tcp_header(buffer, &header);
    if (ret < 0) {
        printf("协议头解析失败: %d\n", ret);
        return ret;
    }
    

    
    // 检查数据完整性
    if (buffer_len < TCP_HEADER_SIZE + header.data_len) {
        printf("数据不完整: %d < %d\n", buffer_len, TCP_HEADER_SIZE + header.data_len);
        return -5; // 数据不完整
    }
    
    char *data = buffer + TCP_HEADER_SIZE;
    char *key = NULL, *value = NULL;
    
    // 解析命令数据（key和value用空格分隔）
    if (header.data_len > 0) {
        data[header.data_len] = '\0'; // 确保字符串结束
        
        char *space = strchr(data, ' ');
        if (space) {
            *space = '\0';
            key = data;
            value = space + 1;
            printf("解析命令: key='%s', value='%s'\n", key, value);
        } else {
            key = data;
            printf("解析命令: key='%s', value=NULL\n", key);
        }
    }
    
    // 处理命令
    char result[1024] = {0};
    int result_len = 0;
    
    switch (header.cmd_type) {
#if ENABLE_ARRAY
    case CMD_SET:
        ret = kvs_array_set(&global_array, key, value);
        if (ret < 0) {
            strcpy(result, "ERROR");
        } else if (ret == 0) {
            strcpy(result, "OK");
        } else {
            strcpy(result, "EXIST");
        }
        printf("SET结果: %s (ret=%d)\n", result, ret);
        break;
        
    case CMD_GET: {
        char *get_result = kvs_array_get(&global_array, key);
        if (get_result == NULL) {
            strcpy(result, "NO EXIST");
        } else {
            strcpy(result, get_result);
        }
        printf("GET结果: %s\n", result);
        break;
    }
        
    case CMD_DEL:
        ret = kvs_array_del(&global_array, key);
        if (ret < 0) {
            strcpy(result, "ERROR");
        } else if (ret == 0) {
            strcpy(result, "OK");
        } else {
            strcpy(result, "NO EXIST");
        }
        break;
        
    case CMD_MOD:
        ret = kvs_array_mod(&global_array, key, value);
        if (ret < 0) {
            strcpy(result, "ERROR");
        } else if (ret == 0) {
            strcpy(result, "OK");
        } else {
            strcpy(result, "NO EXIST");
        }
        break;
        
    case CMD_EXIST:
        ret = kvs_array_exist(&global_array, key);
        if (ret == 0) {
            strcpy(result, "EXIST");
        } else {
            strcpy(result, "NO EXIST");
        }
        break;
#endif

#if ENABLE_RBTREE
    case CMD_RSET:
        ret = kvs_rbtree_set(&global_rbtree, key, value);
        if (ret < 0) {
            strcpy(result, "ERROR");
        } else if (ret == 0) {
            strcpy(result, "OK");
        } else {
            strcpy(result, "EXIST");
        }
        break;
        
    case CMD_RGET: {
        char *get_result = kvs_rbtree_get(&global_rbtree, key);
        if (get_result == NULL) {
            strcpy(result, "NO EXIST");
        } else {
            strcpy(result, get_result);
        }
        break;
    }
#endif

#if ENABLE_HASH
    case CMD_HSET:
        ret = kvs_hash_set(&global_hash, key, value);
        if (ret < 0) {
            strcpy(result, "ERROR");
        } else if (ret == 0) {
            strcpy(result, "OK");
        } else {
            strcpy(result, "EXIST");
        }
        break;
        
    case CMD_HGET: {
        char *get_result = kvs_hash_get(&global_hash, key);
        if (get_result == NULL) {
            strcpy(result, "NO EXIST");
        } else {
            strcpy(result, get_result);
        }
        break;
    }
#endif

#if ENABLE_SKIPTABLE
    case CMD_ZADD: {
        if (!key || !value) {
            strcpy(result, "ERROR");
            break;
        }
        char *score_str = strchr(value, ' ');
        if (!score_str) {
            strcpy(result, "ERROR");
            break;
        }
        *score_str = '\0';
        double score = atof(score_str + 1);
        // 单集合：直接用 global_skiptable
        ret = kvs_skiptable_zadd(&global_skiptable, key, value, score);
        if (ret < 0) strcpy(result, "ERROR");
        else if (ret == 0) strcpy(result, "OK");
        else strcpy(result, "EXIST");
        break;
    }
    case CMD_ZREM: {
        if (!key) {
            strcpy(result, "ERROR");
            break;
        }
        // 单集合：直接用 global_skiptable
        ret = kvs_skiptable_zrem(&global_skiptable, key);
        if (ret == 0) strcpy(result, "OK");
        else strcpy(result, "NO EXIST");
        break;
    }
    case CMD_ZRANGE:
    // 期望格式：ZRANGE member min max
    if (!key || !value) {
        strcpy(result, "ERROR");
        break;
    }
    {
        // 解析 key: "member min"
        
       double min,max;
        if (sscanf(value, "%lf %lf", &min, &max) != 2) {
            strcpy(result, "ERROR");
            break;
        }
        printf("%lf %lf\n",&min,&max);
        
        ZRangeEntry entries[100];  // 最多返回100个
        int n = kvs_skiptable_zrange_entries(&global_skiptable, min, max, entries, 100);

        // 把结构体数据转为字符串返回
        int len = 0;
        if (n == 0) {
            result[0] = '\0'; // 返回空字符串
        } else {
            for (int i = 0; i < n; ++i) {
                int written = snprintf(result + len, sizeof(result) - len,
                                       "%s %s %.2f\n",
                                       entries[i].key, entries[i].value, entries[i].score);
                if (written < 0 || written >= (int)(sizeof(result) - len)) {
                    break;  // 防止溢出
                }
                len += written;
            }
        }
    }
    break;

#endif

    case CMD_SAVE:
        ret = kvs_rdb_save(g_rdb_config.rdb_file);
        if (ret < 0) {
            strcpy(result, "SAVE ERROR");
        } else {
            strcpy(result, "SAVED");
        }
        break;
        
    default:
        strcpy(result, "UNKNOWN COMMAND");
        printf("未知命令: %d\n", header.cmd_type);
        break;
    }
    
    result_len = strlen(result);
    int resp_len = build_tcp_response(response, header.cmd_type, header.seq_id, result, result_len);
    printf("发送响应: %s, 长度: %d\n", result, resp_len);
    return resp_len;
}


int main(int argc, char *argv[]) {
  	
	if (argc < 2) {
        printf("用法: %s <端口> [RDB路径]\n", argv[0]);
        printf("示例: %s 8080\n", argv[0]);
        printf("示例: %s 8080 /data/kvstore.rdb\n", argv[0]);
        printf("示例: %s 8080 /data/db/\n", argv[0]);
        return -1;
    }

	int port = atoi(argv[1]);

	// 如果提供了RDB路径参数，则设置自定义路径
	if (argc >= 3) {
		if (kvs_rdb_set_path(argv[2]) != 0) {
			printf("警告：RDB路径设置失败，使用默认路径\n");
		}
	}
	
	// 初始化KV引擎（包括RDB）
	if (init_kvengine() != 0) {
		printf("KV引擎初始化失败\n");
		return -1;
	}
	
	
	 // 创建并分离守护线程
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, check_auto_save, g_rdb_config.rdb_file);
    pthread_detach(thread_id);

#if (NETWORK_SELECT == NETWORK_REACTOR)
	reactor_start(port, kvs_tcp_protocol);  // 使用新的TCP协议处理函数
#elif (NETWORK_SELECT == NETWORK_NTYCO)
	ntyco_start(port, kvs_tcp_protocol);    // 使用新的TCP协议处理函数
#elif (NETWORK_SELECT == NETWORK_PROACTOR)
	proactor_start(port, kvs_tcp_protocol); // 使用新的TCP协议处理函数
#endif
    

    kvs_rdb_save(g_rdb_config.rdb_file);//保存数据,在退出时候
	dest_kvengine();

}
