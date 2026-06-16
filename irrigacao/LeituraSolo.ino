#include <LiquidCrystal_I2C.h>

#define pinoSensorUmidade A0 // pino do sensor de umidade

// Valores de referência do sensor de umidade
const short UMIDADE_AR = 558;   // leitura do sensor no ar (seco)
const short UMIDADE_AGUA = 258; // leitura do sensor na água (molhado)

const short PORCENT_UMIDADE_MINIMA = 40; // limite mínimo de umidade aceitável

short umidade = 0; // valor bruto da leitura de umidade

bool deveIrrigar = false;    // controle de ativação da irrigação
bool deveFertilizar = false; // controle de ativação da fertilização

// tempo em ms
unsigned long ultimaChecagemIrrigacao = 0; // controle de tempo da leitura

const short TAMANHO_LCD[2] = {16, 2};                        // dimensões do display LCD (colunas, linhas)
LiquidCrystal_I2C lcd(0x27, TAMANHO_LCD[0], TAMANHO_LCD[1]); // endereço e configuração do LCD

// Escreve texto no LCD, com quebra automática ou posição específica
void escreverNoLCD(String txt, short linha = -1)
{
  if (linha < 0)
  {
    const char delimiter[] = " ";     // separador
    const char *txtBuf = txt.c_str(); // conversão de String pra char[]

    char *token = strtok(txtBuf, delimiter); // separa as palavras

    String frases[TAMANHO_LCD[1]] = {}; // frases por linha
    short ultimoIdx = 0;                // número de linhas

    String frase = "";

    // separa as frases
    while (token != NULL)
    {
      String novaFrase = frase + " " + token;
      novaFrase.trim(); // limpa espações inúteis

      if (novaFrase.length() <= TAMANHO_LCD[0])
      {
        frase = novaFrase;
      }
      else
      {
        frases[ultimoIdx] = frase;
        frase = token;
        ultimoIdx++;
      }

      token = strtok(NULL, delimiter); // break
    }

    if (frase != "") // caso de uma linha que não foi pra o array
    {
      frases[ultimoIdx] = frase;
      ultimoIdx++;
    }

    // escreve as frases como der
    for (short i = 0; i < ultimoIdx; i++)
    {
      escreverNoLCD(frases[i], i);
    }
  }
  else
  {
    short tamanhoTxt = txt.length();
    short pos = (TAMANHO_LCD[0] - tamanhoTxt) / 2; // centraliza o texto
    lcd.setCursor(pos, linha);
    lcd.print(txt);
  }
}

// Realiza a leitura do solo e classifica o nível de umidade
void aferirSolo(
    short umidade_ar,  // valor de referência do ar (seco)
    short umidade_agua // valor de referência da água (submerso)
)
{
  lcd.clear();
  escreverNoLCD("Nivel de Umidade:", 0);

  short leitura = analogRead(pinoSensorUmidade); // leitura do sensor

  int porcentagemUmidade = map(leitura, umidade_ar, umidade_agua, 0, 100); // converte para porcentagem
  porcentagemUmidade = constrain(porcentagemUmidade, 0, 100);              // limita entre 0 e 100

  if (porcentagemUmidade > 85)
  {
    escreverNoLCD(String(porcentagemUmidade) + "%|ENCHARCADO", 1);
  }
  else if (70 < porcentagemUmidade && porcentagemUmidade <= 85)
  {
    escreverNoLCD(String(porcentagemUmidade) + "%|MUITO UMIDO", 1);
  }
  else if (PORCENT_UMIDADE_MINIMA < porcentagemUmidade && porcentagemUmidade <= 70)
  {
    escreverNoLCD(String(porcentagemUmidade) + "%|UMIDO", 1);
  }
  else if (porcentagemUmidade <= PORCENT_UMIDADE_MINIMA)
  {
    lcd.clear();
    escreverNoLCD(String(porcentagemUmidade) + "%|SECO", 1);

    deveIrrigar = true; // ativa irrigação quando solo está seco
  }
}

void setup()
{
  pinMode(pinoSensorUmidade, INPUT); // inicializa sensor de umidade

  lcd.begin(TAMANHO_LCD[0], TAMANHO_LCD[1]);
  lcd.init();

  lcd.backlight();   // liga iluminação do LCD
  lcd.leftToRight(); // define direção do texto

  escreverNoLCD("Monitor de umidade do solo");

  Serial.begin(9600);
}

void loop()
{
  unsigned long agora = millis();

  // realiza leitura periódica quando o sistema está ocioso
  if (!deveIrrigar && agora - ultimaChecagemIrrigacao > 1000)
  {
    aferirSolo(UMIDADE_AR, UMIDADE_AGUA);

    ultimaChecagemIrrigacao = agora; // atualiza tempo da última leitura
  }
}