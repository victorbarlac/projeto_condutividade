/*

2019-2020
PROJETO DO IFES CAMPUS ITAPINA EM PARCERIA COM O CAMPUS COLATINA
FEITO POR:
--JULIO CESAR GOLDNER VENDRAMINI
--ROBSON PRUCOLI POSSE
--SABRINA GOBBI SCALDAFERRO
--VICTOR BARCELOS LACERDA

*/

// ========================================================================
// INCLUSÃO DE BIBLIOTECAS
// ========================================================================

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>

// ========================================================================
// CONSTANTES, OBJETOS E VARIÁVEIS GLOBAIS SEM DEPENDÊNCIA
// ========================================================================

// Constante de porta padrão para servidores HTTP
const uint16_t PORTA_SERVIDOR = 80;
// Constante de porta padrão para servidores HTTP
const uint16_t PORTA_SERVIDOR_S = 443;
// Constante com o SSID da rede a se conectar o ESP8266
const char * REDE_SSID = "Lacerda Net";
// Constante com a senha da rede a se conectar o ESP8266
const char * REDE_SENHA = "87654321a";
// Constante com a velocidade de comunicação via Serial
const uint32_t VELOCIDADE_SERIAL = 38400;
// Constante com o ID do canal na plataforma ThingSpeak
const char * ID_CANAL_TS = "1051066";
// Constante com a chave de escrita do canal da plataforma ThingSpeak
const char * CHAVE_ESCRITA_TS = "TR8NCXHI504TMA2E";
// Constante com a chave de leitura do canal da plataforma ThingSpeak
const char * CHAVE_LEITURA_TS = "ZTP77ISN9JZ5DO18";
// Constante com a chave de domínio ou endereço IP da plataforma ThingSpeak
const char * ACESSO_TS = "184.106.153.149";
// Segundos passados antes de declarar perda de terminal
const uint8_t SEGS_PERDA_TERMINAL = 90;
// Segundos passados antes de realizar nova consulta ou atualização à ThingSpeak
const uint8_t SEGS_INTERVALO_TS = 60;

// Constantes de tipos de comunicação (MIME Types)
const uint8_t TIPO_TEXTO_PLANO = 0;
const uint8_t TIPO_TEXTO_HTML = 1;

// Constantes de tipos de tags HTML para gerar
const uint8_t TAG_ABRIR = 0;
const uint8_t TAG_FECHAR = 1;
const uint8_t TAG_ABRIR_E_FECHAR = 2;

// Indicador de interação coma plataforma ThingSpeak
bool rotina_de_cliente_em_andamento = false;
// Ticker genérico
Ticker temporizador;
// Recipiente de cliente (para comunicação via internet com servidores)
WiFiClient cliente_wifi;
// Recipiente de servidor (para comunicação em rede local com clientes)
ESP8266WebServer servidor_wifi(PORTA_SERVIDOR);

// Indicador de disponibilidade do Uno para outra coleta de dados
bool pronto_para_ler = true;
// Variável que carrega o valor lido do sensor
uint16_t valor_sensor = 0;
// Contador genérico de segundos
uint8_t segundos = 0;
// Indicador de leitura recebida
bool leitura_recebida = false;

// Timers individuais
uint8_t timer_intervalo_thingspeak = 0;
uint8_t timer_perda_de_terminal = 0;

const char * LINK_PAGINA_INICIAL = "<a href=\"/\">Pagina Inicial</a><hr />";

// ========================================================================
// CLASSES
// ========================================================================

class VariavelGET
{
private:
    String rotulo, valor;
public:
    VariavelGET()
    {
        rotulo = "";
        valor = "";
    }
    VariavelGET(String rotulo, String valor)
    {
        this->rotulo = rotulo;
        this->valor = valor;
    }
    void mudar_rotulo(String rotulo)
    {
        this->rotulo = rotulo;
    }
    void mudar_valor(String valor)
    {
        this->valor = valor;
    }
    String pegar_rotulo()
    {
        return rotulo;
    }
    String pegar_valor()
    {
        return valor;
    }
    String para_string(bool adicional = false)
    {
        String resultado;
        if(adicional)
        {
            resultado = "&" + rotulo + "=" + valor;
        }
        else
        {
            resultado = rotulo + "=" + valor;
        }
        return resultado;
    }
};

