#!/bin/bash
# setup_c2.sh - Prepara o ambiente do Atacante
# Certifique-se de ter os arquivos: c2_safe_mirai.c, c2_bridge.py

echo "[*] Compilando o Servidor C2 (Mirai Safe)..."
gcc c2_safe_mirai.c -o c2_server

if [ $? -eq 0 ]; then
    echo "[+] Compilação bem-sucedida!"
else
    echo "[-] Erro na compilação."
    exit 1
fi

echo "[*] Verificando Python para a API..."
if ! command -v python3 &> /dev/null; then
    echo "[-] Python3 não encontrado. Instale-o."
    exit 1
fi

echo "[OK] Ambiente pronto."
echo "Para iniciar o lab, abra dois terminais:"
echo "  Terminal 1: ./c2_server 5555"
echo "  Terminal 2: python3 c2_bridge.py"