#include "kvstore.h"

// 全局RDB配置 - 存储RDB模块的所有配置参数
// 这些是默认值，可以通过配置文件或API调用进行修改
rdb_config_t g_rdb_config = {
    .save_interval = 300,                  // 默认5分钟自动保存
    .rdb_file = "kvs.rdb",                 // 默认RDB文件名
    .rdb_enabled = 1,                      // 默认启用RDB
    .rdb_compression = 0,                  // 默认禁用压缩
    .rdb_crc64 = 1,                        // 默认启用CRC64校验
    .rdb_backup = 1,                       // 默认启用备份
    .backup_dir = "./backup"               // 默认备份目录
};

#if ENABLE_ARRAY
extern kvs_array_t global_array;
#endif

#if ENABLE_RBTREE
extern kvs_rbtree_t global_rbtree;
#endif

#if ENABLE_HASH
extern kvs_hash_t global_hash;
#endif

// CRC64查找表 - 使用ECMA-182标准的多项式
static const uint64_t crc64_table[256] = {
    0x0000000000000000ULL, 0x42F0E1EBA9EA3693ULL, 0x85E1C3D753D46D26ULL, 0xC711223CFA3E5BB5ULL,
    0x493366450E42ECDFULL, 0x0BC387AEA7A8DA4CULL, 0xCCD2A5925D9681F9ULL, 0x8E224479F47CB76AULL,
    0x9266CC8A1C85D9BEULL, 0xD0962D61B56FEF2DULL, 0x17870F5D4F51B498ULL, 0x5577EEB6E6BB820BULL,
    0xDB55AACF12C73561ULL, 0x99A54B24BB2D03F2ULL, 0x5EB4691841135847ULL, 0x1C4488F3E8F96ED4ULL,
    0x663D78FF90E185EFULL, 0x24CD9914390BB37CULL, 0xE3DCBB28C335E8C9ULL, 0xA12C5AC36ADFDE5AULL,
    0x2F0E1EBA9EA36930ULL, 0x6DFEFF5137495FA3ULL, 0xAAEFDD6DCD770416ULL, 0xE81F3C86649D3285ULL,
    0xF45BB4758C645C51ULL, 0xB6AB559E258E6AC2ULL, 0x71BA77A2DFB03177ULL, 0x334A9649765A07E4ULL,
    0xBD68D2308226B08EULL, 0xFF9833DB2BCC861DULL, 0x388911E7D1F2DDA8ULL, 0x7A79F00C7818EB3BULL,
    0xCC7AF1FF21C30BDEULL, 0x8E8A101488293D4DULL, 0x499B3228721766F8ULL, 0x0B6BD3C3DBFD506BULL,
    0x854997BA2F81E701ULL, 0xC7B97651866BD192ULL, 0x00A8546D7C558A27ULL, 0x4258B586D5BFBCB4ULL,
    0x5E1C3D753D46D260ULL, 0x1CECDC9E94ACE4F3ULL, 0xDBFDFEA26E92BF46ULL, 0x990D1F49C77889D5ULL,
    0x172F5B3033043EBFULL, 0x55DFBADB9AEE082CULL, 0x92CE98E760D05399ULL, 0xD03E790CC93A650AULL,
    0xAA478900B1228E31ULL, 0xE8B768EB18C8B8A2ULL, 0x2FA64AD7E2F6E317ULL, 0x6D56AB3C4B1CD584ULL,
    0xE374EF45BF6062EEULL, 0xA1840EAE168A547DULL, 0x66952C92ECB40FC8ULL, 0x2465CD79455E395BULL,
    0x3821458AADA7578FULL, 0x7AD1A461044D611CULL, 0xBDC0865DFE733AA9ULL, 0xFF3067B657990C3AULL,
    0x711223CFA3E5BB50ULL, 0x33E2C2240A0F8DC3ULL, 0xF4F3E018F031D676ULL, 0xB60301F359DBE0E5ULL,
    0xDA050215EA6C212FULL, 0x98F5E3FE438617BCULL, 0x5FE4C1C2B9B84C09ULL, 0x1D14202910527A9AULL,
    0x93366450E42ECDF0ULL, 0xD1C685BB4DC4FB63ULL, 0x16D7A787B7FAA0D6ULL, 0x5427466C1E109645ULL,
    0x4863CE9FF6E9F891ULL, 0x0A932F745F03CE02ULL, 0xCD820D48A53D95B7ULL, 0x8F72ECA30CD7A324ULL,
    0x0150A8DAF8AB144EULL, 0x43A04931514122DDULL, 0x84B16B0DAB7F7968ULL, 0xC6418AE602954FFBULL,
    0xBC387AEA7A8DA4C0ULL, 0xFEC89B01D3679253ULL, 0x39D9B93D2959C9E6ULL, 0x7B2958D680B3FF75ULL,
    0xF50B1CAF74CF481FULL, 0xB7FBFD44DD257E8CULL, 0x70EADF78271B2539ULL, 0x321A3E938EF113AAULL,
    0x2E5EB66066087D7EULL, 0x6CAE578BCFE24BEDULL, 0xABBF75B735DC1058ULL, 0xE94F945C9C3626CBULL,
    0x676DD025684A91A1ULL, 0x259D31CEC1A0A732ULL, 0xE28C13F23B9EFC87ULL, 0xA07CF2199274CA14ULL,
    0x167FF3EACBAF2AF1ULL, 0x548F120162451C62ULL, 0x939E303D987B47D7ULL, 0xD16ED1D631917144ULL,
    0x5F4C95AFC5EDC62EULL, 0x1DBC74446C07F0BDULL, 0xDAAD56789639AB08ULL, 0x985DB7933FD39D9BULL,
    0x84193F60D72AF3AFULL, 0xC6E9DE8B7EC0C53CULL, 0x01F8FCB784FE9E89ULL, 0x43081D5C2D14A81AULL,
    0xCD2A5925D9681F70ULL, 0x8FDAB8CE708229E3ULL, 0x4A6ACB4778EC7256ULL, 0x089A2AACD10644C5ULL,
    0x14DEA25F39FF2A61ULL, 0x562E43B490151CF2ULL, 0x913F61886A2B4747ULL, 0xD3CF8063C3C171D4ULL,
    0x5DFB059B1B3DC6BEULL, 0x1F0BE470B2D7F02DULL, 0xD81AC64C48E9AB98ULL, 0x9AFAC7A7E1039D0BULL,
    0x86B64F5409FAF3DFULL, 0xC446AEBFA010C54CULL, 0x03578C835A2E9EF9ULL, 0x41A76D68F3C4A86AULL,
    0xCF85291107B81F00ULL, 0x8D75C8FAAE522993ULL, 0x4A64EAC6546C7226ULL, 0x08940B2DFD8644B5ULL,
    0x72EDFB21859EAF8EULL, 0x301D1ACACEF4991DULL, 0xF70C38F634CAC2A8ULL, 0xB5FCD91D9D20F43BULL,
    0x3BDED5646A5C4351ULL, 0x792E348FC3B675C2ULL, 0xBE3F16B339882E77ULL, 0xFCCFF758906218E4ULL,
    0xE08B7FAF789B7630ULL, 0xA27B9E44D17140A3ULL, 0x656ABC782B4F1B16ULL, 0x279A5D9382A52D85ULL,
    0xA9BA19EAB6D99AEFULL, 0xEB4AF8011F33AC7CULL, 0x2C5BDADDC50DF7C9ULL, 0x6EA55B366CF7C15AULL,
    0x72E1DA21871EAF8EULL, 0x30113BCACEF4991DULL, 0xF70019F634CAC2A8ULL, 0xB5F0F81D9D20F43BULL,
    0x3BD2BC64695C4351ULL, 0x79225D8FC0B675C2ULL, 0xBE337FB33A882E77ULL, 0xFCC39E58936218E4ULL
};

