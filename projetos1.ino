#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// ====================== LCD ======================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ====================== RFID ======================
#define RST_PIN         5
#define SS_PIN          53
#define LED_AUTORIZADO  44
#define LED_NEGADO      42
#define LED_AMARELO     22  // NOVO LED - estoque cheio
#define BUZZER          40

MFRC522 mfrc522(SS_PIN, RST_PIN);

// ====================== SERVOS ======================
Servo servo1;
Servo servo2;
#define SERVO1_PIN 2
#define SERVO2_PIN 3

// ====================== ULTRASSÔNICO ======================
#define TRIG_PIN 9
#define ECHO_PIN 8

// ====================== UIDs AUTORIZADOS ======================
String uids_autorizados[] = {
  "CD AD A2 F2",
  "3D EE A6 F2",
  "A2 64 F3 00",
  "12 34 56 78",
  "FD 19 E0 F1",
  "DD 5E A8 F2",
  "92 37 F9 00",
  "12 A6 EA 00",
  "A2 F0 F2 00",
  "DD 8D 00 C2",
  "5D 43 F2 F1",
  "92 72 ED 00"
};
const int total_uids = sizeof(uids_autorizados) / sizeof(uids_autorizados[0]);

// ====================== VARIÁVEIS GLOBAIS ======================
int contadorObjetos = 0;     
bool objetoPresente = false; 
const int LIMITE_OBJETOS = 5; // limite de armazenamento

// ====================== SETUP ======================
void setup() {
  Serial.begin(9600);
  while (!Serial);

  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();
  Serial.println(F("Aproxime o cartão para ler o UID..."));

  pinMode(LED_AUTORIZADO, OUTPUT);
  pinMode(LED_NEGADO, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(LED_AUTORIZADO, LOW);
  digitalWrite(LED_NEGADO, LOW);
  digitalWrite(LED_AMARELO, LOW);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BEM VINDO");
  lcd.setCursor(0, 1);
  lcd.print("GRUPO G9!!");
  delay(2000);
  lcd.clear();

  // Servos
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(0);
  servo2.write(0);
}

// ====================== FUNÇÃO ULTRASSÔNICO ======================
int lerDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000); 
  int distancia = duracao * 0.034 / 2;
  return distancia;
}

// ====================== LOOP ======================
void loop() {
  // Se estoque cheio, mostrar aviso e ignorar leituras
  if (contadorObjetos >= LIMITE_OBJETOS) {
    digitalWrite(LED_AMARELO, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ESTOQUE CHEIO!");
    lcd.setCursor(0, 1);
    lcd.print("Capacidade: 5");
    tone(BUZZER, 1200);
    delay(800);
    noTone(BUZZER);
    delay(2000);
    return; // bloqueia novas leituras
  }

  // ---- Leitura do sensor ultrassônico ----
  int distancia = lerDistancia();

  if (distancia > 0 && distancia < 20 && !objetoPresente) {
    objetoPresente = true;
    contadorObjetos++;
    Serial.print("Objeto detectado! Total: ");
    Serial.println(contadorObjetos);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Objeto detectado!");
    lcd.setCursor(0, 1);
    lcd.print("Total: ");
    lcd.print(contadorObjetos);
    delay(1000);
  } 
  else if (distancia >= 25) {
    objetoPresente = false;
  }

  // ---- Leitura do RFID ----
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String conteudo = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    conteudo += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    conteudo += String(mfrc522.uid.uidByte[i], HEX);
  }
  conteudo.toUpperCase();
  conteudo = conteudo.substring(1);

  Serial.print("UID lido: ");
  Serial.println(conteudo);

  bool autorizado = false;
  for (int i = 0; i < total_uids; i++) {
    if (conteudo == uids_autorizados[i]) {
      autorizado = true;
      break;
    }
  }

  lcd.clear();

  if (autorizado) {
    // Se estoque estiver cheio, nega abertura
    if (contadorObjetos >= LIMITE_OBJETOS) {
      lcd.setCursor(0, 0);
      lcd.print("ESTOQUE CHEIO!");
      lcd.setCursor(0, 1);
      lcd.print("Sem espaco!");
      digitalWrite(LED_AMARELO, HIGH);
      tone(BUZZER, 1000);
      delay(800);
      noTone(BUZZER);
      digitalWrite(LED_AUTORIZADO, LOW);
      return;
    }

    Serial.println("Acesso AUTORIZADO!");
    digitalWrite(LED_AUTORIZADO, HIGH);
    digitalWrite(LED_NEGADO, LOW);

    lcd.setCursor(0, 0);
    lcd.print("APROVADO :)");
    lcd.setCursor(0, 1);
    lcd.print("BEM-VINDO!");

    // LED e buzzer simultâneos
    tone(BUZZER, 1200);
    delay(200);
    tone(BUZZER, 1500);
    delay(200);
    tone(BUZZER, 1700);
    delay(300);
    noTone(BUZZER);

    delay(600);

    // Exibe total de objetos detectados
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Objetos detectados:");
    lcd.setCursor(0, 1);
    lcd.print(contadorObjetos);
    delay(1300);

    // Movimento dos servos
    for (int pos = 0; pos <= 180; pos++) {
      servo1.write(pos);
      servo2.write(pos);
      delay(10);
    }
    delay(1000);
    for (int pos = 180; pos >= 0; pos--) {
      servo1.write(pos);
      servo2.write(pos);
      delay(10);
    }

    delay(500);
    digitalWrite(LED_AUTORIZADO, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BEM VINDO");
    lcd.setCursor(0, 1);
    lcd.print("GRUPO G9!!");
  } 
  else {
    Serial.println("Acesso NEGADO!");
    digitalWrite(LED_NEGADO, HIGH);
    digitalWrite(LED_AUTORIZADO, LOW);

    lcd.setCursor(0, 0);
    lcd.print("NEGADO :(");
    lcd.setCursor(0, 1);
    lcd.print("TENTE OUTRO!");

    tone(BUZZER, 1500);
    delay(1000);
    noTone(BUZZER);

    digitalWrite(LED_NEGADO, LOW);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(300);
}
