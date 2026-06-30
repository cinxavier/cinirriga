#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define pinoSensorUmidade A0  // pino do sensor de umidade

#define pinoIrrigacao 30     // pino do rele da água
#define pinoFertilizacao 32  // pino do rele do fertilizante
#define pinoPlataforma 34    // pino do servo

#define pinoSensor_PIR 31
#define pinoBuzzer 33
#define pinoButton 35


Servo plataforma;  // servo motor da plataforma

const short TAMANHO_LCD[2] = { 16, 2 };                       // dimensões do display LCD (colunas, linhas)
LiquidCrystal_I2C lcd(0x27, TAMANHO_LCD[0], TAMANHO_LCD[1]);  // endereço e configuração do LCD
String conteudoLCD[2] = {
  "",
  "",
};
String filaMensagens[15] = {};

// Estados do sistema, passos que a máquina segue
enum Estado {
  GIRAR_SERVO,
  ESPERAR_SERVO,
  PARAR_SERVO,
  LIGAR_FONTE,
  DESLIGAR_FONTE,
  ESPERAR_BOMBA
};

Estado acaoAtual = GIRAR_SERVO;  // a ação atual da máquina

// tempo em ms
unsigned long ultimaChecagemIrrigacao = 0;     // controle de tempo da irrigação
unsigned long ultimaChecagemFertilizacao = 0;  // controle de tempo da fertilização
const short TEMPO_GIRO = 600;                  // 0.6 seg | tempo de giro do servo
const short TEMPO_ESPERA = 1000;               // 1 seg   | tempo de espera para estabilização
const short INTERVALO_BUZZER = 2000;
long tempoEntreCiclosIrrigacao = 1000 * 5;           // 5 seg
long tempoEntreCiclosFertilizacao = 1000 * 60 * 60;  // 5 seg

short duracaoIrrigacao = 1000 * 3;     //3 seg
short duracaoFertilizacao = 1000 * 3;  //3 seg

// Controle do servo
short idx_ang = 0;                                          // índice do ângulo atual
const short ANGS[3] = { 90, 0, 180 };                       // ângulos de parada da plataforma
const short TAMANHO_ANGS = sizeof(ANGS) / sizeof(ANGS[0]);  // quantidade de ângulos

// Valores de referência do sensor de umidade
const short UMIDADE_AR = 558;    // leitura do sensor no ar (seco)
const short UMIDADE_AGUA = 258;  // leitura do sensor na água (molhado)

const short PORCENT_UMIDADE_MINIMA = 40;  // limite mínimo de umidade aceitável
short umidade = 0;                        // valor bruto da leitura de umidade

bool deveIrrigar = false;     // controle de ativação da irrigação
bool deveFertilizar = false;  // controle de ativação da fertilização

bool espantalhoLigado = true;   // variavel para controlar se o sistema está ligado, false = iniciar o sistema desligado
bool estadoPIR = false;         // variavel para o estado do sensor
bool estadoBtnAnterior = HIGH;  // variavel para estado salvar o estado anterior do botão

unsigned long tempoAnteriorBotao = 0;
const unsigned long intervaloBotao = 200;  // variavel de debounce para evitar cliques duplos do botao

unsigned long tempoAnteriorPIR = 0;
const unsigned long INTERVALO_PIR = 50;  // 0.05 seg | Intervalo entre as leituras do sensor e prints

// variáveis para controlar a alternância do som (bip)
unsigned long tempoAnteriorBuzzer = 0;

bool estadoBuzzer = false;
const short volumeMaximo = 255;

// Escreve texto no LCD, com quebra automática de linha ou posição específica
void escreverNoLCD(String txt, short linha = -1) {
  if (linha < 0) {
    const char delimiter[] = " ";      // separador
    const char *txtBuf = txt.c_str();  // conversão de String pra char[]

    char *token = strtok(txtBuf, delimiter);  // separa as palavras

    String frases[TAMANHO_LCD[1]] = {};  // frases por linha
    short ultimoIdx = 0;                 // número de linhas

    String frase = "";

    // separa as frases
    while (token != NULL) {
      String novaFrase = frase + " " + token;
      novaFrase.trim();  // limpa espações inúteis

      if (novaFrase.length() <= TAMANHO_LCD[0]) {
        frase = novaFrase;
      } else {
        frases[ultimoIdx] = frase;
        frase = token;
        ultimoIdx++;
      }

      token = strtok(NULL, delimiter);  // break
    }

    if (frase != "")  // caso de uma linha que não foi pra o array
    {
      frases[ultimoIdx] = frase;
      ultimoIdx++;
    }

    // escreve as frases como der
    for (short i = 0; i < ultimoIdx; i++) {
      escreverNoLCD(frases[i], i);
    }
  } else {
    short tamanhoTxt = txt.length();
    short pos = (TAMANHO_LCD[0] - tamanhoTxt) / 2;  // centraliza o texto

    if (txt != conteudoLCD[linha]) {

      lcd.setCursor(0, linha);
      String espacos = "";
      for (short i = 0; i < 16; i++) espacos += " ";
      lcd.print(espacos);

      conteudoLCD[linha] = txt;
    }

    lcd.setCursor(pos, linha);
    lcd.print(conteudoLCD[linha]);
  }
}

