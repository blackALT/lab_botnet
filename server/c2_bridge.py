# c2_bridge.py - Middleware HTTP para Socket
# Uso: python3 c2_bridge.py
import socket
import json
import http.server
import socketserver

C2_IP = '127.0.0.1' # O C2 está na mesma maquina
C2_PORT = 5555
API_PORT = 8080

def get_bots_from_c2():
    try:
        # 1. Conecta ao C2 via Socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2)
        s.connect((C2_IP, C2_PORT))
        
        # 2. Envia Handshake especial de monitoramento
        s.sendall(b"HELLO MONITOR\n")
        
        # 3. Recebe o JSON
        data = b""
        while True:
            chunk = s.recv(4096)
            if not chunk: break
            data += chunk
            if b"]" in chunk: break # Fim do JSON array
            
        s.close()
        return data.decode('utf-8')
    except Exception as e:
        return json.dumps({"error": str(e)})

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # Endpoint da API
        if self.path == '/api/bots':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*') # Importante para o HTML funcionar
            self.end_headers()
            
            response = get_bots_from_c2()
            self.wfile.write(response.encode())
        else:
            # Serve arquivos estáticos (o próprio dashboard.html se estiver na pasta)
            super().do_GET()

print(f"[API] Iniciando Ponte em http://0.0.0.0:{API_PORT}")
print(f"[API] Conectando ao C2 em {C2_IP}:{C2_PORT} quando solicitado.")

with socketserver.TCPServer(("", API_PORT), Handler) as httpd:
    httpd.serve_forever()