#if ENABLE_RBTREE
// 红黑树遍历回调函数 - 用于保存红黑树节点数据到RDB文件
// 参数: node - 当前遍历的节点, fp - 文件指针
static void rbtree_save_callback(rbtree_node *node, FILE *fp) {
    if (node == NULL || node->key == NULL) return;  // 跳过空节点
    
    // 获取键值长度
    uint16_t key_len = strlen((char *)node->key);
    uint16_t val_len = strlen((char *)node->value);

    // 写入键值对数据到文件
    // 格式: [类型][键长度][键内容][值长度][值内容]
    uint8_t type = 1;  // 1表示红黑树类型
    fwrite(&type, sizeof(type), 1, fp);
    fwrite(&key_len, sizeof(key_len), 1, fp);
    fwrite(node->key, 1, key_len, fp);
    fwrite(&val_len, sizeof(val_len), 1, fp);
    fwrite(node->value, 1, val_len, fp);
}

// 封装红黑树遍历函数 - 遍历整棵红黑树并调用回调函数
// 参数: rbtree - 红黑树指针, callback - 回调函数, fp - 文件指针
void kvs_rbtree_traverse(kvs_rbtree_t *rbtree, void (*callback)(rbtree_node *, FILE *), FILE *fp) {
    // 这里需要递归实现红黑树的遍历
    // 示例仅实现根节点的处理，完整实现需深度优先遍历整棵树
    if (rbtree && rbtree->root && rbtree->root != rbtree->nil) {
        callback(rbtree->root, fp);
        // TODO: 添加左子树和右子树的递归调用
        // 完整实现应该包括中序遍历或前序遍历
    }
}
#endif

