// Inclui a biblioteca do Servo
#include <Servo.h>

Servo servoPlataforma; // Define servoPlataforma como a forma de chamar as funções do Servo.h
#define pinoValv 7     // Define o pino que controla o relé
#define pinPlat 6      // Define o pino de movimento do servor motor
#define powerBtn 5

// configurações da plataforma
int angulosServo[3] = {0, 90, 180};
int anguloAtual = 0;
short dx = 1; // delta x, o próximo ângulo que ele tem que parar

// configurações da válvula
bool estadoValvula = true; // válvula aberta ou fechada
long tempoValv;            // controle de intervalos
bool deveRegar = false;    // define se o sistema deve iniciar um ciclo de rega

bool btnAgora;
bool btnAntes;
bool estadoEnergia = true;

bool esperar(long &tempoComponente, int segs) // função para controle de tempo de diferentes componentes
{
  bool passoutTempo = millis() - tempoComponente >= segs;
  if (passoutTempo)
  {
    tempoComponente = millis();
  }
  return passoutTempo;
}

void girarPlataforma()
{
  int arrLen = sizeof(angulosServo) / sizeof(angulosServo[0]);
  if (anguloAtual >= arrLen - 1)
  {
    dx = -1;
  }
  if (anguloAtual == 0)
  {
    dx = 1;
  }
  anguloAtual += dx;
  servoPlataforma.write(angulosServo[anguloAtual]);
}

void ativarValv(int tempoRega)
{
  if (!estadoValvula && deveRegar)
  {
    estadoValvula = true;
    Serial.println("ativ");
    digitalWrite(pinoValv, estadoValvula);
  }

  else if (estadoValvula && esperar(tempoValv, tempoRega))
  {
    estadoValvula = false;
    Serial.println("desativ");
    digitalWrite(pinoValv, estadoValvula);
    deveRegar = false;
  }
}

void toggle()
{
  btnAntes = btnAgora;
  btnAgora = digitalRead(powerBtn);

  if (btnAgora == HIGH && btnAntes == LOW)
  {
    girarPlataforma();
    deveRegar = true;
  }
  ativarValv(2000);
}

void setup()
{
  servoPlataforma.attach(pinPlat); // Atribui ao pino a função e receber os ângulos e rotação do servor
  servoPlataforma.write(0);
  pinMode(pinoValv, OUTPUT);
  pinMode(powerBtn, INPUT);
  Serial.begin(9600);
}

void loop()
{
  toggle();
}
