#define pinoSensor_PIR 2
#define pinoBuzzer 3
#define pinoButton 4

bool espantalhoLigado = false;      // variavel para controlar se o sistema está ligado, false = iniciar o sistema desligado
bool destadoPIR = false;        // variavel para o estado do sensor
bool estadoBtnAnterior = HIGH;  // variavel para estado salvar o estado anterior do botão

unsigned long tempoAnteriorBotao = 0;
const unsigned long intervaloBotao = 200;  // variavel de debounce para evitar cliques duplos do botao

unsigned long tempoAnteriorPIR = 0;
const unsigned long intervaloPIR = 50;  // Intervalo entre as leituras do sensor e prints

// variáveis para controlar a alternância do som (bip)
unsigned long tempoAnteriorBuzzer = 0;
const unsigned long intervaloBuzzer = 200;  // Define a velocidade do bip (200ms ligado, 200ms desligado)

bool estadoBuzzer = false;
const short volumeMaximo = 255;

void setup() {
  pinMode(pinoSensor_PIR, INPUT);
  pinMode(pinoBuzzer, OUTPUT);
  pinMode(pinoButton, INPUT);
  Serial.begin(9600);
}

void loop() {
  verificarBotao();
  if (espantalhoLigado) {
    monitorarSensor();
  } else {
    desativarEspantalho();
  }
}

// Função para gerenciar o estado do sistema via botão
void verificarBotao() {
  unsigned long tempoAtual = millis();
  int estadoBtnAtual = digitalRead(pinoButton);  // estado atual do botão

  if (!estadoBtnAtual && estadoBtnAnterior)  // comparação dos estados do botão para ligar ou desligar o sistema
  {
    Serial.println(estadoBtnAtual);
    if (tempoAtual - tempoAnteriorBotao >= intervaloBotao) {
      espantalhoLigado = !espantalhoLigado;
      tempoAnteriorBotao = tempoAtual;
    }
  }
  estadoBtnAnterior = estadoBtnAtual;
}

// Função para gerenciar a detecção de movimento e o pinoBuzzer
void monitorarSensor() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnteriorPIR >= intervaloPIR) {
    tempoAnteriorPIR = tempoAtual;

    destadoPIR = digitalRead(pinoSensor_PIR);
    if (destadoPIR) {
      estadoBuzzer = HIGH;

      tempoAnteriorBuzzer = tempoAtual;
    }
  }

  // Lógica que alterna o estado do pinoBuzzer a cada 'intervaloBuzzer'
  if (estadoBuzzer && tempoAtual - tempoAnteriorBuzzer >= intervaloBuzzer) estadoBuzzer = LOW;


  analogWrite(pinoBuzzer, estadoBuzzer ? volumeMaximo : 0);
  Serial.print("pinoBuzzer ");  // 1 = movimento detectado
  Serial.println(estadoBuzzer ? "ON" : "OFF");
}



void desativarEspantalho() {
  estadoBuzzer = LOW;
  digitalWrite(pinoBuzzer, estadoBuzzer);
}