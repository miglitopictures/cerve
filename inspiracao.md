Antes de decidir programar "na mão" para aprender e praticar, resolvi meu problema de simular o servidor do Github Pages com o Claude (Sonnet 5.0):

```python
import http.server
import os

PORT = 8080

class GitHubPagesHandler(http.server.SimpleHTTPRequestHandler):
    def send_error(self, code, message=None, explain=None):
        if code == 404:
            self.serve_404()
        else:
            super().send_error(code, message, explain)

    def serve_404(self):
        try:
            with open("404.html", "rb") as f:
                content = f.read()
        except FileNotFoundError:
            content = b"404 Not Found"

        self.send_response(404)
        self.send_header("Content-Type", "text/html")
        self.send_header("Content-Length", str(len(content)))
        self.end_headers()
        self.wfile.write(content)

os.chdir(os.path.dirname(os.path.abspath(__file__)))  # serve from this script's folder

with http.server.HTTPServer(("", PORT), GitHubPagesHandler) as httpd:
    print(f"Serving at http://localhost:{PORT}")
    httpd.serve_forever()
```

Funcionou perfeitamenta para meu uso... E é certamente robusto do que minha implementação em C. Deixo aqui como uma nota de reflexão para mim mesmo, mas pode ser que ajude alguém com o mesmo problema!