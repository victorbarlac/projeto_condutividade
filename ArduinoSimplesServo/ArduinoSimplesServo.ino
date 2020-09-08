// =====================================================================
// INCLUSÃO DE BIBLIOTECAS USADAS PELO PROJETO
// =====================================================================

#include <SoftwareSerial.h>
#include <Servo.h>
#include <Ticker.h>

// =====================================================================
// CONSTANTES, VARIÁVEIS E OBJETOS GLOBAIS NÃO DEPENDENTES
// =====================================================================

// Indicador de porta RX para Software Serial
const uint8_t SS_PORTA_RX = 2;
// Indicador de porta TX para Software Serial
const uint8_t SS_PORTA_TX = 3;
// Valor de milissegundos padrão para eventos do temporizador
const uint16_t INTERVALO_PADRAO_TEMPORIZADOR = 50;
// Constante com a velocidade das comunicações seriais
const uint16_t VELOCIDADE_SERIAL = 38400;
// Recipiente com a porta em que o sensor está conectado
const uint8_t PORTA_SENSOR = A0;
// Recipiente com a porta em que o servo está conectado
const uint8_t PORTA_SERVO = 9;
// Constante de segundos para processamento do sensor com fim de coleta de dados
const uint8_t INTERVALO_COLETA = 10;
// Indicador de posição de repouso do servo
const uint8_t POSICAO_SERVO_REPOUSO = 0;
// Indicador de posição de trabalho do servo
const uint8_t POSICAO_SERVO_TRABALHO = 0;

// Controlador do Servo motor
Servo controlador;
// Software serial para comunicação com ESP8266
SoftwareSerial esp(SS_PORTA_RX, SS_PORTA_TX); // RX, TX

// Contador de segundos genérico
uint8_t segundos = 0;
// Contador de milissegundos temporário
uint16_t milissegundos = 0;
// Indicador de pedido de leitura
bool executar_leitura = false;
// Indicador de fase da leitura
uint8_t fase_leitura = 0;
// Recipiente com a leitura do sensor
uint16_t leitura_sensor = 0;

// =====================================================================
// FUNÇÕES DE TEMPO
// =====================================================================

// Contabiliza os segundos passados de um intervalo a outro
void contabilizar_tempo()
{
    milissegundos += INTERVALO_PADRAO_TEMPORIZADOR;
    if(milissegundos >= 1000)
    {
        segundos += milissegundos / 1000;
        milissegundos = milissegundos % 1000;
    }
}

// Executa rotinas temporizadas
void rotinas_temporizadas()
{
    contabilizar_tempo();
}

// Temporizador
Ticker temporizador(rotinas_temporizadas, INTERVALO_PADRAO_TEMPORIZADOR);

// =====================================================================
// FUNÇÕES DE COMUNICAÇÃO INFORMATIVA
// =====================================================================

// Inicia uma mensagem comum que não deve ser encarada como comando
void iniciar_mensagem_comum()
{
    Serial.print(F("[UNO]: "));
}

// Informa algum dado genérico com texto antes e depois
void informar(String pre, String dado, String pos)
{
    iniciar_mensagem_comum();
    Serial.print(pre);
    Serial.print(dado);
    Serial.println(pos);
}

// Informa alguma mensagem simples
void informar_simples(String mensagem)
{
    iniciar_mensagem_comum();
    Serial.println(mensagem);
}

// =====================================================================
// FUNÇÕES DE CONTROLE DO SERVO
// =====================================================================

// Recolhe o servo para evitar que o sensor fique em contato por muito tempo com a amostra
// (durante os teste dos diferentes materiais dos sensores, foi percebido que todos eles
// tiveram algum tipo de degradação quando ficaram em contato prolongado com as amostras,
// resultando em alteração química da amostra e do aspecto físico do próprio sensor)
void repousar_servo()
{
    controlador.write(POSICAO_SERVO_REPOUSO);
}

// Envia o servo para a posição de leitura que deve ser mantida pelo menor tempo possível
void enviar_servo()
{
    controlador.write(POSICAO_SERVO_TRABALHO);
}

// Configura o controlador do servo e o coloca na posição de repouso como ato inicial,
// esperando a conclusão do movimento
void inicializar_servo()
{
    // Zere o contador de segundos para realizar uma contagem completa
    // Indique ao controlador em qual porta o motor servo está conectado
    controlador.attach(PORTA_SERVO);
    // Mova o servo para a posição inicial
    repousar_servo();
    // Espere um tempo suficiente para o servo se posicionar
    delay(5000);
}

