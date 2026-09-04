import socket, sys, time, threading

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 18080
LOG = "/tmp/origin_log.txt"

BODY = {
    "/small": b"hello proxy world\n" * 3,
    "/medium": (b"0123456789abcdef" * 6000),   # 96000 bytes
    "/large": (b"0123456789abcdef" * 20000),    # 320000 bytes
    "/exact": b"A" * 100000,                     # test close to limit
    "/empty": b"",
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
    path = "/"
    if len(parts) >= 2:
        path = parts[1]
    # strip query
    key = path.split("?")[0]
    body = BODY.get(key, b"not found: %s\n" % key.encode())
    status = "200 OK" if key in BODY else "404 Not Found"
    resp = ("HTTP/1.0 %s\r\nServer: test-origin\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n"
            % (status, len(body))).encode() + body
    try:
        c.sendall(resp)
    except Exception as e:
        print("send err", e, file=sys.stderr)
    c.close()

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("127.0.0.1", PORT))
    s.listen(64)
    open(LOG, "w").close()
    print("origin listening on", PORT, flush=True)
    while True:
        c, _ = s.accept()
        threading.Thread(target=handle, args=(c,), daemon=True).start()

if __name__ == "__main__":
    main()
