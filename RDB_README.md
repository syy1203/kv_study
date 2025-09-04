# KVS RDB 持久化模块

## 概述

KVS RDB模块提供了强大的数据持久化功能，包括配置管理、文件完整性校验、自动备份等特性。

## 主要功能

### 1. 配置管理

RDB模块支持通过配置文件或API调用来控制其行为：

#### 配置文件格式 (kvs.conf)
```ini
# 自动保存间隔（秒）
save_interval = 30

# RDB 文件路径
rdb_file = kvs.rdb

# 是否启用 RDB
rdb_enabled = yes

# 是否启用压缩（可选）
rdb_compression = no

# 是否启用CRC64校验
rdb_crc64 = yes

# 是否启用备份
rdb_backup = no

# 备份目录
backup_dir = ./backup
```

#### 配置API
```c
// 设置自动保存间隔
int rdb_config_set_save_interval(int interval);

// 设置RDB文件路径
int rdb_config_set_rdb_file(const char *file);

// 启用/禁用RDB
int rdb_config_set_enabled(int enabled);

// 启用/禁用压缩
int rdb_config_set_compression(int enabled);

// 启用/禁用CRC64校验
int rdb_config_set_crc64(int enabled);

// 启用/禁用备份
int rdb_config_set_backup(int enabled, const char *backup_dir);

// 打印当前配置
void rdb_config_print(void);
```

### 2. 文件完整性校验

RDB模块提供了多层文件完整性验证机制：

#### 文件头结构
```c
typedef struct {
    uint32_t magic;        // 魔数 "RDBK"
    uint32_t version;      // 版本号
    uint32_t flags;        // 标志位
    uint32_t data_size;    // 数据大小
    uint32_t record_count; // 记录数
    time_t save_time;      // 保存时间
    uint32_t alignment_padding; // 对齐填充
} rdb_header_t;
```

#### 校验功能
- **魔数验证**: 确保文件格式正确
- **版本兼容性检查**: 确保文件版本兼容
- **CRC64校验验证**: 可选的数据完整性校验
- **文件完整性验证**: 综合验证文件完整性

```c
// 验证文件完整性
int rdb_verify_file_integrity(const char *filename);
```

### 3. 自动备份功能

RDB模块支持自动备份功能，在每次保存前自动创建备份：

#### 备份特性
- 自动创建备份目录
- 时间戳命名备份文件
- 备份文件完整性验证
- 支持从备份恢复

```c
// 创建备份
int rdb_create_backup(const char *filename);

// 从备份恢复
int rdb_restore_from_backup(const char *backup_file);
```

### 4. 使用示例

#### 基本使用
```c
// 初始化RDB模块
if (kvs_rdb_init() != 0) {
    printf("RDB初始化失败\n");
    return -1;
}

// 保存数据
if (kvs_rdb_save("data.rdb") == 0) {
    printf("数据保存成功\n");
}

// 加载数据
if (kvs_rdb_load("data.rdb") == 0) {
    printf("数据加载成功\n");
}
```

#### 配置管理
```c
// 加载配置文件
rdb_config_load("kvs.conf");

// 修改配置
rdb_config_set_save_interval(60);
rdb_config_set_rdb_file("custom.rdb");
rdb_config_set_backup(1, "./backups");

// 保存配置
rdb_config_save("kvs.conf");
```

#### 完整性验证
```c
// 验证文件完整性
if (rdb_verify_file_integrity("data.rdb") == 0) {
    printf("文件完整性验证通过\n");
} else {
    printf("文件完整性验证失败\n");
    
    // 尝试从备份恢复
    if (rdb_restore_from_backup("backup_file.rdb") == 0) {
        printf("从备份恢复成功\n");
    }
}
```

## 编译和测试

### 编译
```bash
make clean
make all
```

### 运行测试
```bash
# 运行RDB功能测试
./test_rdb

# 运行主程序
./kvstore 8080
```

### 测试程序
`test_rdb.c` 提供了完整的RDB功能测试，包括：
- 配置初始化测试
- 配置修改测试
- 配置保存测试
- RDB初始化测试
- 数据保存测试
- 文件完整性验证测试
- 备份功能测试

## 文件格式

### RDB文件结构
```
[文件头] (32字节)
├── 魔数 (4字节)
├── 版本号 (4字节)
├── 标志位 (4字节)
├── 校验和 (4字节)
├── 数据大小 (4字节)
├── 记录数量 (4字节)
├── 保存时间 (8字节)
└── 对齐填充 (4字节)

[数据部分]
├── 记录1
│   ├── 类型 (1字节)
│   ├── 键长度 (2字节)
│   ├── 键内容
│   ├── 值长度 (2字节)
│   └── 值内容
├── 记录2
└── ...

[对齐填充] (0-7字节)
└── 填充字节

[文件尾] (24字节)
├── EOF标记 (4字节)
├── CRC64校验和 (8字节)
├── 文件尾大小 (4字节)
└── 对齐填充 (8字节)
```

## 错误处理

RDB模块提供了详细的错误处理和日志输出：

- 文件打开失败
- 文件格式错误
- 版本不兼容
- 校验和验证失败
- 内存分配失败
- 配置加载失败

## 性能考虑

- CRC64计算使用标准算法，对性能影响较小
- 备份功能在保存前执行，不影响正常操作
- 配置加载只在启动时执行一次
- 文件完整性验证在加载时执行，确保数据安全

## 注意事项

1. 确保备份目录有足够的磁盘空间
2. 定期清理旧的备份文件
3. 在生产环境中建议启用CRC64校验功能
4. 根据数据量调整自动保存间隔
5. 定期验证备份文件的完整性 