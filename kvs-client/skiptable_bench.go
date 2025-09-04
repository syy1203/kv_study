package main

import (
	"encoding/binary"
	"fmt"
	"net"
	"os"
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

// 单包打包（大端序）
func buildTcpPacket(cmdType uint32, seqId uint32, data string) []byte {
    dataLen := len(data)
    packetSize := TCP_HEADER_SIZE + dataLen
    packet := make([]byte, packetSize)
    binary.BigEndian.PutUint32(packet[0:], TCP_MAGIC)
    binary.BigEndian.PutUint32(packet[4:], TCP_VERSION)
    binary.BigEndian.PutUint32(packet[8:], cmdType)
    binary.BigEndian.PutUint32(packet[12:], uint32(dataLen))
    binary.BigEndian.PutUint32(packet[16:], seqId)
    binary.BigEndian.PutUint32(packet[20:], 0)
    if dataLen > 0 {
        copy(packet[TCP_HEADER_SIZE:], []byte(data))
    }
    return packet
}

// 单包响应解析（大端序）
func parseTcpResponse(conn net.Conn) (string, error) {
    headerBuf := make([]byte, TCP_HEADER_SIZE)
    if err := readN(conn, headerBuf); err != nil {
        return "", err
    }
    magic := binary.BigEndian.Uint32(headerBuf[0:])
    if magic != TCP_MAGIC {
        return "", fmt.Errorf("invalid magic: %x", magic)
    }
    dataLen := binary.BigEndian.Uint32(headerBuf[12:])
    if dataLen > 0 {
        dataBuf := make([]byte, dataLen)
        if err := readN(conn, dataBuf); err != nil {
            return "", err
        }
        return string(dataBuf), nil
    }
    return "", nil
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

// 单次ZADD测试
func singleZaddTest(conn net.Conn, zsetName string, seqId uint32, member, value string, score int) {
    data := fmt.Sprintf("%s %s %s %d", zsetName, member, value, score)
    packet := buildTcpPacket(CMD_ZADD, seqId, data)
    conn.Write(packet)
    resp, err := parseTcpResponse(conn)
    if err != nil {
        fmt.Printf("ZADD解析失败: %v\n", err)
    } else {
        fmt.Printf("ZADD响应: %s\n", resp)
    }
}

// 单次ZRANGE测试
func singleZrangeTest(conn net.Conn, zsetName string, seqId uint32, startIdx, endIdx int) {
    data := fmt.Sprintf("%s %d %d", zsetName, startIdx, endIdx)
    packet := buildTcpPacket(CMD_ZRANGE, seqId, data)
    conn.Write(packet)
    resp, err := parseTcpResponse(conn)
    if err != nil {
        fmt.Printf("ZRANGE解析失败: %v\n", err)
    } else {
        fmt.Printf("ZRANGE响应: %s\n", resp)
    }
}

func main() {
    conn, err := net.Dial("tcp", SERVER_ADDR)
    if err != nil {
        fmt.Printf("连接失败: %v\n", err)
        os.Exit(1)
    }
    defer conn.Close()

    zsetName := "singlezsetbanji"
    seqId := uint32(1)

    fmt.Println("=== 单次ZADD测试 ===")
    singleZaddTest(conn, zsetName, seqId, "stu1", "99分", 99)
    seqId++

    fmt.Println("=== 单次ZRANGE测试 ===")
    singleZrangeTest(conn, zsetName, seqId, 60, 100)
} 