// ========================================================================
// OBJETOS DEPENDENTES
// ========================================================================

VariavelGET chave_escrita("api_key", CHAVE_ESCRITA_TS);
VariavelGET chave_leitura("api_key", CHAVE_LEITURA_TS);

// ========================================================================
// FUNÇÕES DE COMUNICAÇÃO INFORMATIVA
// ========================================================================

// Inicia uma mensagem comum a ser mostrada na Serial
void iniciar_mensagem_comum()
{
    Serial.print(F("[ESP8266]: "));
}

// Informa uma mensagem comum
void informar_simples(String mensagem)
{
    iniciar_mensagem_comum();
    Serial.println(mensagem);
}

// ========================================================================
// FUNÇÕES DE COMUNICAÇÃO IMPERATIVA
// ========================================================================

// Requisita ao uno a leitura do sensor
void requisitar_leitura()
{
    if(pronto_para_ler == true)
    {
        pronto_para_ler = false;
        timer_perda_de_terminal = 0;
        Serial.println(">leitura");
    }
}

// ========================================================================
// FUNÇÕES DE PROTOCOLO HTTP
// ========================================================================

// Informa a string de tipo (MIME Type) de uma comunicação baseado em seu código informado
String tipo_comunicacao(uint8_t codigo)
{
    // Faça uma triagem para descobrir o tipo
    if(codigo == TIPO_TEXTO_PLANO)
    {
        return String(F("text/plain"));
    }
    if(codigo == TIPO_TEXTO_HTML)
    {
        return String(F("text/html"));
    }
    return String("");
}

// ========================================================================
// FUNÇÕES DE CÓDIGO HTML
// ========================================================================

// Cria uma tag de forma padronizada
String criar_tag(String nome, bool fechamento = false)
{
    String conteudo = "";
    // Forme a tag de forma diferente dependendo se ela for de abertura ou de fechamento
    if(fechamento == false)
    {
        conteudo += "<" + nome + ">";
    }
    else
    {
        conteudo += "</" + nome + ">";
    }
    return conteudo;
}

// Cria uma tag que se fecha sozinha
String criar_tag_unica(String nome)
{
    String conteudo = "<" + nome + " />";
    return conteudo;
}

// Envelopa um conteúdo entre tags
String envelopar_com_tags(String tag, String conteudo)
{
    conteudo = criar_tag(tag, false) + conteudo + criar_tag(tag, true);
    return conteudo;
}

// Gera o início de um HTML padrão
String inicio_html_padrao(String titulo_pagina)
{
    String conteudo = "";
    conteudo += "<html>";
    conteudo += envelopar_com_tags(String("head"), "<meta charset=\"utf-8\" />");
    conteudo += envelopar_com_tags(String("title"), titulo_pagina);
    conteudo += "<body>";
    conteudo += envelopar_com_tags(String("p"), "IP: " + WiFi.localIP().toString());
    return conteudo;
}

// Gera o fim de um HTML padrão
String fim_html_padrao()
{
    String conteudo = "</body></html>";
    return conteudo;
}

// ========================================================================
// FUNÇÕES DO SERVIDOR WEB
// ========================================================================

// Envia uma página HTML completa ao cliente
void enviar_pagina(String &conteudo)
{
    servidor_wifi.send(200, tipo_comunicacao(TIPO_TEXTO_HTML), conteudo);
}

// Cria uma página web para requisição de /
void pagina_root()
{
    // Conteúdo da página
    String conteudo = "";
    // Formando a página HTML
    conteudo += inicio_html_padrao("Sensor de Condutividade");
    conteudo += envelopar_com_tags(String("h1"), String("Sensor de Condutividade"));
    conteudo += criar_tag_unica(String("hr"));
    conteudo += envelopar_com_tags(String("p"), String("Bem-vindo à página do sensor de condutividade!"));
    conteudo += "<a href=\"/leitura\">Última leitura do sensor</a>";
    conteudo += fim_html_padrao();
    // Envie a página
    enviar_pagina(conteudo);
}

