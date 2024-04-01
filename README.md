### Udyat

Esse módulo irá verificar a presença de tags RFID e enviar o UID delas para o
cliente que se conectar via Bluetooth ao módulo.

## Como utilizar na prática

Faça a conexão com o ESP32 e envie a requisição contendo a _string_: `get_uid`.

## O desenvolvimento

E necessário a instalação da [biblioteca](https://github.com/miguelbalboa/rfid)
de RFID que opera no módulo MFRC522. Para os usuário da Arduino IDE, basta
pesquisar por MFRC522 na aba de bibliotecas e instalar o mesmo.
