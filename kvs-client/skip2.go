package main

import (
	"encoding/binary"
	"fmt"
	"net"
	"os"
	"sync"
	"time"
)

const SERVER_ADDR = "127.0.0.1:12345"
const TCP_MAGIC = 0x5453564B
const TCP_VERSION = 1
const TCP_HEADER_SIZE = 24

const (
	CMD_ZADD   = 31
	CMD_ZREM   = 32
	CMD_ZRANGE = 33
	CMD_ZSCORE = 34
	CMD_ZCARD  = 36
	CMD_ZCOUNT = 37
)

type TcpHeader struct {
	Magic   uint32
	Version uint32
	CmdType uint32
	DataLen uint32
	SeqId   uint32
	Flags   uint32
}

func buildTcpPacket(cmdType uint32, seqId uint32, data string) []byte {
	dataLen := len(data)
	packetSize := TCP_HEADER_SIZE + dataLen
	packet := make([]byte, packetSize)
	binary.LittleEndian.PutUint32(packet[0:], TCP_MAGIC)
	binary.LittleEndian.PutUint32(packet[4:], TCP_VERSION)
	binary.LittleEndian.PutUint32(packet[8:], cmdType)
	binary.LittleEndian.PutUint32(packet[12:], uint32(dataLen))
	binary.LittleEndian.PutUint32(packet[16:], seqId)
	binary.LittleEndian.PutUint32(packet[20:], 0)
	if dataLen > 0 {
		copy(packet[TCP_HEADER_SIZE:], []byte(data))
	}
	return packet
}

func parseTcpResponse(conn net.Conn) error {
	headerBuf := make([]byte, TCP_HEADER_SIZE)
	_, err := conn.Read(headerBuf)
	if err != nil {
		return err
	}
	magic := binary.LittleEndian.Uint32(headerBuf[0:])
	if magic != TCP_MAGIC {
		return fmt.Errorf("invalid magic: %x", magic)
	}
	dataLen := binary.LittleEndian.Uint32(headerBuf[12:])
	if dataLen > 0 {
		dataBuf := make([]byte, dataLen)
		_, err = conn.Read(dataBuf)
		if err != nil {
			return err
		}
	}
	return nil
}

func main() {
	conn, err := net.Dial("tcp", SERVER_ADDR)
	if err != nil {
		fmt.Printf("连接失败: %v\n", err)
		os.Exit(1)
	}
	defer conn.Close()

	zsetName := "benchzset"
	memberCount := 10000
	seqId := uint32(1)

	// // 1. 批量ZADD性能测试
	fmt.Printf("=== ZADD 性能测试，插入 %d 个元素 ===\n", memberCount)
	start := time.Now()
	for i := 0; i < memberCount; i++ {
		member := fmt.Sprintf("m%d", i)
		value := fmt.Sprintf("v%d", i)
		score := i
		data := fmt.Sprintf("%s %s %s %d", zsetName, member, value, score)
		packet := buildTcpPacket(CMD_ZADD, seqId, data)
		conn.Write(packet)
		parseTcpResponse(conn) // 忽略响应内容
		seqId++
	}
	elapsed := time.Since(start)
	fmt.Printf("ZADD 总耗时: %v, QPS: %.2f\n", elapsed, float64(memberCount)/elapsed.Seconds())
	

	///
	// zsetName2 := "benchzset2"
	// memberCount2 := 10000
	// seqId2 := uint32(1)

	// // // 1. 批量ZADD性能测试
	// fmt.Printf("=== ZADD 性能测试，插入 %d 个元素 ===\n", memberCount2)
	// start2 := time.Now()
	// for i := 0; i < memberCount; i++ {
	// 	member2 := fmt.Sprintf("m%d", i)
	// 	value2 := fmt.Sprintf("v%d", i)
	// 	score2 := i
	// 	data := fmt.Sprintf("%s %s %s %d", zsetName2, member2, value2, score2)
	// 	packet := buildTcpPacket(CMD_ZADD, seqId2, data)
	// 	conn.Write(packet)
	// 	parseTcpResponse(conn) // 忽略响应内容
	// 	seqId++
	// }
	// elapsed2 := time.Since(start2)
	// fmt.Printf("ZADD 总耗时: %v, QPS: %.2f\n", elapsed, float64(memberCount2)/elapsed2.Seconds())

	// 2. ZRANGE 性能测试
	fmt.Println("=== ZRANGE 性能测试 ===")
	zrangeData := fmt.Sprintf("%s %d", zsetName, 0)
	packet := buildTcpPacket(CMD_ZRANGE, seqId, zrangeData+" 9999")
	start = time.Now()
	conn.Write(packet)
	parseTcpResponse(conn)
	elapsed = time.Since(start)
	fmt.Printf("ZRANGE 查询耗时: %v\n", elapsed)

	// 3. 并发ZADD测试
	fmt.Println("=== 并发ZADD 性能测试 ===")
	concurrent := 10
	perThread := 1000
	var wg sync.WaitGroup
	start = time.Now()
	for t := 0; t < concurrent; t++ {
		wg.Add(1)
		go func(threadId int) {
			defer wg.Done()
			c, err := net.Dial("tcp", SERVER_ADDR)
			if err != nil {
				fmt.Printf("线程%d连接失败: %v\n", threadId, err)
				return
			}
			defer c.Close()
			localSeq := uint32(threadId * perThread)
			for i := 0; i < perThread; i++ {
				member := fmt.Sprintf("cm%d_%d", threadId, i)
				value := fmt.Sprintf("v%d", i)
				score := i
				data := fmt.Sprintf("%s %s %s %d", zsetName, member, value, score)
				packet := buildTcpPacket(CMD_ZADD, localSeq, data)
				c.Write(packet)
				parseTcpResponse(c)
				localSeq++
			}
		}(t)
	}
	wg.Wait()
	elapsed = time.Since(start)
	fmt.Printf("并发ZADD 总耗时: %v, QPS: %.2f\n", elapsed, float64(concurrent*perThread)/elapsed.Seconds())
} 