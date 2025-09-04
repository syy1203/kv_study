package main

import (
	"fmt"
	"net"
	"sync"
	"time"
)

const (
	SERVER_IP   = "127.0.0.1" // 根据实际情况修改
	SERVER_PORT = 12345        // 根据实际情况修改
	THREADS     = 2           // 并发协程数
	REQUESTS_PER_THREAD = 500  // 每个协程请求数
)

func worker(tid int, wg *sync.WaitGroup) {
	defer wg.Done()
	for i := 0; i < REQUESTS_PER_THREAD; i++ {
		conn, err := net.Dial("tcp", fmt.Sprintf("%s:%d", SERVER_IP, SERVER_PORT))
		if err != nil {
			fmt.Printf("Goroutine %d connect error: %v\n", tid, err)
			continue
		}
		key := fmt.Sprintf("key%d_%d", tid, i)
		value := fmt.Sprintf("val%d_%d", tid, i)
		// 发送SET命令
		fmt.Fprintf(conn, "SET %s %s\r\n", key, value)
		buf := make([]byte, 1024)
		conn.Read(buf)
		// 发送GET命令
		fmt.Fprintf(conn, "GET %s\r\n", key)
		conn.Read(buf)
		conn.Close()
	}
}

func main() {
	var wg sync.WaitGroup
	start := time.Now()
	for t := 0; t < THREADS; t++ {
		wg.Add(1)
		go worker(t, &wg)
	}
	wg.Wait()
	end := time.Now()
	fmt.Printf("压测完成，总耗时: %.2f 秒\n", end.Sub(start).Seconds())
} 