#if ENABLE_HASH
// 哈希表遍历回调函数 - 用于保存哈希表节点数据到RDB文件
// 参数: node - 当前链表节点, fp - 文件指针
static void hash_save_callback(hashnode_t *node, FILE *fp) {
    // 遍历链表中的所有节点
    while (node != NULL) {
        // 获取键值长度
        uint16_t key_len = strlen(node->key);
        uint16_t val_len = strlen(node->value);

        // 写入键值对数据到文件
        // 格式: [类型][键长度][键内容][值长度][值内容]
        uint8_t type = 2;  // 2表示哈希表类型
        fwrite(&type, sizeof(type), 1, fp);
        fwrite(&key_len, sizeof(key_len), 1, fp);
        fwrite(node->key, 1, key_len, fp);
        fwrite(&val_len, sizeof(val_len), 1, fp);
        fwrite(node->value, 1, val_len, fp);
        
        node = node->next;  // 移动到下一个节点
    }
}

// 封装哈希表遍历函数 - 遍历哈希表的所有槽位并调用回调函数
// 参数: hash - 哈希表指针, callback - 回调函数, fp - 文件指针
void kvs_hash_traverse(kvs_hash_t *hash, void (*callback)(hashnode_t *, FILE *), FILE *fp) {
    // 遍历哈希表的所有槽位
    for (int i = 0; i < hash->max_slots; i++) {
        if (hash->nodes[i] != NULL) {
            // 对每个非空槽位调用回调函数
            callback(hash->nodes[i], fp);
        }
    }
}
#endif

// RDB Save Function - 将内存中的数据保存到RDB文件
// 参数: filename - 要保存的文件路径
// 返回值: 0表示成功，-1表示失败
int kvs_rdb_save(const char *filename) {
    // 检查RDB功能是否启用
    if (!g_rdb_config.rdb_enabled) {
        printf("[RDB] RDB功能已禁用\n");
        return 0;
    }

    // 如果启用备份功能且文件已存在，先创建备份
    if (g_rdb_config.rdb_backup && access(filename, F_OK) == 0) {
        rdb_create_backup(filename);
    }

    // 以二进制写模式打开文件
    FILE *fp = fopen(filename, "wb");
    if (!fp) return -1;

    // 准备文件头结构
    rdb_header_t header = {0};
    header.magic = RDB_MAGIC;              // 设置魔数 "RDBK"
    header.version = RDB_VERSION;          // 设置版本号
    header.flags = 0;                      // 初始化标志位
    if (g_rdb_config.rdb_compression) header.flags |= 0x01;  // 设置压缩标志
    if (g_rdb_config.rdb_crc64) header.flags |= 0x02;     // 设置CRC64校验标志
    header.save_time = time(NULL);         // 记录保存时间

    // 写入文件头
    fwrite(&header, sizeof(header), 1, fp);

    // 记录数据开始位置，用于后续计算数据大小
    long data_start = ftell(fp);
    int record_count = 0;

#if ENABLE_ARRAY
    // 保存数组数据结构中的所有键值对
    for (int i = 0; i < global_array.total; i++) {
        kvs_array_item_t *item = &global_array.table[i];
        if (item->key == NULL) continue;  // 跳过空项

        // 写入数组类型记录
        // 格式: [类型][键长度][键内容][值长度][值内容]
        uint8_t type = 0;  // 0 表示数组类型
        fwrite(&type, sizeof(type), 1, fp);

        uint16_t key_len = strlen(item->key);
        uint16_t val_len = strlen(item->value);

        fwrite(&key_len, sizeof(key_len), 1, fp);
        fwrite(item->key, 1, key_len, fp);
        fwrite(&val_len, sizeof(val_len), 1, fp);
        fwrite(item->value, 1, val_len, fp);
        record_count++;
    }
#endif

#if ENABLE_RBTREE
    // 保存红黑树数据结构中的所有键值对
    kvs_rbtree_traverse(&global_rbtree, rbtree_save_callback, fp);
    // 注意：这里需要修改rbtree_save_callback来返回记录数
#endif

#if ENABLE_HASH
    // 保存哈希表数据结构中的所有键值对
    kvs_hash_traverse(&global_hash, hash_save_callback, fp);
    // 注意：这里需要修改hash_save_callback来返回记录数
#endif

    // 计算数据部分的大小
    long data_end = ftell(fp);
    header.data_size = data_end - data_start;
    header.record_count = record_count;
     printf("[DEBUG] datastart_start = %d\n",data_start);
     printf("[DEBUG] header.data_size = %d\n", header.data_size);
    // 如果启用CRC64校验功能，计算CRC64并写入文件尾
    if (g_rdb_config.rdb_crc64) {
        // 回到数据开始位置
        fseek(fp, data_start, SEEK_SET);
        printf("[DEBUG] 数据开始位置: %ld, 当前文件位置: %ld\n", data_start, ftell(fp));
        
        // 读取整个数据部分到内存
        uint8_t *data_buffer = malloc(header.data_size);
        if (!data_buffer) {
            printf("[RDB] 内存分配失败，无法验证CRC64\n");
            fclose(fp);
            return -1;
        }
        
        // 读取数据部分
        printf("[DEBUG] header.data_size = %d\n", header.data_size);
        size_t n = fread(data_buffer, 1, header.data_size, fp);
        printf("[DEBUG] fread返回值 = %zu\n", n);

        if (n != header.data_size) {
            printf("[RDB] 数据读取失败，无法验证CRC64\n");
            free(data_buffer);
            fclose(fp);
            return -1;
        }
        
        // 使用rdb_calculate_crc64函数计算CRC64
        uint64_t calculated_crc64 = rdb_calculate_crc64(data_buffer, header.data_size);
        free(data_buffer);
        
        // 计算当前文件位置，确保文件尾8字节对齐
        long current_pos = ftell(fp);
        long aligned_pos = RDB_ALIGN_SIZE(current_pos);
        long padding_size = aligned_pos - current_pos;
        
        // 写入对齐填充（用0填充）
        if (padding_size > 0) {
            uint8_t padding[8] = {0};
            fwrite(padding, 1, padding_size, fp);
        }
        
        // 写入EOF标记和CRC64
        rdb_footer_t footer = {0};
        memcpy(footer.eof_marker, RDB_EOF_MARKER, RDB_EOF_MARKER_SIZE);
        footer.crc64 = calculated_crc64;
        footer.footer_size = RDB_FOOTER_SIZE;
        footer.alignment_padding = 0;  // 填充字段设为0
        
        // 写入footer前打印
        printf("[DEBUG] 写入footer前文件位置: %ld\n", ftell(fp));
        printf("[DEBUG] footer.eof_marker写入: [%02x %02x %02x %02x] (应为: %02x %02x %02x %02x)\n", \
            footer.eof_marker[0], footer.eof_marker[1], footer.eof_marker[2], footer.eof_marker[3], \
            RDB_EOF_MARKER[0], RDB_EOF_MARKER[1], RDB_EOF_MARKER[2], RDB_EOF_MARKER[3]);
        
        fwrite(&footer, sizeof(footer), 1, fp);
    }

    // 回到文件开头，更新文件头
    fseek(fp, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, fp);

    fclose(fp);

    // 更新最后保存时间并输出保存结果
    last_save_time = time(NULL);
    printf("[RDB] 保存完成，共保存 %d 条记录\n", record_count);
    return 0;
}

