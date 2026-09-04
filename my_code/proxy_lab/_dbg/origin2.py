import socket, sys, threading

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 18081
LOG = "/tmp/origin2_log.txt"

BODY = {
    "/small": b"CONTENT-FROM-PORT2 **different**\n",
}

def handle(c):
    data = b""
    c.settimeout(3)
    try:
        while b"\r\n\r\n" not in data and len(data) < 65536:
            chunk = c.recv(4096)
            if not chunk:
                break
            data += chunk
    except socket.timeout:
        pass
    line = data.split(b"\r\n")[0].decode("latin1", "replace")
    with open(LOG, "a") as f:
        f.write(line + "\n")
    parts = line.split()
    path = parts[1] if len(parts) >= 2 else "/"
    key = path.split("?")[0]
    body = BODY.get(key, b"nf2\n")
    status = "200 OK" if key in BODY else "404 Not Found"
    resp = ("HTTP/1.0 %s\r\nServer: test-origin2\r\nContent-Length: %d\r\nConnection: close\r\n\r\n"
            % (status, len(body))).encode() + body
    try:
        c.sendall(resp)
    except Exception:
        pass
    c.close()

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("127.0.0.1", PORT))
    s.listen(64)
    open(LOG, "w").close()
    print("origin2 listening on", PORT, flush=True)
    while True:
        c, _ = s.accept()
        threading.Thread(target=handle, args=(c,), daemon=True).start()

if __name__ == "__main__":
    main()
