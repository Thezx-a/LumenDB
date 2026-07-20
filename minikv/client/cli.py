import socket
import struct
import sys

MAGIC = 0x4D4B
CMD = {"PUT": 1, "GET": 2, "DEL": 3, "SCAN": 4, "BATCH": 5}
STATUS = {0: "OK", 1: "NOT_FOUND", 2: "ERROR"}

class MiniKVClient:
    def __init__(self, host="127.0.0.1", port=8888):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))

    def _send(self, cmd, key, value=b""):
        hdr = struct.pack("<HBII", MAGIC, cmd, len(key), len(value))
        self.sock.sendall(hdr + key + value)
        resp_hdr = self.sock.recv(9)
        magic, status, vlen = struct.unpack("<HBI", resp_hdr)
        val = b""
        while len(val) < vlen:
            val += self.sock.recv(vlen - len(val))
        return STATUS.get(status, "UNKNOWN"), val.decode()

    def put(self, key, value):
        return self._send(CMD["PUT"], key.encode(), value.encode())
    def get(self, key):
        return self._send(CMD["GET"], key.encode())
    def delete(self, key):
        return self._send(CMD["DEL"], key.encode())

def main():
    if len(sys.argv) >= 2:
        host, port = sys.argv[1].split(":")
        port = int(port)
    else:
        host, port = "127.0.0.1", 8888
    client = MiniKVClient(host, port)
    print(f"Connected to MiniKV at {host}:{port}")
    print("Commands: PUT key value | GET key | DEL key | quit")
    while True:
        try:
            line = input("minikv> ").strip()
            if not line or line == "quit":
                break
            parts = line.split(None, 2)
            cmd = parts[0].upper()
            if cmd == "PUT" and len(parts) == 3:
                status, val = client.put(parts[1], parts[2])
                print(status)
            elif cmd == "GET" and len(parts) == 2:
                status, val = client.get(parts[1])
                print(f"{status}: {val}" if val else status)
            elif cmd == "DEL" and len(parts) == 2:
                status, val = client.delete(parts[1])
                print(status)
            else:
                print("Usage: PUT key value | GET key | DEL key")
        except KeyboardInterrupt:
            break
    client.sock.close()

if __name__ == "__main__":
    main()