// 自动保存守护线程函数 - 定期自动保存数据到RDB文件
// 参数: arg - 文件路径（void*类型，符合pthread要求）
// 返回值: NULL（线程函数要求）
void *check_auto_save(void *arg) {
    const char *filename = (const char *)arg;  // 转换参数类型
    time_t now = time(NULL);
    
    // 无限循环，定期执行自动保存
    while (1) {
        // 休眠配置的保存间隔时间
        sleep(g_rdb_config.save_interval);
        
        // 检查RDB功能是否启用，如果禁用则跳过保存
        if (!g_rdb_config.rdb_enabled) {
            continue; // 如果RDB被禁用，跳过保存
        }
        
        // 获取数组互斥锁，确保数据一致性
        pthread_mutex_lock(&global_array.array_mutex);
        
        // TODO: 获取红黑树和哈希表的互斥锁
        // #if ENABLE_RBTREE
        //     pthread_mutex_lock(&global_rbtree.rbtree_mutex);
        // #endif
        // #if ENABLE_HASH
        //     pthread_mutex_lock(&global_hash.hash_mutex);
        // #endif
        
        // 执行数据保存操作
        if (kvs_rdb_save(filename) == 0) {
            printf("[AUTO SAVE] Saved to %s\n", filename);
        } else {
            printf("[AUTO SAVE] Save failed.\n");
        }
        
        // TODO: 释放红黑树和哈希表的互斥锁
        // #if ENABLE_HASH
        //     pthread_mutex_unlock(&global_hash.hash_mutex);
        // #endif
        // #if ENABLE_RBTREE
        //     pthread_mutex_unlock(&global_rbtree.rbtree_mutex);
        // #endif
        
        // 释放数组互斥锁
        pthread_mutex_unlock(&global_array.array_mutex);
    }
    return NULL;
}

