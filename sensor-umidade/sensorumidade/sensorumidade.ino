// Sistema de monitoramento da umidade no solo
#include <LiquidCrystal_I2C.h>

const int sensor = A0;    // Define o pino do sensor como sendo o A0
int valorUmidade = 0;     // O valor bruto da umidade
int porcentagem = 0;      // A porcentagem que vai ser convertida pelo map
const int valAr = 550;    // da tensao que o ar traz pra o circuito
const int valAgua = 250;  // da tensao da agua que ela traz pra o circuito


//Declarando o display LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {

  lcd.begin(16, 2);
  Serial.begin(9600);


  //Inicializa o display LCD
  lcd.init();

  lcd.backlight();
  lcd.leftToRight();
  lcd.setCursor(3, 0);
  lcd.print("Monitor de");
  lcd.setCursor(0, 1);
  lcd.print("Umidade do Solo");
  delay(4000);
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nivel de Umidade:");

  // lê o pino do sensor
  valorUmidade = analogRead(sensor);                        // Lê o pino A1
  porcentagem = map(valorUmidade, valAr, valAgua, 0, 100);  // Converte o pra percentual
  porcentagem = constrain(porcentagem, 0, 100);             // USA CONSTRAIN PRA MANTER UM LIMITE DE UMIDADE, TANTO PRA CIMA COMO PRA BAIXO

  //Condiçoes para cada nivel de tensao
  if (porcentagem <= 30) {

    Serial.print(analogRead(sensor));
    Serial.print("\n");
    lcd.setCursor(6, 1);
    lcd.print("SECO");
    delay(2000);
    lcd.clear();
  }

  if (porcentagem > 30 && porcentagem < 55) {
    Serial.print(analogRead(sensor));
    Serial.print("\n");
    lcd.setCursor(5, 1);
    lcd.print("UMIDO");
    delay(2000);
    lcd.clear();
  }

  if (porcentagem > 55 && porcentagem < 75) {

    Serial.print(analogRead(sensor));
    Serial.print("\n");
    lcd.setCursor(3, 1);
    lcd.print("MUITO UMIDO");
    delay(2000);
    lcd.clear();
  }
  if (porcentagem > 75) {

    Serial.print(analogRead(sensor));
    Serial.print("\n");
    lcd.setCursor(5, 1);
    lcd.print("ENCHARCADO");
    delay(2000);
    lcd.clear();
  }
  delay(10);
}
