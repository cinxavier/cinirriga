#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define pinoSensorUmidade A0  //pino do sensor de umidade
#define pinoIrrigacao 30      //pino do rele da água
#define pinoFertilizacao 32   //pino do rele do fertilizante
#define pinoServo 34          //pino do servo


Servo plataforma;
LiquidCrystal_I2C lcd(0x27, 16, 2);

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
const unsigned long TEMPO_ESPERA = 500;

// Controle do servo
int idx_ang = 0;
int dx = 1;  // delta x
int angs[] = { 0, 60, 120, 180 };
short angsLen = sizeof(angs) / sizeof(angs[0]);

const int valAr = 500;           // da tensao que o ar traz pra o circuito
const int valAgua = 200;         // da tensao da agua que ela traz pra o circuito
int umidade = 0;                 //valor da umidade base
short porcentagemUmidade = 0;    //porcentagemUmidade de umidade no solo (calcula depois)
const short umidadeMinima = 40;  //num minimo de umidade, menos que isso tá seco

//Declarando o display LCD


void iniciarCicloIrrigacao(short segs, long AGORA) {
  switch (acaoAtual) {
    case GIRAR_SERVO:
      idx_ang += dx;
      plataforma.write(angs[idx_ang]);
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
      digitalWrite(pinoIrrigacao, HIGH);  // liga bomba

      if (AGORA - tempoAnterior >= segs) {
        acaoAtual = DESLIGAR_BOMBA;
        tempoAnterior = AGORA;
      }
      break;

    case DESLIGAR_BOMBA:
      digitalWrite(pinoIrrigacao, LOW);  // desliga bomba
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


void setup() {
  plataforma.attach(pinoServo);
  plataforma.write(0);

  pinMode(pinoIrrigacao, OUTPUT);
  digitalWrite(pinoIrrigacao, LOW);  // inicia o rele desligado

  pinMode(pinoFertilizacao, OUTPUT);
  digitalWrite(pinoFertilizacao, LOW);  // inicia o rele desligado

  lcd.begin(16, 2);
  lcd.init();

  lcd.backlight();
  lcd.leftToRight();
  lcd.setCursor(3, 0);
  lcd.print("Monitor de");
  lcd.setCursor(0, 1);
  lcd.print("Umidade do Solo");

  Serial.begin(9600);  // serial begin sem segredo
}

void loop() {
  unsigned long AGORA = millis();

  if (AGORA - tempoAnterior > 1000) {
    Serial.println(umidade);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Nivel de Umidade:");

    umidade = analogRead(pinoSensorUmidade);                     // lê o valor dito pelo sensor de umidade
    porcentagemUmidade = map(umidade, valAr, valAgua, 0, 100);   // transforma pra porcentagemUmidade
    porcentagemUmidade = constrain(porcentagemUmidade, 0, 100);  // USA CONSTRAIN PRA MANTER UM LIMITE DE UMIDADE, TANTO PRA CIMA COMO PRA BAIXO


    if (porcentagemUmidade > 85) {
      lcd.setCursor(3, 1);
      lcd.print("ENCHARCADO");
    } else if (70 < porcentagemUmidade <= 85) {
      lcd.setCursor(3, 1);
      lcd.print("MUITO UMIDO");
    } else if (30 < porcentagemUmidade <= 70) {
      lcd.setCursor(3, 1);
      lcd.print("UMIDO");
    } else if (porcentagemUmidade <= 30) {
      lcd.setCursor(3, 1);
      lcd.print("SECO");
      iniciarCicloIrrigacao(5000, AGORA);
    }
    // delay(2000);
    tempoAnterior = AGORA;
  }
}