// RDB Load Function - 从RDB文件加载数据到内存
// 参数: filename - 要加载的文件路径
// 返回值: 0表示成功，-1表示失败
int kvs_rdb_load(const char *filename) {
    // 检查RDB功能是否启用
    if (!g_rdb_config.rdb_enabled) {
        printf("[RDB] RDB功能已禁用\n");
        return 0;
    }

    // 验证文件完整性（魔数、版本、CRC64）
    if (rdb_verify_file_integrity(filename) != 0) {
        printf("[RDB] 文件完整性验证失败，尝试从备份恢复\n");
        
        // 如果启用备份功能，尝试从最新的备份恢复
        if (g_rdb_config.rdb_backup) {
            // TODO: 这里可以添加查找最新备份文件的逻辑
            printf("[RDB] 请手动指定备份文件进行恢复\n");
        }
        return -1;
    }

    // 以二进制读模式打开文件
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("无法打开文件: %s\n", filename);
        return -1;
    }

    // 读取文件头结构
    rdb_header_t header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        printf("文件头读取失败\n");
        fclose(fp);
        return -3;
    }

    // 输出文件信息
    printf("[RDB] 加载文件: 版本=%u, 记录数=%u, 数据大小=%u\n", 
           header.version, header.record_count, header.data_size);

    // 逐条读取记录并加载到内存
    int record_count = 0;
    long data_end = ftell(fp) + header.data_size;
    
    while (ftell(fp) < data_end && !feof(fp)) {
        // 读取记录类型
        uint8_t type;
        if (fread(&type, sizeof(type), 1, fp) != 1) break;

        // 读取键长度并进行合理性检查
        uint16_t key_len, val_len;
        if (fread(&key_len, sizeof(key_len), 1, fp) != 1) break;

        // 合理性检查，防止异常大值导致内存分配问题
        if (key_len == 0 || key_len > 1024) {
            printf("key_len异常: %u\n", key_len);
            break;
        }

        // 分配并读取键内容
        char *key = (char *)malloc(key_len + 1);
        if (!key) {
            printf("key内存分配失败\n");
            break;
        }
        if (fread(key, 1, key_len, fp) != key_len) {
            printf("key内容读取失败\n");
            free(key);
            break;
        }
        key[key_len] = '\0';  // 确保字符串结束

        // 读取值长度并进行合理性检查
        if (fread(&val_len, sizeof(val_len), 1, fp) != 1) {
            printf("val_len读取失败\n");
            free(key);
            break;
        }

        if (val_len > 65535) {
            printf("val_len异常: %u\n", val_len);
            free(key);
            break;
        }
        printf("val_len: %d\n", val_len); 
        
        // 分配并读取值内容
        char *val = (char *)malloc(val_len + 1);
        if (!val) {
            printf("val内存分配失败\n");
            free(key);
            break;
        }
        if (fread(val, 1, val_len, fp) != val_len) {
            printf("val内容读取失败\n");
            free(key);
            free(val);
            break;
        }
        val[val_len] = '\0';  // 确保字符串结束
        
        // 根据记录类型将数据加载到对应的数据结构
        switch (type) {
#if ENABLE_ARRAY
        case 0:  // 数组类型
            kvs_array_set(&global_array, key, val);
            break;
#endif
#if ENABLE_RBTREE
        case 1:  // 红黑树类型
            kvs_rbtree_set(&global_rbtree, key, val);
            break;
#endif
#if ENABLE_HASH
        case 2:  // 哈希表类型
            kvs_hash_set(&global_hash, key, val);
            break;
#endif
        default:
            printf("未知type: %d\n", type);
            break;
        }
        
        // printf("record_count: %d\n", record_count);
        free(key);
        free(val);
        record_count++;
    }

    fclose(fp);
    printf("RDB加载完成，共加载%d条记录\n", record_count);
    return 0;
}

// 配置管理函数实现

// 初始化RDB配置 - 加载配置文件或使用默认配置load
// 返回值: 0表示成功，-1表示失败
int rdb_config_init(void) {
    // 尝试加载配置文件，如果失败则使用默认配置
    if (rdb_config_load(RDB_CONFIG_FILE) != 0) {
        printf("[RDB] 配置文件加载失败，使用默认配置\n");
        // 创建默认配置文件供用户参考
        rdb_config_save(RDB_CONFIG_FILE);
    }
    return 0;
}

// 从配置文件加载RDB配置
// 参数: config_file - 配置文件路径
// 返回值: 0表示成功，-1表示失败
int rdb_config_load(const char *config_file) {
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        return -1;
    }

    char line[MAX_CONFIG_LINE];
    while (fgets(line, sizeof(line), fp)) {
        // 跳过注释行（以#开头）和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // 去除行尾的换行符和回车符
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';

        // 查找等号分隔符
        char *equals = strchr(line, '=');
        if (!equals) continue;

        // 分割键值对
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;

        // 去除键值对前后的空格
        while (*key == ' ') key++;
        while (*value == ' ') value++;
        
        char *end = key + strlen(key) - 1;
        while (end > key && *end == ' ') *end-- = '\0';
        end = value + strlen(value) - 1;
        while (end > value && *end == ' ') *end-- = '\0';

        // 根据键名设置对应的配置项
        if (strcmp(key, "save_interval") == 0) {
            g_rdb_config.save_interval = atoi(value);
        } else if (strcmp(key, "rdb_file") == 0) {
            strncpy(g_rdb_config.rdb_file, value, MAX_PATH_LEN - 1);
            g_rdb_config.rdb_file[MAX_PATH_LEN - 1] = '\0';
        } else if (strcmp(key, "rdb_enabled") == 0) {
            g_rdb_config.rdb_enabled = (strcmp(value, "yes") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "rdb_compression") == 0) {
            g_rdb_config.rdb_compression = (strcmp(value, "yes") == 0 || strcmp(value, "1") == 0);
        }  else if (strcmp(key, "rdb_backup") == 0) {
            g_rdb_config.rdb_backup = (strcmp(value, "yes") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "backup_dir") == 0) {
            strncpy(g_rdb_config.backup_dir, value, MAX_PATH_LEN - 1);
            g_rdb_config.backup_dir[MAX_PATH_LEN - 1] = '\0';
        } else if (strcmp(key, "rdb_crc64") == 0) {
            g_rdb_config.rdb_crc64 = (strcmp(value, "yes") == 0 || strcmp(value, "1") == 0);
        }
    }

    fclose(fp);
    return 0;
}