// =====================================================================
// FUNÇÕES DE COMUNICAÇÃO IMPERATIVA
// =====================================================================

// Envia um comando ao ESP8266
void enviar_comando(String conteudo)
{
    informar_simples("ENV CMD");
    Serial.print('>');
    Serial.println(conteudo);
    esp.print('>');
    esp.println(conteudo);
}

// Interpreta uma comunicação enviada pelo ESP8266 e executa as devidas ações associadas a elas
void tratar_comunicacao(String base)
{
    // Verifica se há algo a ser tratado
    if(base.length() > 0)
    {
        // Verifica se a comunicação recebida se refere a um comando
        if(base.charAt(0) == '>')
        {
            // Comando de execução de leitura
            if(base.indexOf("leitura") == 1)
            {
                // Agende uma leitura do sensor
                executar_leitura = true;
            }
            // Se o comando não estiver catalogado, acuse erro
            else
            {
                informar_simples("CMD DESCONHECIDO");
            }
        }
    }
}

// Ouve as comunicações advindas do ESP8266
String ouvir_esp()
{
    // Recipiente da comunicação recebida
    String esp_disse = "";
    // Se houver algo que foi comunicado
    if(esp.available())
    {
        // Consiga o conteúdo da comunicação
        esp_disse = esp.readStringUntil('\n');
        // E mostre ao terminal de depuração
        informar("ESP DISSE: ", esp_disse, "");
    }
    return esp_disse;
}

// Ouve as comunicações advindas do PC
String ouvir_pc()
{
    // Recipiente da comunicação recebida
    String pc_disse = "";
    // Se houver algo que foi comunicado
    if(Serial.available())
    {
        // Consiga o conteúdo da comunicação
        pc_disse  = Serial.readString();
        // E mostre ao terminal de depuração
        informar("PC DISSE: ", pc_disse, "");
    }
    return pc_disse;
}

// =====================================================================
// FUNÇÕES DO PROGRAMA PRINCIPAL
// =====================================================================

// Realiza os procedimentos padrões para conseguir a leitura do sensor
void procedimentos_de_leitura()
{
    // Templates de informação
    String mensagem = "PROC LEITURA ";
    // Realize a leitura de acordo com as fases planejadas
    switch(fase_leitura)
    {
    // Mover o servo de 0 a 90 graus
    case 0:
        enviar_servo();
        fase_leitura++;
        segundos = 0;
        informar(mensagem, String(0), "");
        break;
    // Esperar 5 segundos para concluir o posicionamento do servo
    case 1:
        if(segundos > 5)
        {
            fase_leitura++;
            segundos = 0;
            informar(mensagem, String(1), "");
        }
        break;
    // Execute a leitura por um certo tempo
    case 2:
        leitura_sensor = analogRead(PORTA_SENSOR);
        if(segundos > 20)
        {
            fase_leitura++;
            segundos = 0;
            informar(mensagem, String(2), "");
        }
        break;
    // Ao fim da leitura, recolha o sensor, espere seu retorno, zere o contador de segundos e
    // indique o reinício do processo de leitura, bem como 
    case 3:
        repousar_servo();
        if(segundos > 10)
        {
            segundos = 0;
            fase_leitura = 0;
            executar_leitura = false;
            informar(mensagem, String(3), "");
            informar_simples(mensagem + "FIM");
            // Envie o valor da leitura ao ESP8266
            enviar_comando("sensor " + String(leitura_sensor));
        }
        break;
    }
}

// Configura o Arduino para utilização
void setup()
{
    // Informe o início da configuração
    informar_simples("INIC CONFIG");
    // Configuração de tempo
    temporizador.start();
    // Configure a porta onde o sensor está ligado como entrada de dados
    pinMode(PORTA_SENSOR, INPUT_PULLUP);
    // Inicialize o motor servo
    inicializar_servo();
    // Inicie as comunicações seriais com o módulo ESP8266 e com o terminal de depuração
    Serial.begin(VELOCIDADE_SERIAL);
    esp.begin(VELOCIDADE_SERIAL);
    // Informe o fim da configuração
    informar_simples("FIM CONFIG");
}

// PROGRAMA PRINCIPAL
void loop()
{
    // Contabiliza o tempo passado
    temporizador.update();
    // Enquanto tiver algo a ser lido pela comunicação serial com o ESP8266...
    // Trata a informação enviada pelo ESP
    tratar_comunicacao(ouvir_esp());
    tratar_comunicacao(ouvir_pc());
    // Verifique se uma leitura foi agendada
    if(executar_leitura == true)
    {
        procedimentos_de_leitura();
    }
}
