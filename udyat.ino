#include <SPI.h>

// https://github.com/miguelbalboa/rfid
#include <MFRC522.h>

MFRC522 rfid(5, 27);

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
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

void loop() {
  char uid[10] = { 0 };

  // verifica se o há a presença da tag e se ela pode
  // ser lida.
  if(rfid.PICC_IsNewCardPresent()) {
    if(rfid.PICC_ReadCardSerial()) {

      // quando chamamos o método ReadCardSerial, o PICC_SELECT
      // é chamado e as para pegar os bytes ocorrem
      for(short i = 0; i < rfid.uid.size; i++) {
        uid[i] = rfid.uid.uidByte[i];
        Serial.print(uid[i], HEX);
      }
      Serial.println();

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

  // lendo a tag sem precisar distanciar e aproximar todo instante.
  on_off_cycle();
}