// Cria uma página web para leitura do último valor recebido do sensor
void pagina_leitura()
{
    // Conteúdo da página
    String conteudo = inicio_html_padrao(String("Última leitura do sensor"));
    conteudo += envelopar_com_tags(String("h1"), String("Última leitura do sensor"));
    conteudo += criar_tag_unica(String("hr"));
    conteudo += envelopar_com_tags(String("p"), envelopar_com_tags(String("b"), ("Ultima leitura do sensor: " + String(valor_sensor))));
    conteudo += LINK_PAGINA_INICIAL;
    // Disponibilize o link de leitura apenas se o Uno estiver pronto para outra coleta
    if(pronto_para_ler == true)
    {
        conteudo += "<a href=\"/atualizar\">Atualizar Leitura</a><hr />";
    }
    else
    {
        conteudo += "<p><b>O dispositivo ainda não está pronto para outra coleta...</b></p>";
    }
    conteudo += fim_html_padrao();
    // Envie a página ao cliente
    enviar_pagina(conteudo);
}

// Cria um página para atualizar a leitura do sensor
void pagina_atualizar()
{
    // Conteúdo da página
    String conteudo = inicio_html_padrao(String("Atualizar leitura do sensor"));
    // Se o Uno estiver disponível para leitura, realize-a
    if(pronto_para_ler == true)
    {
        // ENVIO DE COMANDO AO UNO
        conteudo += envelopar_com_tags(String("p"), String("Pedido para atualizar a leitura do sensor enviada com sucesso!"));
        requisitar_leitura();
    }
    // Se o Uno não estiver disponível para leitura, não a faça
    else
    {
        conteudo += envelopar_com_tags(String("p"), String("A unidade responsável pelo sensor não está disponível no momento!"));
    }
    conteudo += LINK_PAGINA_INICIAL;
    conteudo += fim_html_padrao();
    // Envie a página ao cliente
    enviar_pagina(conteudo);
}

// Cria uma página apenas com a última leitura do sensor
void pagina_ultima()
{
    String conteudo = String(valor_sensor);
    enviar_pagina(conteudo);
}

// Configura o servidor Web para responder requisições de diferentes formas
void configurar_servidor_web()
{
    // Crie as respostas das diferentes páginas
    servidor_wifi.on("/", pagina_root);
    servidor_wifi.on("/leitura", pagina_leitura);
    servidor_wifi.on("/atualizar", pagina_atualizar);
    servidor_wifi.on("/ultima", pagina_ultima);
    // Inicie o servidor após tudo ser configurado
    servidor_wifi.begin();
}

// Executa trabalhos relacionados à rotina do servidor de dados
void rotinas_de_servidor()
{
    // Trate conexões de clientes que queiram se comunicar com o servidor
    servidor_wifi.handleClient();
    // Atualize o serviço de mDNS
    MDNS.update();
}

// ========================================================================
// FUNÇÕES DE REDE E CONECTIVIDADE
// ========================================================================

// Tenta entrar em uma rede wifi com os dados informados
void conectar_na_rede(const char * nome, const char * senha)
{
    // Use o ESP8266 como host de uma rede externa
    WiFi.mode(WIFI_STA);
    // Conecte-se usando o objeto wifi
    WiFi.begin(nome, senha);
    // Continue tentando conectar enquanto não conseguir
    while(WiFi.status() != WL_CONNECTED)
    {
        // Espere um pouco
        delay(500);
        // Informe a tentativa de conexão
        Serial.print(F("[ESP8266]: Tentando conectar na rede "));
        Serial.println(nome);
    }
    // Ao fim, informe o sucesso da conexão
    Serial.print(F("[ESP8266]: Conectado com sucesso na rede "));
    Serial.print(nome);
    Serial.print(F(". O IP é: "));
    Serial.print(WiFi.localIP());
    Serial.print(F(" >> Acesse via Porta: "));
    Serial.println(PORTA_SERVIDOR);
}

