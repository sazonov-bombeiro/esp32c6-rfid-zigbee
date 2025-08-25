# ESP32-C6 RFID (Wiegand) + Zigbee + Web UI

**IDF:** 5.3.x • **Board:** ESP32-C6 • **RFID:** Wiegand • **Zigbee:** ESP-Zigbee-SDK • **Web UI:** esp_http_server (SoftAP)

## Estrutura
- `main/app_wiegand.*` — Leitura Wiegand (D0/D1), relé, LED, buzzer (comentários em RU)
- `main/app_zigbee.*` — Endpoint HA (0x0104), cluster custom 0xFC00 com atributo `last_uid` (comentários em RU)
- `main/app_http.*` — Web UI simples em SoftAP (SSID `rfid-c6`, senha `12345678`)

## Pré-requisitos
- ESP-IDF v5.3.x instalado e exportado (`. ./export.sh`)
- ESP-Zigbee-SDK clonado. Ex.: `git clone https://github.com/espressif/esp-zigbee-sdk`
- Defina `ZIGBEE_SDK_PATH` apontando para o SDK **ou** coloque a pasta `esp-zigbee-sdk` ao lado deste projeto.

## Compilar e flash
```bash
idf.py set-target esp32c6
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Notas
- Ajuste os pinos Wiegand em `main.c` (estrutura `wiegand_pins_t`).
- Integração com Home Assistant (ZHA): adicione o dispositivo à rede Zigbee e crie quirk se quiser expor `last_uid` como sensor/texto.
- Web UI: conecte-se ao AP `rfid-c6` para acessar `http://192.168.4.1/`.
