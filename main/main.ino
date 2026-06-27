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
unsigned long ultimaChecagemIrrigacao = 0;                     // controle de tempo da irrigação
unsigned long ultimaChecagemFertilizacao = 0;                  // controle de tempo da fertilização
const short TEMPO_GIRO = 600;                                  // 0.6 seg | tempo de giro do servo
const short TEMPO_ESPERA = 1000;                               // 1 seg   | tempo de espera para estabilização
const short TEMPO_ENTRE_CICLOS_IRRIGACAO = 1000 * 5;           // 5 seg
const short TEMPO_ENTRE_CICLOS_FERTILIZACAO = 1000 * 60 * 60;  // 5 seg

short duracaoIrrigacao = 1000 * 3;     //3 seg
short duracaoFertilizacao = 1000 * 3;  //3 seg

// Controle do servo
short idx_ang = 0;                                          // índice do ângulo atual
const short ANGS[3] = { 0, 90, 180 };                       // ângulos de parada da plataforma
const short TAMANHO_ANGS = sizeof(ANGS) / sizeof(ANGS[0]);  // quantidade de ângulos

// Valores de referência do sensor de umidade
const short UMIDADE_AR = 558;    // leitura do sensor no ar (seco)
const short UMIDADE_AGUA = 258;  // leitura do sensor na água (molhado)

const short PORCENT_UMIDADE_MINIMA = 40;  // limite mínimo de umidade aceitável
short umidade = 0;                        // valor bruto da leitura de umidade

bool deveIrrigar = false;     // controle de ativação da irrigação
bool deveFertilizar = false;  // controle de ativação da fertilização

bool espantalhoLigado = false;  // variavel para controlar se o sistema está ligado, false = iniciar o sistema desligado
bool estadoPIR = false;         // variavel para o estado do sensor
bool estadoBtnAnterior = HIGH;  // variavel para estado salvar o estado anterior do botão

unsigned long tempoAnteriorBotao = 0;
const unsigned long intervaloBotao = 200;  // variavel de debounce para evitar cliques duplos do botao

unsigned long tempoAnteriorPIR = 0;
const unsigned long INTERVALO_PIR = 50;  // 0.05 seg | Intervalo entre as leituras do sensor e prints

// variáveis para controlar a alternância do som (bip)
unsigned long tempoAnteriorBuzzer = 0;
const unsigned long intervaloBuzzer = 200;  // 0.2 seg | Define a velocidade do bip (200ms ligado, 200ms desligado)

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

// Inicia o ciclo de irrigação ou fertilização
bool enviouIR = false;
void irrigar() {
  deveIrrigar = true;
  if (!enviouIR) {
    Serial.println("IR:1");
    enviouIR = true;
  }
}
bool enviouFE = false;
void fertilizar() {
  deveFertilizar = true;
  if (!enviouFE) {
    Serial.println("FE:1");
    enviouFE = true;
  }
}

void iniciarCicloIrrigacao(
  short segs,                           // tempo que a válvula fica ligada
  long agora,                           // millis atual
  short pinoFonte = pinoIrrigacao,      // pino do rele utilizado
  bool &gatilhoAtivacao = deveIrrigar  // controle de ativação do ciclo
) {
  switch (acaoAtual) {
    case GIRAR_SERVO:
      if (idx_ang >= TAMANHO_ANGS) {  // fim de um ciclo completo
        idx_ang = 0;                  // reinicia posição
        gatilhoAtivacao = false;      // encerra ciclo
      }

      plataforma.write(ANGS[idx_ang]);  // gira para o próximo ângulo
      idx_ang++;

      acaoAtual = ESPERAR_SERVO;  // próxima etapa
      ultimaChecagemIrrigacao = agora;
      break;

    case ESPERAR_SERVO:  // espera o servo terminar o movimento
      if (agora - ultimaChecagemIrrigacao >= TEMPO_GIRO) {
        acaoAtual = PARAR_SERVO;
        ultimaChecagemIrrigacao = agora;
      }
      break;

    case PARAR_SERVO:  // garante que o servo pare antes de ligar a válvula
      if (agora - ultimaChecagemIrrigacao >= TEMPO_ESPERA) {
        acaoAtual = LIGAR_FONTE;
        ultimaChecagemIrrigacao = agora;
      }
      break;

    case LIGAR_FONTE:                 // liga o rele da válvula
      digitalWrite(pinoFonte, HIGH);  // liga bomba

      if (agora - ultimaChecagemIrrigacao >= segs) {
        acaoAtual = DESLIGAR_FONTE;
        ultimaChecagemIrrigacao = agora;
      }
      break;

    case DESLIGAR_FONTE:             // desliga a válvula
      digitalWrite(pinoFonte, LOW);  // desliga bomba
      acaoAtual = ESPERAR_BOMBA;
      ultimaChecagemIrrigacao = agora;
      break;

    case ESPERAR_BOMBA:  // espera o fluxo de água parar completamente
      if (agora - ultimaChecagemIrrigacao >= TEMPO_ESPERA) {
        acaoAtual = GIRAR_SERVO;
        ultimaChecagemIrrigacao = agora;
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
    }
  }
  estadoBtnAnterior = estadoBtnAtual;
}

