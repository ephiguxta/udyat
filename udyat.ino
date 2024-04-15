#include <SPI.h>
#include <BluetoothSerial.h>
#include <string.h>

// https://github.com/miguelbalboa/rfid
#include <MFRC522.h>

// rfid(cs, rst)
MFRC522 rfid(5, 27);

BluetoothSerial SerialBT;
char device_name[32] = "telemetria_";

bool send_bluetooth_data(const char *log);
bool check_data_request(void);
void nibble_to_byte(const char data, char conv_char[2]);
char valid_char(const char data);
bool check_cmd(const char *cmd);
void error_log(const char *log);
void get_uid(char uid[10]);
void set_bt_name(char device_name[32]);

void setup() {
  Serial.begin(115200);
  while(!Serial);

  SPI.begin();
  rfid.PCD_Init();

  delay(5000);

  set_bt_name(device_name);

  delay(124);
}

void loop() {
  delay(256);

  char uid[10] = { 0 };

  // a tag rfid só será lida caso houver um comando bluetooth
  if(check_data_request()) {
    Serial.printf("bluetooth name: [%s]\n", device_name);
    get_uid(uid);
    send_bluetooth_data(uid);
  }
}

void error_log(const char *log) {
  while(*log != '\0') {
    Serial.printf("%c", *log);
    SerialBT.write(*log);
    log++;
  }
  Serial.println();
}

bool check_cmd(const char *cmd) {
  char tmp[16] = "get_uid";

  int size = strlen(tmp);
  int cmp_ret = strncmp(cmd, tmp, size);

  if(cmp_ret == 0) {
    Serial.printf("[%s] é um comando válido!\n", cmd);
    return true;
  }

  // TODO: verifique porque pra cada requisição sempre
  // sobra um byte a mais no final ou no começo que
  // sempre será inválido
  Serial.printf("[%s] é um comando inválido!\n", cmd);
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
        char buff;
        buff = SerialBT.read();

        if(buff == '\0' || buff == '\r' || buff == '\n') {
          break;
        }

        cmd[i] = buff;

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
      SerialBT.write(log[i]);
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

void nibble_to_byte(const char data, char conv_char[2]) {
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
  conv_char[0] = nibble;

  nibble = data;
  nibble &= 0x0f;
  nibble = valid_char(nibble);
  conv_char[1] = nibble;
}

void get_uid(char uid[10]) {

  // habilitando a antena para realizar a leitura do rfid
  rfid.PCD_AntennaOn();
  rfid.PCD_AntennaOff();
  rfid.PCD_AntennaOn();

  // verifica se o há a presença da tag e se ela pode
  // ser lida.
  if(rfid.PICC_IsNewCardPresent()) {
    if(rfid.PICC_ReadCardSerial()) {

      // quando chamamos o método ReadCardSerial, o PICC_SELECT
      // é chamado e as para pegar os bytes ocorrem
      for(short i = 0; i < rfid.uid.size; i++) {
        char buffer[2] = { 0 };
        char uid_byte = rfid.uid.uidByte[i];

        nibble_to_byte(uid_byte, buffer);
        strncat(uid, buffer, 2);
      }

      uid[(rfid.uid.size * 2) + 1] = '\0';

      Serial.printf("Li o UID: [%s]\n", uid);

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  } else {
    const char* log = "rfid_off";
    error_log(log);
  }

  rfid.PCD_AntennaOff();
  return;
}

void set_bt_name(char device_name[32]) {
  delay(128);

  char uid[10] = { 0 };
  get_uid(uid);

  Serial.printf("Peguei o UID, e é [%s]\n", uid);

  unsigned int len = strlen(device_name);
  Serial.printf("len: %d\n", len);

  // esse campo rfid.uid.size é preenchido quando acessamos o
  // get_uid, se ele for zero (aka falso) significa que nada
  // foi lido
  if(rfid.uid.size) {

    strncat(device_name, uid, strlen(uid));

    Serial.printf("O nome é valido: [%s]\n", device_name);

  } else {
    const char *error = "error";
    for(int i = 0; i < 5; i++) {
      device_name[i + len] = error[i];
    }
    Serial.printf("O nome é inválido: [%s]\n", device_name);
  }

  SerialBT.begin(device_name);
}
