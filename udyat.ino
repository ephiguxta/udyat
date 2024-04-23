#include <SPI.h>
#include <BluetoothSerial.h>
#include <string.h>

// essa lib vai servir apenas para o momento em que ocorre o boot do
// microcontrolador e durante a atribuição de nome do dispositivo,
// ficará algo como telemetria_<mac>
#include <WiFi.h>

// https://github.com/miguelbalboa/rfid
#include <MFRC522.h>

// rfid(cs, rst)
MFRC522 rfid(5, 27);

BluetoothSerial SerialBT;
char device_name[32] = "telemetria_";

// bool send_bluetooth_data(const char *log);
void send_bluetooth_data(const char *log);
bool check_data_request(void);
void nibble_to_byte(const char data, char conv_char[2]);
char valid_char(const char data);
bool check_cmd(const char *cmd);
void error_log(const char *log);
void get_uid(char uid[15]);
void set_bt_name(char device_name[32]);
bool insert_passwd(void);

void setup() {
  Serial.begin(115200);
  while(!Serial);

	byte mac[6];
	WiFi.macAddress(mac);

  SPI.begin();

  rfid.PCD_Init();

  delay(5000);

  set_bt_name(device_name);

  delay(124);
}

void loop() {
  delay(256);

  char uid[15] = { 0 };

  // a tag rfid só será lida caso houver um comando bluetooth
  if(check_data_request()) {
    const char *name = "telemetria_error";
    if(strncmp(device_name, name, strlen(name))) {
			Serial.printf("[%s] == [%s]\n", device_name, name);
      get_uid(uid);
			Serial.printf("Tentando enviar o [%s]\n", uid);
      send_bluetooth_data(uid);
			delay(256);
			Serial.printf("Dado bluetooth enviado!\n");
    }
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
	if(size <= 4)
		return false;

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

void send_bluetooth_data(const char *log) {
	unsigned log_length = strlen(log);

	Serial.printf("[");
  for(int i = 0; i < log_length; i++) {
    if(log[i] != '\0') {
			Serial.printf("%c", log[i]);
      SerialBT.write(log[i]);
    }
  }
	Serial.printf("]\n");
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

void get_uid(char uid[15]) {
	// caso ele tente a comunicação com rfid e ele não "bater"
	// a mesma senha ele vai entrar em loop infinito
	if(!insert_passwd()) {
		Serial.println("Senha incorreta!");

		while(1) {
			delay(128);
		}
	}

  // habilitando a antena para realizar a leitura do rfid
  rfid.PCD_AntennaOn();
	delay(128);

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

	byte mac[6] = { 0 };
	byte mac_reversed[6] = { 0 };

	// como o mac vem em bytes é necessário uma conversão
	// para ASCII válido. Como são 6 bytes, vamos precisar
	// de 12 caracteres + '\0'
	char mac_converted[13] = { 0 };

	// inserindo o mac dentro da variável mac
	WiFi.macAddress(mac);

	// o mac vem de maneira inversa, precisamos inverter ele
	short j = 0;
	for(int i = 5; i >= 0; i--) {
		mac_reversed[j++] = mac[i];
	}

	// essa máscara serve apenas para o mac não ficar exposto
	// no nome do bluetooth, pois é para isso que estamos pegando
	// o mac do dispositivo, já quem em teoria cada um tem o seu
	byte mask[6] = {
		0xca, 0xfe, 0xee,
		0xba, 0xbe, 0xee
	};

	// ""criptografia"" do mac através do XOR
	for(int i = 0; i < 6; i++) {
		mac_reversed[i] ^= mask[i];
	}

	for(int i = 0; i < 6; i++) {
		char buffer[2] = { 0 };

		nibble_to_byte(mac_reversed[i], buffer);
    strncat(mac_converted, buffer, 2);
	}

	mac_converted[12] = '\0';

  Serial.printf("Peguei o MAC, e é [%s]\n", mac_converted);

  unsigned int len = strlen(device_name);
  Serial.printf("len: %d\n", len);

  strncat(device_name, mac_converted, strlen(mac_converted));

  Serial.printf("O nome é valido: [%s]\n", device_name);
  SerialBT.begin(device_name);
}


bool insert_passwd(void) {
	// por padrão ele possui uma senha de 32 bits com todos os bits em 1
	byte passwd_buff[] = { 0xff, 0xff, 0xff, 0xff };
	byte pack[] = { 0x0, 0x0 };

	// esse método de autenticação retorna um tipo "enum StatuCode..."
	byte status_code = rfid.PCD_NTAG216_AUTH(&passwd_buff[0], pack);

	// STATUS_OK == 0
	if (status_code) {
		Serial.println("STATUS_OK");
		return true;
	}

	Serial.println("erro!");
	return false;
}
