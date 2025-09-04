package main

import (
	"bufio"
	"encoding/binary"
	"fmt"
	"net"
	"os"
	"strings"
	"time"
)

const SERVER_ADDR = "127.0.0.1:12345"
const TCP_MAGIC = 0x5453564B
const TCP_VERSION = 1
const TCP_HEADER_SIZE = 24

// 命令类型
const (
	CMD_SET = 1
	CMD_GET = 2
	CMD_DEL = 3
	CMD_MOD = 4
	CMD_EXIST = 5
	CMD_SAVE = 6
	CMD_HSET = 21
	CMD_HGET = 22
)

type TcpClient struct {
	conn   net.Conn
	seqId  uint32
	reader *bufio.Reader
}

func NewTcpClient(addr string) (*TcpClient, error) {
	conn, err := net.Dial("tcp", addr)
	if err != nil {
		return nil, err
	}
	
	return &TcpClient{
		conn:   conn,
		seqId:  1,
		reader: bufio.NewReader(conn),
	}, nil
}

func (c *TcpClient) Close() {
	if c.conn != nil {
		c.conn.Close()
	}
}

func (c *TcpClient) buildPacket(cmdType uint32, data string) []byte {
	dataLen := len(data)
	packetSize := TCP_HEADER_SIZE + dataLen
	packet := make([]byte, packetSize)
	
	// 写入头部
	binary.LittleEndian.PutUint32(packet[0:], TCP_MAGIC)
	binary.LittleEndian.PutUint32(packet[4:], TCP_VERSION)
	binary.LittleEndian.PutUint32(packet[8:], cmdType)
	binary.LittleEndian.PutUint32(packet[12:], uint32(dataLen))
	binary.LittleEndian.PutUint32(packet[16:], c.seqId)
	binary.LittleEndian.PutUint32(packet[20:], 0)
	
	// 写入数据
	if dataLen > 0 {
		copy(packet[TCP_HEADER_SIZE:], []byte(data))
	}
	
	return packet
}

func (c *TcpClient) parseResponse() (string, error) {
	// 读取头部
	headerBuf := make([]byte, TCP_HEADER_SIZE)
	_, err := c.reader.Read(headerBuf)
	if err != nil {
		return "", err
	}
	
	magic := binary.LittleEndian.Uint32(headerBuf[0:])
	if magic != TCP_MAGIC {
		return "", fmt.Errorf("invalid magic: %x", magic)
	}
	
	dataLen := binary.LittleEndian.Uint32(headerBuf[12:])
	
	// 读取数据
	if dataLen > 0 {
		dataBuf := make([]byte, dataLen)
		_, err = c.reader.Read(dataBuf)
		if err != nil {
			return "", err
		}
		return string(dataBuf), nil
	}
	
	return "", nil
}

func (c *TcpClient) SendCommand(cmdType uint32, data string) (string, error) {
	packet := c.buildPacket(cmdType, data)
	
	_, err := c.conn.Write(packet)
	if err != nil {
		return "", err
	}
	
	response, err := c.parseResponse()
	if err != nil {
		return "", err
	}
	
	c.seqId++
	return response, nil
}

func (c *TcpClient) Set(key, value string) (string, error) {
	data := fmt.Sprintf("%s %s", key, value)
	return c.SendCommand(CMD_SET, data)
}

func (c *TcpClient) Get(key string) (string, error) {
	return c.SendCommand(CMD_GET, key)
}

func (c *TcpClient) Del(key string) (string, error) {
	return c.SendCommand(CMD_DEL, key)
}

func (c *TcpClient) Save() (string, error) {
	return c.SendCommand(CMD_SAVE, "")
}

func (c *TcpClient) BatchSet(count int) error {
	fmt.Printf("开始批量SET %d 个键值对...\n", count)
	start := time.Now()
	
	for i := 1; i <= count; i++ {
		key := fmt.Sprintf("batch_key_%d", i)
		value := fmt.Sprintf("batch_value_%d", i)
		
		response, err := c.Set(key, value)
		if err != nil {
			return fmt.Errorf("SET %s 失败: %v", key, err)
		}
		
		if i%100 == 0 {
			fmt.Printf("已处理 %d 个SET操作\n", i)
		}
	}
	
	duration := time.Since(start)
	fmt.Printf("批量SET完成，耗时: %v, 平均: %v/个\n", duration, duration/time.Duration(count))
	return nil
}

