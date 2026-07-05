import http.server
import socketserver

PORT = 8081

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b"Hello from Mini-CI Python App!\n")

if __name__ == "__main__":
    # Allow port reuse to avoid "Address already in use" errors during quick restarts
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Python server listening on port {PORT}...")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass
