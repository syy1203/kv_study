package main

import (
	"bytes"
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
const MAX_PACKET_SIZE = 4096 // 包含头部的最大包长

const (
	CMD_ZADD = 31
    CMD_ZREM = 32
    CMD_ZRANGE = 33
)

type TcpHeader struct {
	Magic   uint32
	Version uint32
	CmdType uint32
	DataLen uint32
	SeqId   uint32
	Flags   uint32
}

// 安全读取 n 字节
func readN(conn net.Conn, buf []byte) error {
    total := 0
    for total < len(buf) {
        n, err := conn.Read(buf[total:])
        if err != nil {
            return err
        }
        total += n
    }
    return nil
}
// SendPipeline sends a pipeline request and returns responses.
func SendPipeline(conn net.Conn, cmdType uint32, seqId uint32, argsList [][]string) ([]string, error) {
	var allResponses []string
	var cmds []string
	for _, args := range argsList {
		cmds = append(cmds, joinArgs(args))
	}

	// 分包逻辑
	var curCmds []string
	curSize := TCP_HEADER_SIZE
	curSeq := seqId
	for i, cmd := range cmds {
		cmdLen := 4 + len([]byte(cmd))
		if curSize+cmdLen > MAX_PACKET_SIZE {
			// 发送当前包
			packet := buildPipelinePacket(cmdType, curSeq, curCmds)
			if _, err := conn.Write(packet); err != nil {
				return nil, err
			}
			responses, err := parsePipelineResponse(conn)
			if err != nil {
				return nil, err
			}
			allResponses = append(allResponses, responses...)
			// 新包
			curCmds = nil
			curSize = TCP_HEADER_SIZE
			curSeq = seqId + uint32(i)
		}
		curCmds = append(curCmds, cmd)
		curSize += cmdLen
	}
	if len(curCmds) > 0 {
		packet := buildPipelinePacket(cmdType, curSeq, curCmds)
		if _, err := conn.Write(packet); err != nil {
			return nil, err
		}
		responses, err := parsePipelineResponse(conn)
		if err != nil {
			return nil, err
		}
		allResponses = append(allResponses, responses...)
	}
	return allResponses, nil
}

// joinArgs joins command arguments with space.
func joinArgs(args []string) string {
	return fmt.Sprintf("%s", joinWithSpace(args))
}

func joinWithSpace(args []string) string {
	return fmt.Sprint(bytes.Join(stringSliceToByteSlice(args), []byte(" ")))
}

func stringSliceToByteSlice(strs []string) [][]byte {
	var result [][]byte
	for _, s := range strs {
		result = append(result, []byte(s))
	}
	return result
}



// pipeline协议打包
func buildPipelinePacket(cmdType uint32, seqId uint32, cmds []string) []byte {
	var body bytes.Buffer
	for _, cmd := range cmds {
		data := []byte(cmd)
		binary.Write(&body, binary.LittleEndian, uint32(len(data))) // 4字节长度（小端）
		body.Write(data)
	}
	bodyBytes := body.Bytes()
	packetSize := TCP_HEADER_SIZE + len(bodyBytes)
	packet := make([]byte, packetSize)
	binary.LittleEndian.PutUint32(packet[0:], TCP_MAGIC)
	fmt.Printf("MAGIC (hex): 0x%08X\n", binary.LittleEndian.Uint32(packet[0:4]))
	binary.LittleEndian.PutUint32(packet[4:], TCP_VERSION)
	binary.LittleEndian.PutUint32(packet[8:], cmdType)
	binary.LittleEndian.PutUint32(packet[12:], uint32(len(bodyBytes)))
	binary.LittleEndian.PutUint32(packet[16:], seqId)
	binary.LittleEndian.PutUint32(packet[20:], 0)
	copy(packet[TCP_HEADER_SIZE:], bodyBytes)
	return packet
}

// pipeline响应解析
func parsePipelineResponse(conn net.Conn) ([]string, error) {
	headerBuf := make([]byte, TCP_HEADER_SIZE)
	if err := readN(conn, headerBuf); err != nil {
		return nil, err
	}
	magic := binary.LittleEndian.Uint32(headerBuf[0:])
	if magic != TCP_MAGIC {
		return nil, fmt.Errorf("invalid magic: %x", magic)
	}
	dataLen := binary.LittleEndian.Uint32(headerBuf[12:])
	if dataLen == 0 {
		return nil, nil
	}
	dataBuf := make([]byte, dataLen)
	if err := readN(conn, dataBuf); err != nil {
		return nil, err
	}
	// 解析多条响应
	var results []string
	offset := 0
	for offset+4 <= len(dataBuf) {
		respLen := int(binary.LittleEndian.Uint32(dataBuf[offset : offset+4]))
		offset += 4
		if offset+respLen > len(dataBuf) {
			break // 数据不完整
		}
		resp := string(dataBuf[offset : offset+respLen])
		results = append(results, resp)
		offset += respLen
	}
	return results, nil
}


func pipelineZaddTest(conn net.Conn, zsetName string, startSeq uint32, count int) {
	var argsList [][]string
	for i := 0; i < count; i++ {
		member := fmt.Sprintf("pm%d", i)
		value := fmt.Sprintf("pv%d", i)
		score := fmt.Sprintf("%d", i)
		argsList = append(argsList, []string{zsetName, member, value, score})
	}
	responses, err := SendPipeline(conn, CMD_ZADD, startSeq, argsList)
	if err != nil {
		fmt.Println("解析失败:", err)
		return
	}
	for i, resp := range responses {
		fmt.Printf("第%d条响应: %s\n", i+1, resp)
	}
}

// pipeline ZRANGE 测试函数
func pipelineZrangeTest(conn net.Conn, zsetName string, startSeq uint32, count int, startIdx, endIdx int) {
	var argsList [][]string
	for i := 0; i < count; i++ {
		argsList = append(argsList, []string{
			zsetName,
			fmt.Sprintf("%d", startIdx),
			fmt.Sprintf("%d", endIdx),
		})
	}
	responses, err := SendPipeline(conn, CMD_ZRANGE, startSeq, argsList)
	if err != nil {
		fmt.Println("解析失败:", err)
		return
	}
	for i, resp := range responses {
		fmt.Printf("第%d条响应: %s\n", i+1, resp)
	}
}

func concurrentPipelineZaddTest(concurrent, perThread int, zsetName string) {
	var wg sync.WaitGroup
	start := time.Now()

	for t := 0; t < concurrent; t++ {
		wg.Add(1)
		go func(threadId int) {
			defer wg.Done()
			conn, err := net.Dial("tcp", SERVER_ADDR)
			if err != nil {
				fmt.Printf("线程 %d 连接失败: %v\n", threadId, err)
				return
			}
			defer conn.Close()

			var argsList [][]string
			for i := 0; i < perThread; i++ {
				member := fmt.Sprintf("cm%d_%d", threadId, i)
				value := fmt.Sprintf("v%d", i)
				score := fmt.Sprintf("%d", i)
				argsList = append(argsList, []string{zsetName, member, value, score})
			}

			_, err = SendPipeline(conn, CMD_ZADD, uint32(threadId*perThread), argsList)
			if err != nil {
				fmt.Printf("线程 %d 发送失败: %v\n", threadId, err)
			}
		}(t)
	}
	wg.Wait()
	elapsed := time.Since(start)
	fmt.Printf("并发 pipeline ZADD 总耗时: %v, QPS: %.2f\n", elapsed, float64(concurrent*perThread)/elapsed.Seconds())
}
func main() {
	conn, err := net.Dial("tcp", SERVER_ADDR)
	if err != nil {
		fmt.Printf("连接失败: %v\n", err)
		os.Exit(1)
	}
	defer conn.Close()
    

    zsetName := "singlezsetbanji"
	pipelineCount := 10
    startSeq := uint32(1000)

    fmt.Println("===== pipeline ZADD 测试 =====")
    pipelineZaddTest(conn, zsetName, startSeq, pipelineCount)

    fmt.Println("===== pipeline ZRANGE 测试 =====")
    pipelineZrangeTest(conn, zsetName, startSeq+uint32(pipelineCount), pipelineCount, 0, 5)

    // // 并发性能测试
    // fmt.Println("===== 并发 ZADD 测试 =====")
    // concurrentPipelineZaddTest(10, 100, "concurrent_zset")
} 