// 保存RDB配置到配置文件
// 参数: config_file - 配置文件路径
// 返回值: 0表示成功，-1表示失败
int rdb_config_save(const char *config_file) {
    FILE *fp = fopen(config_file, "w");
    if (!fp) {
        return -1;
    }

    // 写入配置文件头部注释
    fprintf(fp, "# KVS RDB 配置文件\n");
    fprintf(fp, "# 格式: key = value\n");
    fprintf(fp, "# 注释以 # 开头\n\n");
    
    // 写入各项配置
    fprintf(fp, "# 自动保存间隔（秒）\nsave_interval = %d\n\n", g_rdb_config.save_interval);
    fprintf(fp, "# RDB 文件路径\nrdb_file = %s\n\n", g_rdb_config.rdb_file);
    fprintf(fp, "# 是否启用 RDB\nrdb_enabled = %s\n\n", g_rdb_config.rdb_enabled ? "yes" : "no");
    fprintf(fp, "# 是否启用压缩（可选）\nrdb_compression = %s\n\n", g_rdb_config.rdb_compression ? "yes" : "no");
    fprintf(fp, "# 是否启用CRC64校验\nrdb_crc64 = %s\n\n", g_rdb_config.rdb_crc64 ? "yes" : "no");
    fprintf(fp, "# 是否启用备份\nrdb_backup = %s\n\n", g_rdb_config.rdb_backup ? "yes" : "no");
    fprintf(fp, "# 备份目录\nbackup_dir = %s\n", g_rdb_config.backup_dir);

    fclose(fp);
    return 0;
}

// 打印当前RDB配置信息
void rdb_config_print(void) {
    printf("=== RDB 配置信息 ===\n");
    printf("自动保存间隔: %d 秒\n", g_rdb_config.save_interval);
    printf("RDB 文件路径: %s\n", g_rdb_config.rdb_file);
    printf("RDB 启用状态: %s\n", g_rdb_config.rdb_enabled ? "是" : "否");
    printf("压缩功能: %s\n", g_rdb_config.rdb_compression ? "启用" : "禁用");
    printf("CRC64校验功能: %s\n", g_rdb_config.rdb_crc64 ? "启用" : "禁用");
    printf("备份功能: %s\n", g_rdb_config.rdb_backup ? "启用" : "禁用");
    printf("备份目录: %s\n", g_rdb_config.backup_dir);
    printf("==================\n");
}

// 配置设置函数

// 设置自动保存间隔
// 参数: interval - 保存间隔（秒）
// 返回值: 0表示成功，-1表示失败
int rdb_config_set_save_interval(int interval) {
    if (interval <= 0) return -1;  // 间隔必须大于0
    g_rdb_config.save_interval = interval;
    return 0;
}

// 设置RDB文件路径
// 参数: file - 文件路径
// 返回值: 0表示成功，-1表示失败
int rdb_config_set_rdb_file(const char *file) {
    if (!file) return -1;  // 路径不能为空
    strncpy(g_rdb_config.rdb_file, file, MAX_PATH_LEN - 1);
    g_rdb_config.rdb_file[MAX_PATH_LEN - 1] = '\0';  // 确保字符串结束
    return 0;
}

// 启用/禁用RDB功能
// 参数: enabled - 1表示启用，0表示禁用
// 返回值: 0表示成功
int rdb_config_set_enabled(int enabled) {
    g_rdb_config.rdb_enabled = enabled ? 1 : 0;
    return 0;
}

// 启用/禁用压缩功能
// 参数: enabled - 1表示启用，0表示禁用
// 返回值: 0表示成功
int rdb_config_set_compression(int enabled) {
    g_rdb_config.rdb_compression = enabled ? 1 : 0;
    return 0;
}

// 启用/禁用CRC64校验功能
// 参数: enabled - 1表示启用，0表示禁用
// 返回值: 0表示成功
int rdb_config_set_crc64(int enabled) {
    g_rdb_config.rdb_crc64 = enabled ? 1 : 0;
    return 0;
}

// 启用/禁用备份功能并设置备份目录
// 参数: enabled - 1表示启用，0表示禁用, backup_dir - 备份目录路径
// 返回值: 0表示成功
int rdb_config_set_backup(int enabled, const char *backup_dir) {
    g_rdb_config.rdb_backup = enabled ? 1 : 0;
    if (backup_dir) {
        strncpy(g_rdb_config.backup_dir, backup_dir, MAX_PATH_LEN - 1);
        g_rdb_config.backup_dir[MAX_PATH_LEN - 1] = '\0';  // 确保字符串结束
    }
    return 0;
}

