
package main

import (
	"fmt"
	"net"
	"os"
	"time"
)

func main() {
	conn, err := net.Dial("tcp", "192.168.194.133:12345")
	if err != nil {
		fmt.Println("connect failed: ", err)
		os.Exit(1)
	}
	defer conn.Close()


	for i := 0; i < 1000; i++ {
       key := fmt.Sprintf("key%d", i)
	   value := fmt.Sprintf("value%d", i)
	   message := fmt.Sprintf("SET %s %s", key, value)
	   _, err = conn.Write([]byte(message))
	//    time.Sleep(10 * time.Millisecond)//睡眠100毫秒
	   if err != nil {
		fmt.Println("send failed: ", err)
	   }
	   fmt.Printf("send msg: %s\n", message)
	}
	fmt.Println("send done")
	
}