// Função para gerenciar a detecção de movimento e o pinoBuzzer
bool enviouES = false;
unsigned long tempoAnteriorEnviouES = 0;
void monitorarSensor() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnteriorPIR >= INTERVALO_PIR) {
    if (tempoAtual - tempoAnteriorEnviouES >= 3000) {
      enviouES = false;
    }
    tempoAnteriorPIR = tempoAtual;

    estadoPIR = digitalRead(pinoSensor_PIR);
    if (estadoPIR) {
      estadoBuzzer = HIGH;

      tempoAnteriorBuzzer = tempoAtual;
    }
  }

  // Lógica que alterna o estado do pinoBuzzer a cada 'intervaloBuzzer'
  if (estadoBuzzer && tempoAtual - tempoAnteriorBuzzer >= intervaloBuzzer) {
    estadoBuzzer = LOW;
    enviouES = true;
  }


  analogWrite(pinoBuzzer, estadoBuzzer ? volumeMaximo : 0);
}

void desativarEspantalho() {
  estadoBuzzer = LOW;
  digitalWrite(pinoBuzzer, estadoBuzzer);
}

void lerApp() {
  if (Serial.available()) {
    escreverNoLCD("conectado");
    String t = Serial.readStringUntil("\n");
    escreverNoLCD(t, 1);
  }
}

void setup() {
  plataforma.attach(pinoPlataforma);  // inicializa o servo
  plataforma.write(0);

  pinMode(pinoIrrigacao, OUTPUT);
  digitalWrite(pinoIrrigacao, LOW);  // inicia desligado

  pinMode(pinoFertilizacao, OUTPUT);
  digitalWrite(pinoFertilizacao, LOW);  // inicia desligado

  pinMode(pinoSensorUmidade, INPUT);  // inicializa sensor de umidade

  pinMode(pinoSensor_PIR, INPUT);
  pinMode(pinoBuzzer, OUTPUT);
  pinMode(pinoButton, INPUT);

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
  if (!deveIrrigar && !deveFertilizar && agora - ultimaChecagemIrrigacao > TEMPO_ENTRE_CICLOS_IRRIGACAO) {
    escreverNoLCD("Nivel de Umidade:", 0);
    umidade = analogRead(pinoSensorUmidade);                                  // leitura do sensor
    int porcentagemUmidade = map(umidade, UMIDADE_AR, UMIDADE_AGUA, 0, 100);  // converte para porcentagem
    porcentagemUmidade = constrain(porcentagemUmidade, 0, 100);               // limita entre 0 e 100

    Serial.println("UM:" + String(porcentagemUmidade));

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

  if (!deveIrrigar && !deveFertilizar && agora - ultimaChecagemFertilizacao > TEMPO_ENTRE_CICLOS_FERTILIZACAO) {
    enviouFE = false;
    fertilizar();
  }
  // ciclo de irrigação
  if (deveIrrigar && !deveFertilizar) {
    iniciarCicloIrrigacao(duracaoIrrigacao, agora);
    escreverNoLCD("Irrigando...", 0);
    // ultimaChecagemIrrigacao = agora;
  }
  // ciclo de fertilização
  else if (!deveIrrigar && deveFertilizar) {
    iniciarCicloIrrigacao(duracaoFertilizacao, agora, pinoFertilizacao, deveFertilizar);
    escreverNoLCD("Fertilizando...", 0);
    ultimaChecagemIrrigacao = agora;
    ultimaChecagemFertilizacao = agora;
  }
  // prioridade: irrigação antes da fertilização
  else if (deveIrrigar && deveFertilizar) {
    deveIrrigar = false;
  }

  verificarBotao();

  if (!deveIrrigar && !deveFertilizar && espantalhoLigado) {
    monitorarSensor();

  } else {
    desativarEspantalho();
  }

  lerApp();
}