// 验证文件完整性 - 综合验证RDB文件的完整性
// 参数: filename - 要验证的文件路径
// 返回值: 0表示文件完整，-1表示文件损坏
int rdb_verify_file_integrity(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("[RDB] 无法打开文件: %s\n", filename);
        return -1;
    }

    // 读取文件头
    rdb_header_t header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        printf("[RDB] 文件头读取失败\n");
        fclose(fp);
        return -1;
    }

    // 验证魔数 - 确保文件格式正确
    if (header.magic != RDB_MAGIC) {
        printf("[RDB] 文件魔数验证失败\n");
        fclose(fp);
        return -1;
    }

    // 验证版本 - 确保文件版本兼容
    if (header.version != RDB_VERSION) {
        printf("[RDB] 文件版本不兼容: %u\n", header.version);
        fclose(fp);
        return -1;
    }

    // 如果启用校验和功能，验证CRC64
    if (g_rdb_config.rdb_crc64) {
        // 检查文件大小是否足够包含文件尾
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        printf("file_size:%ld\n", file_size);
        if (file_size < sizeof(header) + header.data_size + RDB_FOOTER_SIZE) {
            printf("[RDB] 文件大小不足，无法验证CRC64\n");
            fclose(fp);
            return -1;
        }

        // 读取文件尾
        fseek(fp, file_size - RDB_FOOTER_SIZE, SEEK_SET);
        rdb_footer_t footer;
        if (fread(&footer, sizeof(footer), 1, fp) != 1) {
            printf("[RDB] 文件尾读取失败\n");
            fclose(fp);
            return -1;
        }

        // 验证EOF标记
        if (memcmp(footer.eof_marker, RDB_EOF_MARKER, RDB_EOF_MARKER_SIZE) != 0) {
            printf("[RDB] EOF标记验证失败\n");
            fclose(fp);
            return -1;
        }

        // 验证文件尾大小
        if (footer.footer_size != RDB_FOOTER_SIZE) {
            printf("[RDB] 文件尾大小验证失败\n");
            fclose(fp);
            return -1;
        }

        // 读取footer后打印
        printf("[DEBUG] 读取footer后文件位置: %ld\n", ftell(fp));
        printf("[DEBUG] footer.eof_marker读取: [%02x %02x %02x %02x] (应为: %02x %02x %02x %02x)\n", \
            footer.eof_marker[0], footer.eof_marker[1], footer.eof_marker[2], footer.eof_marker[3], \
            RDB_EOF_MARKER[0], RDB_EOF_MARKER[1], RDB_EOF_MARKER[2], RDB_EOF_MARKER[3]);
        
        // 计算数据部分的CRC64（不包括对齐填充）
        fseek(fp, sizeof(header), SEEK_SET);
        
        // 读取整个数据部分到内存
        uint8_t *data_buffer = malloc(header.data_size);
        if (!data_buffer) {
            printf("[RDB] 内存分配失败，无法验证CRC64\n");
            fclose(fp);
            return -1;
        }
        
        // 读取数据部分
        if (fread(data_buffer, 1, header.data_size, fp) != header.data_size) {
            printf("[RDB] 数据读取失败，无法验证CRC64\n");
            free(data_buffer);
            fclose(fp);
            return -1;
        }
        
        // 使用rdb_calculate_crc64函数计算CRC64
        uint64_t calculated_crc64 = rdb_calculate_crc64(data_buffer, header.data_size);
        free(data_buffer);
        
        // 验证CRC64
        if (calculated_crc64 != footer.crc64) {
            printf("[RDB] CRC64验证失败\n");
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    printf("[RDB] 文件完整性验证通过\n");
    return 0;
}

// 备份功能

// 创建RDB文件的备份
// 参数: filename - 要备份的文件路径
// 返回值: 0表示成功，-1表示失败
int rdb_create_backup(const char *filename) {
    // 如果备份功能被禁用，直接返回成功
    if (!g_rdb_config.rdb_backup) return 0;

    // 创建备份目录（如果不存在）
    struct stat st = {0};
    if (stat(g_rdb_config.backup_dir, &st) == -1) {
        mkdir(g_rdb_config.backup_dir, 0700);  // 创建目录，权限为700
    }

    // 生成带时间戳的备份文件名
    char backup_file[MAX_PATH_LEN];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    // 检查备份目录长度，确保有足够空间添加时间戳
    size_t dir_len = strlen(g_rdb_config.backup_dir);
    if (dir_len >= MAX_PATH_LEN - 50) {  // 预留50字节给时间戳和文件名
        printf("[RDB] 备份目录路径过长: %s\n", g_rdb_config.backup_dir);
        return -1;
    }
    
    // 使用更安全的格式化方式
    int result = snprintf(backup_file, sizeof(backup_file), 
                         "%s/kvs_backup_%04d%02d%02d_%02d%02d%02d.rdb",
                         g_rdb_config.backup_dir,
                         tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                         tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    // 检查格式化是否成功
    if (result < 0 || result >= sizeof(backup_file)) {
        printf("[RDB] 备份文件名生成失败，路径过长\n");
        return -1;
    }

    // 打开源文件和目标文件
    FILE *src = fopen(filename, "rb");
    if (!src) return -1;

    FILE *dst = fopen(backup_file, "wb");
    if (!dst) {
        fclose(src);
        return -1;
    }

    // 逐块复制文件内容
    uint8_t buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dst);
    }

    fclose(src);
    fclose(dst);

    printf("[RDB] 备份已创建: %s\n", backup_file);
    return 0;
}

