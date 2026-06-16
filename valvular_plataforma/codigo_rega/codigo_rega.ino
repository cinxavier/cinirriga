#include <Servo.h>

Servo meuServo;
#define pinoServo 6
#define pinoBomba 7

// Estados do sistema
enum Estado {
  GIRAR_SERVO,
  ESPERAR_SERVO,
  PARAR_SERVO,
  LIGAR_BOMBA,
  DESLIGAR_BOMBA,
  ESPERAR_BOMBA
};

Estado acaoAtual = GIRAR_SERVO;
// tempo em ms
unsigned long tempoAnterior = 0;
const unsigned long TEMPO_GIRO = 400;
const unsigned long TEMPO_BOMBA = 5000;
const unsigned long TEMPO_ESPERA = 500;

// Controle do servo
short idx_ang = 0;
int dx = 1;  // delta x
int angs[] = { 0, 60, 120, 180 };
short angsLen = sizeof(angs) / sizeof(angs[0]);

void setup() {
  meuServo.attach(pinoServo);
  meuServo.write(0);
  pinMode(pinoBomba, OUTPUT);
  digitalWrite(pinoBomba, LOW);  //  bomba desligada
  Serial.begin(9600);
}

void loop() {
  unsigned long AGORA = millis();
  switch (acaoAtual) {
    case GIRAR_SERVO:
      idx_ang += dx;
      meuServo.write(angs[idx_ang]);
      if (idx_ang >= angsLen - 1 || idx_ang <= 0) {
        dx *= -1;
      }
      acaoAtual = ESPERAR_SERVO;
      tempoAnterior = AGORA;

      break;

    case ESPERAR_SERVO:
      if (AGORA - tempoAnterior >= TEMPO_GIRO) {
        acaoAtual = PARAR_SERVO;
        tempoAnterior = AGORA;
      }
      break;

    case PARAR_SERVO:
      if (AGORA - tempoAnterior >= TEMPO_ESPERA) {
        acaoAtual = LIGAR_BOMBA;
        tempoAnterior = AGORA;
      }
      break;

    case LIGAR_BOMBA:
      digitalWrite(pinoBomba, HIGH);  // liga bomba

      if (AGORA - tempoAnterior >= TEMPO_BOMBA) {
        acaoAtual = DESLIGAR_BOMBA;
        tempoAnterior = AGORA;
      }
      break;

    case DESLIGAR_BOMBA:
      digitalWrite(pinoBomba, LOW);  // desliga bomba
      acaoAtual = ESPERAR_BOMBA;
      tempoAnterior = AGORA;
      break;

    case ESPERAR_BOMBA:
      if (AGORA - tempoAnterior >= TEMPO_ESPERA) {
        acaoAtual = GIRAR_SERVO;
        tempoAnterior = AGORA;
      }
      break;
  }
}