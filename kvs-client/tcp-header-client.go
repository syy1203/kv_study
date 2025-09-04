package main

import (
	"encoding/binary"
	"fmt"
	"net"
	"os"
)

const SERVER_ADDR = "127.0.0.1:12345"

// 协议头定义（与服务端保持一致）
const (
	TCP_MAGIC   = 0x5453564B // "KVST"
	TCP_VERSION = 1
	TCP_HEADER_SIZE = 24
)

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

type TcpHeader struct {
	Magic   uint32
	Version uint32
	CmdType uint32
	DataLen uint32
	SeqId   uint32
	Flags   uint32
}

// 构建带协议头的TCP数据包
func buildTcpPacket(cmdType uint32, seqId uint32, data string) []byte {
	dataLen := len(data)
	packetSize := TCP_HEADER_SIZE + dataLen




	
	packet := make([]byte, packetSize)
	
	// 构建头部
	header := TcpHeader{
		Magic:   TCP_MAGIC,
		Version: TCP_VERSION,
		CmdType: cmdType,
		DataLen: uint32(dataLen),
		SeqId:   seqId,
		Flags:   0,
	}
	
	// 写入头部
	binary.LittleEndian.PutUint32(packet[0:], header.Magic)
	binary.LittleEndian.PutUint32(packet[4:], header.Version)
	binary.LittleEndian.PutUint32(packet[8:], header.CmdType)
	binary.LittleEndian.PutUint32(packet[12:], header.DataLen)
	binary.LittleEndian.PutUint32(packet[16:], header.SeqId)
	binary.LittleEndian.PutUint32(packet[20:], header.Flags)
	
	// 写入数据
	if dataLen > 0 {
		copy(packet[TCP_HEADER_SIZE:], []byte(data))
	}
	
	return packet
}

// 解析TCP响应
func parseTcpResponse(conn net.Conn) (string, error) {
	// 读取头部
	headerBuf := make([]byte, TCP_HEADER_SIZE)
	_, err := conn.Read(headerBuf)
	if err != nil {
		return "", err
	}
	
	// 解析头部
	magic := binary.LittleEndian.Uint32(headerBuf[0:])
	if magic != TCP_MAGIC {
		return "", fmt.Errorf("invalid magic: %x", magic)
	}
	
	dataLen := binary.LittleEndian.Uint32(headerBuf[12:])
	
	// 读取数据
	if dataLen > 0 {
		dataBuf := make([]byte, dataLen)
		_, err = conn.Read(dataBuf)
		if err != nil {
			return "", err
		}
		return string(dataBuf), nil
	}
	
	return "", nil
}

func main() {
	conn, err := net.Dial("tcp", SERVER_ADDR)
	if err != nil {
		fmt.Printf("连接失败: %v\n", err)
		os.Exit(1)
	}
	defer conn.Close()
	
	seqId := uint32(1)
	
	// 测试SET命令
	fmt.Println("=== 测试SET命令 ===")
	setData := "testkey2 testvalue2"
	setPacket := buildTcpPacket(CMD_SET, seqId, setData)
	fmt.Printf("发送SET命令，数据: %s\n", setData)
	conn.Write(setPacket)
	
	response, err := parseTcpResponse(conn)
	if err != nil {
		fmt.Printf("解析响应失败: %v\n", err)
	} else {
		fmt.Printf("SET响应: %s\n", response)
	}
	seqId++
	
	// 测试GET命令
	fmt.Println("\n=== 测试GET命令 ===")
	getData := "testkey2"
	getPacket := buildTcpPacket(CMD_GET, seqId, getData)
	fmt.Printf("发送GET命令，数据: %s\n", getData)
	conn.Write(getPacket)
	
	response, err = parseTcpResponse(conn)
	if err != nil {
		fmt.Printf("解析响应失败: %v\n", err)
	} else {
		fmt.Printf("GET响应: %s\n", response)
	}
	seqId++
	
	//测试批量SET
	// fmt.Println("\n=== 测试批量SET ===")
	// for i := 101; i <= 200; i++ {
	// 	key := fmt.Sprintf("key%d", i)
	// 	value := fmt.Sprintf("value%d", i)
	// 	data := fmt.Sprintf("%s %s", key, value)
		
	// 	packet := buildTcpPacket(CMD_SET, seqId, data)
	// 	conn.Write(packet)
		
	// 	response, err := parseTcpResponse(conn)
	// 	if err != nil {
	// 		fmt.Printf("SET %s 失败: %v\n", key, err)
	// 	} else {
	// 		fmt.Printf("SET %s: %s\n", key, response)
	// 	}
	// 	seqId++
	// }
	// // 测试批量GET
	fmt.Println("\n=== 测试批量GET ===")
	for i := 101; i <= 200; i++ {
		key := fmt.Sprintf("key%d", i)
		
		packet := buildTcpPacket(CMD_GET, seqId, key)
		conn.Write(packet)
		
		response, err := parseTcpResponse(conn)
		if err != nil {
			fmt.Printf("GET %s 失败: %v\n", key, err)
		} else {
			fmt.Printf("GET %s: %s\n", key, response)
		}
		seqId++
	}
	
	
	// 测试SAVE命令
	fmt.Println("\n=== 测试SAVE命令 ===")
	savePacket := buildTcpPacket(CMD_SAVE, seqId, "")
	conn.Write(savePacket)
	
	response, err = parseTcpResponse(conn)
	if err != nil {
		fmt.Printf("SAVE失败: %v\n", err)
	} else {
		fmt.Printf("SAVE响应: %s\n", response)
	}
	
	fmt.Println("\n测试完成")
} 