// 从备份文件恢复RDB数据
// 参数: backup_file - 备份文件路径
// 返回值: 0表示成功，-1表示失败
int rdb_restore_from_backup(const char *backup_file) {
    if (!backup_file) return -1;

    // 验证备份文件的完整性
    if (rdb_verify_file_integrity(backup_file) != 0) {
        printf("[RDB] 备份文件验证失败\n");
        return -1;
    }

    // 打开备份文件和目标文件
    FILE *src = fopen(backup_file, "rb");
    if (!src) return -1;

    FILE *dst = fopen(g_rdb_config.rdb_file, "wb");
    if (!dst) {
        fclose(src);
        return -1;
    }

    // 逐块复制文件内容
    uint8_t buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dst);
    }

    fclose(src);
    fclose(dst);

    printf("[RDB] 已从备份恢复: %s\n", backup_file);
    return 0;
}

// RDB初始化函数 - 初始化RDB模块，加载配置和数据
// 返回值: 0表示成功，-1表示失败
int kvs_rdb_init(void) {
    printf("[RDB] 初始化RDB模块...\n");
    
    // 初始化配置（加载配置文件或使用默认配置）
    if (rdb_config_init() != 0) {
        printf("[RDB] 配置初始化失败\n");
        return -1;
    }
    
    // 打印当前配置信息
    rdb_config_print();
    
    // 如果RDB功能被禁用，直接返回成功
    if (!g_rdb_config.rdb_enabled) {
        printf("[RDB] RDB功能已禁用\n");
        return 0;
    }
    
    // 尝试加载现有数据文件
    if (access(g_rdb_config.rdb_file, F_OK) == 0) {
        printf("[RDB] 发现现有数据文件，正在加载...\n");
        if (kvs_rdb_load(g_rdb_config.rdb_file) == 0) {
            printf("[RDB] 数据加载成功\n");
        } else {
            printf("[RDB] 数据加载失败\n");
        }
    } else {
        printf("[RDB] 未发现现有数据文件，将创建新文件\n");
    }
    
    printf("[RDB] 初始化完成\n");
    return 0;
}

// 兼容性函数 - 保持向后兼容，提供旧版本API的接口

// 设置RDB文件路径（兼容旧版本API）
// 参数: path - 文件路径
// 返回值: 0表示成功，-1表示失败
int kvs_rdb_set_path(const char *path) {
    return rdb_config_set_rdb_file(path);
}

// 设置备份目录（兼容旧版本API）
// 参数: dir - 备份目录路径
// 返回值: 0表示成功，-1表示失败
int kvs_rdb_set_directory(const char *dir) {
    // 这个函数可以用于设置备份目录
    return rdb_config_set_backup(1, dir);
}

// 设置RDB文件名（兼容旧版本API）
// 参数: filename - 文件名
// 返回值: 0表示成功，-1表示失败
int kvs_rdb_set_filename(const char *filename) {
    return rdb_config_set_rdb_file(filename);
}

// 设置自动保存参数（兼容旧版本API）
// 参数: enabled - 是否启用自动保存, interval - 保存间隔
// 返回值: 0表示成功
int kvs_rdb_set_auto_save(int enabled, int interval) {
    rdb_config_set_enabled(enabled);
    if (interval > 0) {
        rdb_config_set_save_interval(interval);
    }
    return 0;
}

// 获取RDB文件的完整路径（兼容旧版本API）
// 参数: full_path - 输出缓冲区, max_len - 缓冲区最大长度
// 返回值: 0表示成功，-1表示失败
int kvs_rdb_get_full_path(char *full_path, size_t max_len) {
    if (!full_path || max_len == 0) return -1;
    strncpy(full_path, g_rdb_config.rdb_file, max_len - 1);
    full_path[max_len - 1] = '\0';  // 确保字符串结束
    return 0;
}

// 打印RDB配置信息（兼容旧版本API）
void kvs_rdb_print_config(void) {
    rdb_config_print();
}

// 初始化RDB配置（兼容旧版本API）
// 返回值: 0表示成功，-1表示失败
int kvs_rdb_init_config(void) {
    return rdb_config_init();
}

// CRC64计算函数 - 使用ECMA-182标准的多项式
// 参数: data - 要计算CRC64的数据, size - 数据大小
// 返回值: 计算出的CRC64值
uint64_t rdb_calculate_crc64(const void *data, size_t size) {
    uint64_t crc = 0xFFFFFFFFFFFFFFFFULL;
    const uint8_t *bytes = (const uint8_t *)data;
    
    for (size_t i = 0; i < size; i++) {
        crc = crc64_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFFFFFFFFFFULL;
}


