#include <SPI.h>
#include <BluetoothSerial.h>

// https://github.com/miguelbalboa/rfid
#include <MFRC522.h>

// rfid(cs, rst)
MFRC522 rfid(5, 27);

BluetoothSerial SerialBT;
const char *device_name = "Telemetria";

bool send_bluetooth_data(const char *log);
bool check_data_request(void);
void send_nibble(const char data);
char valid_char(const char data);
bool check_cmd(const char *cmd);
void error_log(const char *log);
void on_off_cycle();

void setup() {
  Serial.begin(115200);
  while(!Serial);

  SPI.begin();
  rfid.PCD_Init();

  SerialBT.begin(device_name);

  delay(124);
}

void loop() {
  char uid[10] = { 0 };

  // a tag rfid só será lida caso houver um comando bluetooth
  if(check_data_request()) {
    // lendo a tag sem precisar distanciar e aproximar todo instante.
    on_off_cycle();

    // verifica se o há a presença da tag e se ela pode
    // ser lida.
    if(rfid.PICC_IsNewCardPresent()) {
      if(rfid.PICC_ReadCardSerial()) {
        // quando chamamos o método ReadCardSerial, o PICC_SELECT
        // é chamado e as para pegar os bytes ocorrem
        for(short i = 0; i < rfid.uid.size; i++) {
          uid[i] = rfid.uid.uidByte[i];
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();

        send_bluetooth_data(uid);
      }
    } else {
      const char* log = "rfid_off";
      error_log(log);
    }

  }
}

void error_log(const char *log) {
  while(*log != '\0') {
    Serial.printf("%c", *log - '\0');
    SerialBT.write(*log - '\0');
    log++;
  }
  Serial.println();
}

bool check_cmd(const char *cmd) {
  // TODO: fazer mais comandos, pois será acoplado
  // GPS e Cartão SD ao módulo
  char tmp[16] = "get_uid";

  int counter = 0;

  for(int i = 0; i < 16; i++) {
    if(tmp[i] == '\0' || cmd[i] == '\0') {
      break;
    }

    if(tmp[i] == cmd[i]) {
      counter++;
    }
  }

  if(counter == 7) {
    return true;
  }

  return false;
}

bool check_data_request(void) {
  // get_uid
  char cmd[16] = { 0 };
  char in_data;

  if(SerialBT.available()) {

    // caso haja dados sendo enviados por outro dispositivo,
    // pega os 16 bytes, ou menos, e verifica se é um
    // comando.
    //
    for(int i = 0; i < 16; i++) {
      if(SerialBT.available()) {
        cmd[i] = SerialBT.read();

      } else {
        // TODO: alguns dispositivos podem enviar apenas os caracteres "visuais",
        // outros podem enviar um CRLF, procure um jeito de tornar essa atribuição
        // dinâmica e não hardcoded.
        //

        // esse (i - 1) é para ignorar o \n
        cmd[i - 1] = '\0';
      }
    }

    if(check_cmd(cmd)) {
      return true;
    }
  }
  delay(20);

  return false;
}

bool send_bluetooth_data(const char *log) {

  for(int i = 0; i < 16; i++) {
    if(log[i] != '\0') {
      send_nibble(log[i]);
      continue;
    }

    return false;
  }

  return true;
}

char valid_char(const char data) {
  // apenas converte o nibble obtido em um
  // caractere hexadecimal válido.

  char conv = data;

  if(conv >= 0 && conv <= 9) {
    conv += 48;

  } else {
    conv += 87;
  }

  return conv;
}

void send_nibble(const char data) {
  // as máscaras servirão para dividir um
  // byte em dois bytes com o nibble mais significativo todo zerado,
  // então enviaremos dois bytes ao invés de um, para que
  // essas partes sejam representadas no log do destinatário.
  //

  char nibble;

  nibble = data;
  nibble >>= 4;
  nibble &= 0x0f;

  nibble = valid_char(nibble);
  SerialBT.write(nibble);

  nibble = data;
  nibble &= 0x0f;
  nibble = valid_char(nibble);
  SerialBT.write(nibble);
}

void on_off_cycle() {
  // como a tag vai ficar parada, a leitura será feita num intervalode ~1s
  // desligando e ligando a antena, assim poderemos ler o rfid.
  //

  delay(512);
  rfid.PCD_AntennaOff();
  delay(512);
  rfid.PCD_AntennaOn();
}