unsigned long ultimoEnvio = 0;
short ultimoLugarFila = 0;
void enviarApp(String data = "") {
  long agora = millis();
  if (data == "") {
    if (agora - ultimoEnvio > 1200) {
      Serial.println(filaMensagens[0]);
      for (short i = 1; i <= ultimoLugarFila; i++) {
        filaMensagens[i - 1] = filaMensagens[i];
      }
      ultimoLugarFila--;
      ultimoEnvio = agora;
    }
  } else {
    filaMensagens[ultimoLugarFila++] = data;
  }
}

// Inicia o ciclo de irrigação ou fertilização
bool enviouIR = false;
void irrigar() {
  deveIrrigar = true;
  if (!enviouIR) {
    enviarApp("IR:1");
    enviouIR = true;
  }
}
bool enviouFE = false;
void fertilizar() {
  deveFertilizar = true;
  if (!enviouFE) {
    enviarApp("FE:1");
    enviouFE = true;
  }
}

void iniciarCicloIrrigacao(
  short segs,                           // tempo que a válvula fica ligada
  long agora,                           // millis atual
  short pinoFonte = pinoIrrigacao,      // pino do rele utilizado
  bool &gatilhoAtivacao = deveIrrigar,  // controle de ativação do ciclo
  unsigned long &registroTempo = ultimaChecagemIrrigacao) {
  switch (acaoAtual) {
    case GIRAR_SERVO:
      if (idx_ang >= TAMANHO_ANGS) {  // fim de um ciclo completo
        idx_ang = 0;                  // reinicia posição
        gatilhoAtivacao = false;      // encerra ciclo
      }

      plataforma.write(ANGS[idx_ang]);  // gira para o próximo ângulo
      idx_ang++;

      acaoAtual = ESPERAR_SERVO;  // próxima etapa
      registroTempo = agora;
      break;

    case ESPERAR_SERVO:  // espera o servo terminar o movimento
      if (agora - registroTempo >= TEMPO_GIRO) {
        acaoAtual = PARAR_SERVO;
        registroTempo = agora;
      }
      break;

    case PARAR_SERVO:  // garante que o servo pare antes de ligar a válvula
      if (agora - registroTempo >= TEMPO_ESPERA) {
        acaoAtual = LIGAR_FONTE;
        registroTempo = agora;
      }
      break;

    case LIGAR_FONTE:                 // liga o rele da válvula
      digitalWrite(pinoFonte, HIGH);  // liga bomba

      if (agora - registroTempo >= segs) {
        acaoAtual = DESLIGAR_FONTE;
        registroTempo = agora;
      }
      break;

    case DESLIGAR_FONTE:             // desliga a válvula
      digitalWrite(pinoFonte, LOW);  // desliga bomba
      acaoAtual = ESPERAR_BOMBA;
      registroTempo = agora;
      break;

    case ESPERAR_BOMBA:  // espera o fluxo de água parar completamente
      if (agora - registroTempo >= TEMPO_ESPERA) {
        acaoAtual = GIRAR_SERVO;
        registroTempo = agora;
      }
      break;
  }
}


void verificarBotao() {
  unsigned long tempoAtual = millis();
  int estadoBtnAtual = digitalRead(pinoButton);  // estado atual do botão

  if (!estadoBtnAtual && estadoBtnAnterior)  // comparação dos estados do botão para ligar ou desligar o sistema
  {
    if (tempoAtual - tempoAnteriorBotao >= intervaloBotao) {
      espantalhoLigado = !espantalhoLigado;
      tempoAnteriorBotao = tempoAtual;
      Serial.println(estadoBtnAtual);
    }
  }
  estadoBtnAnterior = estadoBtnAtual;
}

// Função para gerenciar a detecção de movimento e o pinoBuzzer

void monitorarSensor() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnteriorPIR >= INTERVALO_PIR) {
    tempoAnteriorPIR = tempoAtual;

    estadoPIR = digitalRead(pinoSensor_PIR);
    if (estadoPIR) {
      estadoBuzzer = HIGH;
    }
  }
}

bool enviouES = false;
void ativarEspantalho() {
  unsigned long tempoAtual = millis();
  if (!enviouES) {
    enviarApp("ES:1");
    enviouES = true;
  }

  // Lógica que alterna o estado do pinoBuzzer a cada 'intervaloBuzzer'
  if (estadoBuzzer && tempoAtual - tempoAnteriorBuzzer >= INTERVALO_BUZZER) {
    estadoBuzzer = LOW;
    enviouES = false;
    tempoAnteriorBuzzer = tempoAtual;
  }


  analogWrite(pinoBuzzer, estadoBuzzer ? volumeMaximo : 0);
}
void desativarEspantalho() {
  estadoBuzzer = LOW;
  digitalWrite(pinoBuzzer, estadoBuzzer);
}

