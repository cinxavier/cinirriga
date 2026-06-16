#include <Servo.h>

#define pinoIrrigacao 30    // pino do rele da água
#define pinoFertilizacao 32 // pino do rele do fertilizante
#define pinoPlataforma 34   // pino do servo

Servo plataforma; // servo motor da plataforma

enum Estado // Estados do sistema, passos que a máquina segue
{
  GIRAR_SERVO,
  ESPERAR_SERVO,
  PARAR_SERVO,
  LIGAR_FONTE,
  DESLIGAR_FONTE,
  ESPERAR_BOMBA
};

Estado acaoAtual = GIRAR_SERVO; // a ação que a máquina está fazendo no momento

// tempo em ms
unsigned long ultimaChecagemIrrigacao = 0;
unsigned long ultimaChecagemFertilizacao = 0;
const short TEMPO_GIRO = 800;    // 0.8s
const short TEMPO_ESPERA = 1000; // 1s, o tempo de espera para a água parar de correr depois que a bomba para ou a válvula fecha

// Controle do servo
short idx_ang = 0;                                         // o ângulo atual da plataforma
const short ANGS[] = {0, 60, 120, 180};                    // ângulos que a plataforma vai parar, para alterar as posições das plantas basta mudar aqui
const short TAMANHO_ANGS = sizeof(ANGS) / sizeof(ANGS[0]); // quantidade de angulos pra ela parar

bool deveIrrigar = false;    // controle de ativação da água
bool deveFertilizar = false; // controle de ativação do fertilizante

void setup()
{
  plataforma.attach(pinoPlataforma); // init do servo
  plataforma.write(0);

  pinMode(pinoIrrigacao, OUTPUT);   // setup do rele da água
  digitalWrite(pinoIrrigacao, LOW); // inicia o rele desligado

  pinMode(pinoFertilizacao, OUTPUT); // setup do rele do fertilizante
  digitalWrite(pinoFertilizacao, LOW);

  Serial.begin(9600);
}

void iniciarCicloIrrigacao(
    short segs,                         // tempo em milisegundos que a válvula fica ativada
    long agora,                         // millis() atual
    long &ultimaChecagem,               // ultimo momento que a função foi chamada, tenha uma variável apenas para essa função e a reinicie sempre que a chamar
    short pinoFonte = pinoIrrigacao,    // pino do rele da vávula
    bool &gatilhoAtivacao = deveIrrigar // gatilho de ativação da válvula
)

{
  switch (acaoAtual)
  {
  case GIRAR_SERVO:
    if (idx_ang >= TAMANHO_ANGS) // caso um ciclo tenha terminado
    {
      idx_ang = 0;             // volta a máquina pra o início
      gatilhoAtivacao = false; // para essa função
    }

    plataforma.write(ANGS[idx_ang]); // gira a plataforma para o ângulo definido
    idx_ang++;                       // define o próximo ângulo que a plataforma deve parar

    acaoAtual = ESPERAR_SERVO; // próxima etapa
    ultimaChecagem = agora;    // atualização do tempo
    break;

  case ESPERAR_SERVO: // espera um tempo pra o servo terminar o percurso
    if (agora - ultimaChecagem >= TEMPO_GIRO)
    {
      acaoAtual = LIGAR_FONTE;
      ultimaChecagem = agora;
    }
    break;

    // essa etapa é para ter certeza que o servo só vai girar quando a válvula estiver fechada
    // case PARAR_SERVO:
    //   if (agora - ultimaChecagem >= TEMPO_ESPERA)
    //   {
    //     acaoAtual = LIGAR_FONTE;
    //     ultimaChecagem = agora;
    //   }
    //   break;

  case LIGAR_FONTE:                // liga o rele da válvula pelo tempo que por dado
    digitalWrite(pinoFonte, HIGH); // liga bomba

    if (agora - ultimaChecagem >= segs)
    {
      acaoAtual = DESLIGAR_FONTE;
      ultimaChecagem = agora;
    }
    break;

  case DESLIGAR_FONTE:            // desliga a válvula
    digitalWrite(pinoFonte, LOW); // desliga bomba
    acaoAtual = ESPERAR_BOMBA;
    ultimaChecagem = agora;
    break;

  case ESPERAR_BOMBA: // espera a água parar totalmente
    if (agora - ultimaChecagem >= TEMPO_ESPERA)
    {
      acaoAtual = GIRAR_SERVO;
      ultimaChecagem = agora;
    }
    break;
  }
}

void loop()
{
  unsigned long AGORA = millis();
  iniciarCicloIrrigacao(3000, AGORA);
}