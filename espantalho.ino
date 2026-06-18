#define LED_indicador 7
#define sensor_PIR 2
#define buzzer 4
#define button 5

bool sistema_ligado = false; // variavel para controlar se o sistema está ligado, false = iniciar o sistema desligado
bool estado_sensor = false; // variavel para o estado do sensor
bool estado_btn_anterior = HIGH; // variavel para estado salvar o estado anterior do botão  

unsigned long tempoAnteriorBotao = 0;
const unsigned long intervaloBotao = 200; // variavel de debounce para evitar cliques duplos do botao

unsigned long tempoAnteriorSensor = 0;
const unsigned long intervaloSensor = 50; // Intervalo entre as leituras do sensor e prints

// variáveis para controlar a alternância do som (bip)
unsigned long tempoAnteriorBuzzer = 0;
const unsigned long intervaloBuzzer = 200; // Define a velocidade do bip (200ms ligado, 200ms desligado)
bool estadoBuzzerAlternado = LOW;

void setup()
{
    pinMode(sensor_PIR, INPUT); 
    pinMode(buzzer, OUTPUT);
    pinMode(button, INPUT_PULLUP);
    pinMode(LED_indicador, OUTPUT);
    Serial.begin(9600);
}

void loop()
{
    verificarBotao();
    monitorarSensor();
}

// Função para gerenciar o estado do sistema via botão
void verificarBotao() {
    unsigned long tempoAtual = millis();
    int estadoBtnAtual = digitalRead(button); // estado atual do botão 
    
    if (estadoBtnAtual == LOW && estado_btn_anterior == HIGH) // comparação dos estados do botão para ligar ou desligar o sistema
    {
        if (tempoAtual - tempoAnteriorBotao >= intervaloBotao)
        {
            sistema_ligado = !sistema_ligado;
            tempoAnteriorBotao = tempoAtual; 
        }
    }
    estado_btn_anterior = estadoBtnAtual;
}

// Função para gerenciar a detecção de movimento e o buzzer
void monitorarSensor() {
    unsigned long tempoAtual = millis();
    
    if (tempoAtual - tempoAnteriorSensor >= intervaloSensor)
    {
        tempoAnteriorSensor = tempoAtual; 

        if (sistema_ligado) // sistema ligado - lê o estado do sensor e ativa o buzzer se houver movimento
        {
            estado_sensor = digitalRead(sensor_PIR); 

            if (estado_sensor) // se houver leitura
            {
                // Lógica que alterna o estado do buzzer a cada 'intervaloBuzzer'
                if (tempoAtual - tempoAnteriorBuzzer >= intervaloBuzzer)
                {
                    tempoAnteriorBuzzer = tempoAtual;
                    estadoBuzzerAlternado = !estadoBuzzerAlternado;
                }

                digitalWrite(buzzer, estadoBuzzerAlternado); 
                digitalWrite(LED_indicador, HIGH);
                Serial.println("1"); // 1 = movimento detectado
            }
            else 
            {
                digitalWrite(buzzer, LOW);
                estadoBuzzerAlternado = LOW; // Reseta para começar tocando no próximo movimento
                digitalWrite(LED_indicador, LOW);
                Serial.println("0"); //0 = sem movimento
            }
        }
        else // sistema desligado - ignora qualquer leitura do sensor e garante que o buzzer e o led estejam desligados
        {
            digitalWrite(buzzer, LOW);
            estadoBuzzerAlternado = LOW;
            digitalWrite(LED_indicador, LOW);
        }
    }
}