// Configura um serviço de DNS mínimo e automaziado
void configurar_mdns()
{
    // Tente iniciar o serviço de DNS e verifique se teve êxito
    if(MDNS.begin("espsensor"))
    {
        // Informe o início do serviço
        delay(3000);
        iniciar_mensagem_comum();
        Serial.println(F("Servico mDNS iniciado!"));
    }
}

// Função que forma uma requisição GET para envio de dados ao canal
String formar_requisicao_get(String caminho, String variaveis)
{
    String req_get;
    req_get = "/" + caminho;
    if(variaveis.length() > 0)
    {
        req_get += "?" + variaveis;
    }
    return req_get;
}

// Envia dados à plataforma da ThingSpeak
void atualizar_thingspeak()
{
    // Verifique se a leitura pode ser feita (sem impedir o processamento do timer)
    if(timer_intervalo_thingspeak > SEGS_INTERVALO_TS)
    {
        // Zere o timer de consulta à ThingSpeak devido à atualização de valores
        timer_intervalo_thingspeak = 0;
        // Anule o indicador de leitura recebida
        leitura_recebida = false;
        
        // Cliente HTTP
        HTTPClient cli;
        // Recipiente com o valor lido pelo sensor
        VariavelGET val_sensor("field1", String(valor_sensor));
        // Recipiente com o valor do indicador de leitura
        VariavelGET indicador_leitura("field2", "0");
        // Código de retorno para requisições
        int codigo_http = 0;
        // Resposta de uma requisição HTTP
        String resposta;
        // Indique o envio de dados
        iniciar_mensagem_comum();
        Serial.println(F("Atualizando | Enviando Dados"));
        // String com dados a enviar
        String dados = String("http://") + String(ACESSO_TS) + formar_requisicao_get("update", chave_escrita.para_string() + val_sensor.para_string(true) + indicador_leitura.para_string(true));
        // Mostre a string a ser enviada
        informar_simples(dados);
        
        // Inicie a comunicação com a plataforma ThingSpeak
        cli.begin(dados);
        // Realize a requisição GET
        codigo_http = cli.GET();
        // Verifique o código retornado
        if(codigo_http > 0)
        {
            // Consiga a resposta e imprima
            resposta = cli.getString();
            iniciar_mensagem_comum();
            Serial.print(F("Resp Server: "));
            Serial.println(resposta);
        }
        // Encerre a conexão
        cli.end();
    }
}

// Envia dados à plataforma da ThingSpeak
bool consultar_thingspeak()
{
    // Recipiente com a resposta da ThingSpeak
    bool resultado = false;
    // Verifique se a ThingSpeak pode ser acessada
    if(timer_intervalo_thingspeak > SEGS_INTERVALO_TS)
    {
        // Zere o contador de segundos
        timer_intervalo_thingspeak = 0;
        
        // Cliente HTTP
        HTTPClient cli;
        // Código de retorno para requisições
        int codigo_http = 0;
        // Resposta de uma requisição HTTP
        String resposta;
        // Indique o envio de dados
        iniciar_mensagem_comum();
        Serial.println(F("Consultando | Enviando dados"));
        // String com dados a enviar
        String dados = String("http://") + String(ACESSO_TS) + formar_requisicao_get(String("channels/") + ID_CANAL_TS + "/fields/2/last.txt", chave_leitura.para_string());
        // Mostre a string a ser enviada
        informar_simples(dados);
        
        // Inicie a comunicação com a plataforma ThingSpeak
        cli.begin(dados);
        // Realize a requisição GET
        codigo_http = cli.GET();
        // Verifique o código retornado
        if(codigo_http > 0)
        {
            // Consiga a resposta e imprima
            resposta = cli.getString();
            // Avalie se uma leitura é necessária
            if(resposta.toInt() > 0)
                resultado = true;
            else
                resultado = false;
            iniciar_mensagem_comum();
            Serial.print(F("Resp Server: "));
            Serial.println(resposta);
        }
        // Encerre a conexão
        cli.end();
    }
    // Devolva o resultado da consulta
    return resultado;
}

