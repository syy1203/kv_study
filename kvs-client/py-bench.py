import socket
import threading
import time

SERVER_IP = '127.0.0.1'  # 根据实际情况修改
SERVER_PORT = 2000       # 根据实际情况修改
THREADS = 20             # 并发线程数
REQUESTS_PER_THREAD = 500  # 每个线程请求数


def worker(tid):
    for i in range(REQUESTS_PER_THREAD):
        try:
            s = socket.socket()
            s.connect((SERVER_IP, SERVER_PORT))
            key = f"key{tid}_{i}"
            value = f"val{tid}_{i}"
            # 发送SET命令
            s.sendall(f"SET {key} {value}\r\n".encode())
            s.recv(1024)
            # 发送GET命令
            s.sendall(f"GET {key}\r\n".encode())
            s.recv(1024)
            s.close()
        except Exception as e:
            print(f"Thread {tid} error: {e}")

def main():
    threads = []
    start = time.time()
    for t in range(THREADS):
        th = threading.Thread(target=worker, args=(t,))
        th.start()
        threads.append(th)
    for th in threads:
        th.join()
    end = time.time()
    print(f"压测完成，总耗时: {end - start:.2f} 秒")

if __name__ == '__main__':
    main() 