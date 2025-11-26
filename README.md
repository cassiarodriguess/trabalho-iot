# Sistema IoT de Monitoramento de Temperatura com ESP32 e MQTT

Este repositório contém o código, o diagrama do circuito e a documentação do projeto desenvolvido para a disciplina de Internet das Coisas.  
O objetivo é monitorar a temperatura de uma caixa de vacinas utilizando o **ESP32 DevKit V1**, o sensor **DS18B20** e a comunicação **MQTT** via broker HiveMQ.  
O sistema também aciona **LED** e **buzzer** quando a temperatura sai da faixa segura.

---

## 1. Funcionamento do Sistema

O ESP32 realiza leituras periódicas da temperatura e publica os dados no broker MQTT usando o tópico:

vacinas/box01/temp

markdown
Copiar código

Faixa considerada **segura**: **2°C a 8°C**  
Faixa **de risco**:
- Abaixo de 2°C → risco de congelamento  
- Acima de 8°C → risco para conservação  
- Acima de 10°C → condição crítica  

Quando a temperatura sai da faixa segura:
- LED vermelho **acende** (alerta visual)  
- Buzzer **toca** (alerta sonoro)  
- Estado é publicado no tópico:

vacinas/box01/alarme

markdown
Copiar código

Também é possível **acionar ou desligar o alarme remotamente** com comandos MQTT:

Tópico: vacinas/box01/cmd

Mensagens aceitas:
ALARM_ON
ALARM_OFF

markdown
Copiar código

---

## 2. Hardware Utilizado

### Plataforma de Desenvolvimento — ESP32 DevKit V1
O “cérebro” do sistema.  
Responsável por:
- ler o sensor DS18B20  
- controlar LED e buzzer  
- processar regras de alarme  
- conectar ao Wi-Fi e enviar dados ao MQTT  

### Sensor de Temperatura — DS18B20
Realiza a medição da temperatura usando protocolo **1-Wire**.  
Conexões:
- DATA → GPIO 4  
- VDD → 3V3  
- GND → GND  
- Resistor de pull-up **4,7 kΩ** entre DATA e 3V3  

### Atuadores
- **LED vermelho (GPIO17)** → indicado para alerta visual  
- **Buzzer ativo (GPIO16)** → indicado para alerta sonoro  

### Componentes Auxiliares
- Protoboard  
- Jumpers  
- Resistor 220 Ω (LED)  
- Resistor 4,7 kΩ (DS18B20)  
- Cabo USB para alimentação  

---

## 3. Comunicação e Protocolos (MQTT)

**Protocolo utilizado:** MQTT  
**Broker:** HiveMQ – `broker.hivemq.com`  
**Porta:** 1883  

### Tópicos utilizados:

| Tópico | Descrição |
|--------|-----------|
| `vacinas/box01/temp` | Publicação da temperatura em JSON |
| `vacinas/box01/alarme` | Estado atual do alarme |
| `vacinas/box01/cmd` | Comandos remotos (ALARM_ON / ALARM_OFF) |

### Exemplo de payload publicado:
```json
{
  "tC": 5.23,
  "uptime_s": 120
}
4. Software Desenvolvido
O código principal está no arquivo:

Copiar código
sketch.ino
Bibliotecas utilizadas:

PubSubClient

OneWire

DallasTemperature

As funcionalidades implementadas incluem:

leitura do sensor DS18B20

cálculo de histerese

acionamento automático de LED/buzzer

callback para comandos MQTT

reconexão automática Wi-Fi/MQTT

publicação em JSON

5. Como Reproduzir o Projeto
1- Montar o circuito
Use o arquivo diagram.json (compatível com Wokwi):

DS18B20 no GPIO4

LED no GPIO17 com resistor

Buzzer no GPIO16

2- Abrir o código
Abra sketch.ino no Wokwi ou Arduino IDE.

3️ Iniciar a simulação
Acompanhe o Serial Monitor para verificar leituras e alarmes.

4️ Testar o MQTT
Use o cliente WebSocket da HiveMQ:
https://www.hivemq.com/demos/websocket-client/

Assine:

vacinas/box01/temp

vacinas/box01/alarme

Publique comandos em:

vacinas/box01/cmd

6. Estrutura do Repositório
pgsql
Copiar código
.
├── README.md
├── sketch.ino
├── diagram.json
└── libraries.txt
7. Vídeo Demonstrativo
https://youtu.be/SYpp5RNKpes

8. Autores
Rita de Cássia Gonçalves Rodrigues

Gabriel Agles Gomes