void lerApp() {
  if (Serial.available()) {
    String input = Serial.readStringUntil("\n");
    int index = input.indexOf(":");
    if (index != -1) {
      String inputData[2];
      int StringCount = 0;

      while (input.length() > 0) {
        if (index == -1) {
          inputData[StringCount++] = input;
          break;
        } else {
          inputData[StringCount++] = input.substring(0, index);
          input = input.substring(index + 1);
        }
      }

      if (inputData[0] == "IR") {
        irrigar();
        iniciarCicloIrrigacao(inputData[1].toInt() * 1000, millis());
      } else if (inputData[0] == "RR") {
        if (inputData[1] == "RE") {
          espantalhoLigado = true;
        } else if (inputData[1] == "EN") {
          espantalhoLigado = false;
        }
      } else if (inputData[0] == "TF") {
        fertilizar();
        iniciarCicloIrrigacao(inputData[1].toInt() * 1000, millis(), pinoFertilizacao, deveFertilizar);
      } else if (inputData[0] == "AII") {
        tempoEntreCiclosIrrigacao = inputData[1].toFloat() * 60 * 1000;
        Serial.println(String(tempoEntreCiclosIrrigacao));
      } else if (inputData[0] == "AIF") {
        tempoEntreCiclosFertilizacao = inputData[1].toFloat() * 60 * 1000;
        Serial.println(tempoEntreCiclosFertilizacao);
      }
    }
  }
}

void setup() {
  plataforma.attach(pinoPlataforma);  // inicializa o servo
  plataforma.write(ANGS[0]);

  pinMode(pinoIrrigacao, OUTPUT);
  digitalWrite(pinoIrrigacao, LOW);  // inicia desligado

  pinMode(pinoFertilizacao, OUTPUT);
  digitalWrite(pinoFertilizacao, LOW);  // inicia desligado

  pinMode(pinoSensorUmidade, INPUT);  // inicializa sensor de umidade

  pinMode(pinoSensor_PIR, INPUT);
  pinMode(pinoBuzzer, OUTPUT);
  pinMode(pinoButton, INPUT_PULLUP);

  lcd.begin(TAMANHO_LCD[0], TAMANHO_LCD[1]);
  lcd.init();

  lcd.backlight();    // liga iluminação do LCD
  lcd.leftToRight();  // define direção do texto

  escreverNoLCD("Monitor de umidade do solo");
  Serial.begin(9600);
  delay(1000);
}

void loop() {
  unsigned long agora = millis();

  //leitura de umidade quando o sistema está ocioso
  if (!deveIrrigar && !deveFertilizar && agora - ultimaChecagemIrrigacao > tempoEntreCiclosIrrigacao) {
    escreverNoLCD("Nivel de Umidade:", 0);
    umidade = analogRead(pinoSensorUmidade);                                  // leitura do sensor
    int porcentagemUmidade = map(umidade, UMIDADE_AR, UMIDADE_AGUA, 0, 100);  // converte para porcentagem
    porcentagemUmidade = constrain(porcentagemUmidade, 0, 100);               // limita entre 0 e 100

    enviarApp("UM:" + String(porcentagemUmidade));

    if (porcentagemUmidade > 85) {
      escreverNoLCD(String(porcentagemUmidade) + "%|ENCHARCADO", 1);
    } else if (70 < porcentagemUmidade && porcentagemUmidade <= 85) {
      escreverNoLCD(String(porcentagemUmidade) + "%|MUITO UMIDO", 1);
    } else if (PORCENT_UMIDADE_MINIMA < porcentagemUmidade && porcentagemUmidade <= 70) {
      escreverNoLCD(String(porcentagemUmidade) + "%|UMIDO", 1);
    } else if (porcentagemUmidade <= PORCENT_UMIDADE_MINIMA) {
      escreverNoLCD(String(porcentagemUmidade) + "%|SECO", 1);
      enviouIR = false;
      irrigar();  // ativa irrigação
    }
    ultimaChecagemIrrigacao = agora;
  }

  if (!deveIrrigar && !deveFertilizar && agora - ultimaChecagemFertilizacao > tempoEntreCiclosFertilizacao) {
    enviouFE = false;
    fertilizar();
  }
  // ciclo de irrigação
  if (deveIrrigar && !deveFertilizar) {
    iniciarCicloIrrigacao(duracaoIrrigacao, agora);
    escreverNoLCD("Irrigando...", 0);
  }
  // ciclo de fertilização
  else if (!deveIrrigar && deveFertilizar) {
    iniciarCicloIrrigacao(duracaoFertilizacao, agora, pinoFertilizacao, deveFertilizar,ultimaChecagemFertilizacao);
    escreverNoLCD("Fertilizando...", 0);
  }
  // prioridade: irrigação antes da fertilização
  else if (deveIrrigar && deveFertilizar) {
    deveIrrigar = false;
  }

  verificarBotao();

  if (!deveIrrigar && !deveFertilizar && espantalhoLigado) {
    monitorarSensor();
    if (estadoBuzzer) {
      ativarEspantalho();
    }
  } else {
    desativarEspantalho();
  }

  lerApp();
  if (filaMensagens[0].indexOf(":") > -1) {
    enviarApp();
  }
}