func (c *TcpClient) PerformanceTest() error {
	fmt.Println("\n=== 性能测试 ===")
	
	// SET性能测试
	fmt.Println("1. SET性能测试")
	start := time.Now()
	for i := 1; i <= 1000; i++ {
		key := fmt.Sprintf("perf_key_%d", i)
		value := fmt.Sprintf("perf_value_%d", i)
		_, err := c.Set(key, value)
		if err != nil {
			return err
		}
	}
	setDuration := time.Since(start)
	fmt.Printf("1000次SET耗时: %v, 平均: %v/次\n", setDuration, setDuration/1000)
	
	// GET性能测试
	fmt.Println("2. GET性能测试")
	start = time.Now()
	for i := 1; i <= 1000; i++ {
		key := fmt.Sprintf("perf_key_%d", i)
		_, err := c.Get(key)
		if err != nil {
			return err
		}
	}
	getDuration := time.Since(start)
	fmt.Printf("1000次GET耗时: %v, 平均: %v/次\n", getDuration, getDuration/1000)
	
	return nil
}

func (c *TcpClient) InteractiveMode() {
	fmt.Println("进入交互模式，输入 'help' 查看命令，输入 'quit' 退出")
	
	scanner := bufio.NewScanner(os.Stdin)
	for {
		fmt.Print("kvstore> ")
		if !scanner.Scan() {
			break
		}
		
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}
		
		if line == "quit" || line == "exit" {
			break
		}
		
		if line == "help" {
			fmt.Println("可用命令:")
			fmt.Println("  set <key> <value>  - 设置键值对")
			fmt.Println("  get <key>          - 获取值")
			fmt.Println("  del <key>          - 删除键")
			fmt.Println("  save               - 保存数据")
			fmt.Println("  quit/exit          - 退出")
			continue
		}
		
		parts := strings.Fields(line)
		if len(parts) == 0 {
			continue
		}
		
		cmd := parts[0]
		var response string
		var err error
		
		switch cmd {
		case "set":
			if len(parts) < 3 {
				fmt.Println("用法: set <key> <value>")
				continue
			}
			response, err = c.Set(parts[1], parts[2])
		case "get":
			if len(parts) < 2 {
				fmt.Println("用法: get <key>")
				continue
			}
			response, err = c.Get(parts[1])
		case "del":
			if len(parts) < 2 {
				fmt.Println("用法: del <key>")
				continue
			}
			response, err = c.Del(parts[1])
		case "save":
			response, err = c.Save()
		default:
			fmt.Printf("未知命令: %s\n", cmd)
			continue
		}
		
		if err != nil {
			fmt.Printf("错误: %v\n", err)
		} else {
			fmt.Printf("响应: %s\n", response)
		}
	}
}

func main() {
	client, err := NewTcpClient(SERVER_ADDR)
	if err != nil {
		fmt.Printf("连接失败: %v\n", err)
		os.Exit(1)
	}
	defer client.Close()
	
	fmt.Printf("连接到服务器 %s 成功\n", SERVER_ADDR)
	
	// 基本测试
	fmt.Println("\n=== 基本功能测试 ===")
	
	response, err := client.Set("testkey", "testvalue")
	if err != nil {
		fmt.Printf("SET失败: %v\n", err)
	} else {
		fmt.Printf("SET响应: %s\n", response)
	}
	
	response, err = client.Get("testkey")
	if err != nil {
		fmt.Printf("GET失败: %v\n", err)
	} else {
		fmt.Printf("GET响应: %s\n", response)
	}
	
	// 批量测试
	fmt.Println("\n=== 批量操作测试 ===")
	err = client.BatchSet(100)
	if err != nil {
		fmt.Printf("批量SET失败: %v\n", err)
	}
	
	// 性能测试
	err = client.PerformanceTest()
	if err != nil {
		fmt.Printf("性能测试失败: %v\n", err)
	}
	
	// 交互模式
	fmt.Println("\n=== 交互模式 ===")
	client.InteractiveMode()
	
	fmt.Println("客户端退出")
} 