// Função que trata os dados na plataforma ThingSpeak
void rotinas_de_cliente()
{
    // Indicador de pedido de leitura
    bool pedido = false;
    // Verifique se pode consultar a ThingSpeak (se o dispositivo de leitura estiver disponível)
    if(pronto_para_ler == true)
    {
        pedido = consultar_thingspeak();
    }
    // Verifique se alguma leitura foi requisitada
    if(pedido == true)
    {
        // Faça o pedido de leitura
        informar_simples("Leitura requisitada");
        requisitar_leitura();
    }
}

// Consegue a leitura do sensor enviada pelo Arduino Uno, se ele tiver lido algo
void conseguir_leitura()
{
    // Recipiente para comunicação recebida do Uno
    String uno_disse = "";
    // Verifique se a leitura está sendo esperada
    if(pronto_para_ler == false)
    {
        // Cheque se há algo a ser lido
        if(Serial.available())
        {
            uno_disse = Serial.readStringUntil('\n');
        }
        // Se o comunicado existir, cheque-o
        if(uno_disse.length() > 0)
        {
            // Verifique se é um comando
            if(uno_disse.charAt(0) == '>')
            {
                // Trate o comando
                if(uno_disse.indexOf("sensor") == 1)
                {
                    if(uno_disse.length() >= 9)
                    {
                        valor_sensor = (uno_disse.substring(8)).toInt();
                        pronto_para_ler = true;
                        informar_simples("Valor do sensor: " + String(valor_sensor));
                        leitura_recebida = true;
                    }
                }
            }
        }
    }
}

// Dá o sensor ou o Uno como não conectados ao ESP
void perda_de_terminal()
{
    if(timer_perda_de_terminal > SEGS_PERDA_TERMINAL && pronto_para_ler == false)
    {
        valor_sensor = 2000;
        pronto_para_ler = true;
        informar_simples("Perda de terminal");
        atualizar_thingspeak();
    }
}

// ========================================================================
// FUNÇÕES DE TEMPO
// ========================================================================

// Registra os segundos passados
void contabilizar_tempo()
{
    segundos++;
}

// Atualiza timers individuais
void atualizar_timers()
{
    timer_intervalo_thingspeak += segundos;
    timer_perda_de_terminal += segundos;
    /*
    if(segundos > 0)
    {
        informar_simples("TIMERS");
        informar_simples("--Intervalo TS: " + String(timer_intervalo_thingspeak));
        informar_simples("--Perda de Terminal: " + String(timer_perda_de_terminal));
    }
    */
    segundos = 0;
}

// ========================================================================
// PROGRAMA PRINCIPAL
// ========================================================================

// Função de configuração do ESP8266
void setup()
{
    // Conecte-se ao Arduino via Serial
    Serial.begin(VELOCIDADE_SERIAL);
    // Configura um serviço de DNS mínimo e automaziado
    configurar_mdns();
    // Configure o servidor Web
    configurar_servidor_web();
    // Espere um pouco
    delay(1000);
    // Conecte-se a uma rede wifi desejada
    conectar_na_rede(REDE_SSID, REDE_SENHA);
    // Associe a função de atualização com o ticker
    temporizador.attach(1, contabilizar_tempo);
}

// Função de execução de rotinas do ESP8266
void loop()
{
    // Atualiza os timers
    atualizar_timers();
    // Realize os trabalhos relacionados ao servidor de dados
    rotinas_de_servidor();
    // Conseguir dados do sensor
    conseguir_leitura();
    // Verifique se a leitura foi recebida
    if(leitura_recebida == true)
    {
        // Tente atualizar a plataforma
        atualizar_thingspeak();
    }
    // Se não foi recebida
    else
    {
        // Realize os trabalhos relacionados à consulta da ThingSpeak
        rotinas_de_cliente();
    }
    // Verifique se deve ser assumido que o Uno não está disponível
    perda_de_terminal();
}
