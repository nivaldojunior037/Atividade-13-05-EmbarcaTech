#include <stdio.h>               // Biblioteca padrão para entrada e saída
#include <string.h>              // Biblioteca manipular strings
#include <stdlib.h>              // funções para realizar várias operações, incluindo alocação de memória dinâmica (malloc)
#include <math.h>

#include "pico/stdlib.h"         // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "hardware/adc.h"        // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC
#include "pico/cyw43_arch.h"     // Biblioteca para arquitetura Wi-Fi da Pico com CYW43  

#include "lwip/pbuf.h"           // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"            // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"          // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

// Credenciais WIFI - Tome cuidado se publicar no github!
#define WIFI_SSID "SUA_REDE"
#define WIFI_PASSWORD "SUA_SENHA"

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43
#define LED_GREEN_PIN 11                // GPIO11 - LED verde
#define LED_RED_PIN 13                  // GPIO13 - LED vermelho
#define BUTTON_A 5                      //Definição da porta GPIO do botão A
#define BUTTON_B 6                      //Definição da porta GPIO do botão B
#define BUZZER 21                       //Definição da porta GPIO do Buzzer A
#define VRX_PIN 27                      //Definição da porta GPIO do potenciômetro do eixo X do Joystick
#define VRY_PIN 26                      //Definição da porta GPIO do potenciômetro do eixo Y do Joystick

// Definição de todos os contadores, flags, variáveis e estruturas que serão utilizadas de forma global 
static volatile uint32_t ultimo_tempo = 0;
static volatile float freq = 0;
static volatile int contador = 0;
static volatile int cont1 = 0;
static volatile int cont2 = 0;
static volatile int spo2 = 0;
static volatile int bpm = 0;
static volatile bool alerta1 = false;
static volatile bool alerta2 = false;
static volatile float intervalos[5];
static volatile char *cor_fundo = "#68CEE2";
static volatile char *mensagem = " ";

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void setup(void);

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Leitura da temperatura interna
float temp_read(void);

// Tratamento do request do usuário
void user_request(char **request);

void tocar_buzzer(int frequencia, int duracao);

// Função que administrará a interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

// Função principal
int main()
{
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializar os pinos GPIO da BitDogLab utilizados
    setup();

    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");
    
    // Interrupção com callback para cada um dos botões 
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true)
    {
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        
        // Leitura do ADC para a entrada 1 (GPIO 27)
        adc_select_input(1);
        uint16_t vrx_value = adc_read();

        // O valor lido será passado para a variável luz_verm;
        float luz_verm;
        // O valor lido é convertido de forma que varie de 2048 na região central a cerca de 0 nas extremidades máxima e mínima
        if(vrx_value<2000){
            luz_verm = vrx_value; 
        } else if (vrx_value> 2100.0){
            luz_verm = 4096.0-vrx_value;
        } else {
            luz_verm = 2048;
        }

        // Leitura do ADC para a entrada 0 (GPIO 26)
        adc_select_input(0);
        uint16_t vry_value = adc_read();

        // O valor lido será passado patra a variável luz_infrav;
        float luz_infrav;
        // O valor lido é convertido de forma que varie de 2048 na região central a cerca de 0 nas extremidades máxima e mínima
        if(vry_value < 2000){
            luz_infrav = vry_value;
        } else if (vry_value > 2100.0){
            luz_infrav = 4096.0-vry_value;
        } else {
            luz_infrav = 2048;
        }
        
        // A saturação de oxigênio é calculada de modo que na região central se aproxime de 100% e diminua em direção às extremidades
        float saturacao = 0.8+(luz_infrav+luz_verm)/20480.0; 

        // A saturação é limitada entre 100% e 80%, para melhorar a visualização
        if(saturacao>1.0){
            saturacao = 1.0;
        } else if (saturacao<0.8){
            saturacao = 0.8;
        }

        // Se a saturação estiver entre 85% e 90%, a flag alerta1 é ativada e alerta2 é desativada
        if(saturacao < 0.9 && saturacao > 0.85){
            alerta1 = true;
            alerta2 = false;
        } 
        // Se a saturação estiver menor que 85%, a flag alerta2 é ativada e alerta1 é desativada
        else if (saturacao < 0.85){
            alerta2 = true;
            alerta1 = false;
        }

        // O valor calculado para freq na interrupção é convertido em um inteiro e armazenado em bpm
        bpm = round(freq);

        // O valor calculado para saturacao é convertido também em um inteiro e armazenado em spo2
        spo2 = round(saturacao*100.0);
        
        // A depender das variáveis bpm, cont1 e cont2 e das flags alerta1 e alerta2, diferentes mensagens são impressas
        // Além das mensagens, os estados dos LEDs verde e vermelho do conjunto RGB e do buzzer A também são alterados
        if((bpm > 100 || bpm < 60) && cont1 >= 5){
            gpio_put(LED_GREEN_PIN, 1);
            gpio_put(LED_RED_PIN, 1);
            cor_fundo = "#DAE268";
            mensagem = "Freq. cardiaca alterada!";
            tocar_buzzer(500, 250);
            sleep_ms(500);
        } else if(alerta1){
            gpio_put(LED_GREEN_PIN, 1);
            gpio_put(LED_RED_PIN, 1);
            cor_fundo = "#DAE268";
            mensagem = "Alerta: SpO2 baixa";
            tocar_buzzer(500, 250);
            alerta1 = false;
            sleep_ms(500);
        } else if((bpm > 120 || bpm < 40) && cont2 >= 5){
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_RED_PIN, 1);
            cor_fundo = "#E26868";
            mensagem = "Freq. cardiaca critica!!";
            tocar_buzzer(500,1000);
            sleep_ms(500);
        } else if(alerta2){
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_RED_PIN, 1);
            cor_fundo = "#E26868";
            mensagem = "Alerta: SpO2 critica!";
            tocar_buzzer(500,1000);
            alerta2 = false;
            sleep_ms(500);
        }
        // Se nenhum dos casos de alerta for disparado, o display exibe apenas os valores atuais de batimentos e de oxigenação com luz verde
        else {
            gpio_put(LED_GREEN_PIN, 1);
            gpio_put(LED_RED_PIN, 0);
            cor_fundo = "#68CEE2";
            mensagem = "";
        }
        // Pequeno delay entre duas iterações consecutivas
        sleep_ms(10);
        sleep_ms(100);      // Reduz o uso da CPU
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Funções ---------------------------------

// Função que inicializará todas as entradas, saídas e interfaces necessárias à execução do código
void setup(){
    // Inicialização dos LEDs verde e vermelho do conjunto RGB e do buzzer A e a definição de todos como saída
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, 0);
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, 0);
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, 0);

    // Inicialização do botão A e do botão B, definição como entrada e acionamento do pull-up interno
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);


    // Inicialização do ADC e configuração para os GPIOs 26 e 27 e para o sensor de temperatura
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);
    adc_set_temp_sensor_enabled(true);

    // O vetor intervalos é percorrido e todos os valores são zerados inicialmente
    for(int j=0; j<5; j++){
        intervalos[j]=0.0;
    }

}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário (mantido para uso na versão final)
void user_request(char **request){

    if (strstr(*request, "GET /green_on") != NULL)
    {
        gpio_put(LED_GREEN_PIN, 1);
    }
    else if (strstr(*request, "GET /green_off") != NULL)
    {
        gpio_put(LED_GREEN_PIN, 0);
    }
    else if (strstr(*request, "GET /red_on") != NULL)
    {
        gpio_put(LED_RED_PIN, 1);
    }
    else if (strstr(*request, "GET /red_off") != NULL)
    {
        gpio_put(LED_RED_PIN, 0);
    }
    else if (strstr(*request, "GET /on") != NULL)
    {
        cyw43_arch_gpio_put(LED_PIN, 1);
    }
    else if (strstr(*request, "GET /off") != NULL)
    {
        cyw43_arch_gpio_put(LED_PIN, 0);
    }
};

// Leitura da temperatura interna
float temp_read(void){
    adc_select_input(4);
    uint16_t raw_value = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float temperature = 29.3f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
        return temperature;
}

// Função que acionará o buzzer com uma dada frequência e duração
void tocar_buzzer(int frequencia, int duracao)
{
    for (int i = 0; i < duracao * 1000; i += (1000000 / frequencia) / 2)
    {
        gpio_put(BUZZER, 1);
        sleep_us((1000000 / frequencia) / 2);
        gpio_put(BUZZER, 0);
        sleep_us((1000000 / frequencia) / 2);
    }
}

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);
    
    // Leitura da temperatura interna
    float temperature = temp_read();

    // Cria a resposta HTML
    char html[1024];

    // Instruções html do webserver
    snprintf(html, sizeof(html), // Formatar uma string e armazená-la em um buffer de caracteres
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<meta http-equiv=\"refresh\" content=\"2\">\n"
             "<title> Embarcatech - Monitoramento de Sinais Vitais </title>\n"
             "<style>\n"
             "body { background-color: %s; font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
             "h1 { font-size: 64px; margin-bottom: 30px; }\n"
             "button { background-color: LightGray; font-size: 36px; margin: 10px; padding: 20px 40px; border-radius: 10px; }\n"
             ".sinaisvitais { font-size: 48px; margin-top: 30px; color: #333; }\n"
             "</style>\n"
             "</head>\n"
             "<body>\n"
             "<h1>EmbarcaFit</h1>\n"
             "<br><br>\n"
             "<h1>%s</h1>\n"
             "<p class=\"sinaisvitais\">Temperatura Corporal: %.2f &deg;C</p>\n"
             "<p class=\"sinaisvitais\">Saturacao: %d%%</p>\n"
             "<p class=\"sinaisvitais\">Frequencia Cardiaca: %d BPM</p>\n"
             "</body>\n"
             "</html>\n",
             cor_fundo, mensagem, temperature, spo2, bpm);

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    //libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}

// Função de interrupção com debouncing
static void gpio_irq_handler(uint gpio, uint32_t events){
    // Obtenção do tempo em que ocorre a interrupção desde a inicialização
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    // Verificação de alteração em um intervalo maior que 200ms (debouncing)
    if(tempo_atual - ultimo_tempo > 200000){
        contador++;
        // Um novo intervalo entre dois batimentos é gerado e armazenado em interv
        float interv = tempo_atual-ultimo_tempo;
        // Os valores do vetor intervalos são atualizados de modo que o valor da posição 4 passe para a posição 5 e assim por diante
        // Isso permite que o último intervalo detectado seja inserido no vetor e o mais antigo seja apagado
        for(int i=4; i>0; i--){
            intervalos[i] = intervalos[i-1];
        } 
        // O valor de interv é inserido no vetor na posição 0
        intervalos[0] = interv;
        // O valor de ultimo_tempo é atualizado
        ultimo_tempo = tempo_atual; 

        // A soma dos intervalos entre batimentos é realizada e armazenada em intervalo_medio
        float intervalo_medio = 0.0;
        for(int k=0; k<5; k++){
            intervalo_medio = intervalo_medio + intervalos[k];
        }
        // Se contador estiver em 5, isso significa que o vetor já está cheio e uma frequencia média pode ser calculada
        if(contador>=5){
            // A frequência será dada pela divisão de 60 segundos pela média dos intervalos armazenados no vetor anterior
            freq = 60000000.0/(intervalo_medio/5.0);
        }
        
        // Se a frequência estiver acima de 100 ou abaixo de 60, cont1 é incrementado e cont2 é zerado
        if((freq>100.0 && freq<120.0)||(freq<60.0 && freq>40.0)){
            cont1++;
            cont2 = 0;
        } 
        // Se a frequência estiver abaixo de 40 ou acima de 120, cont2 é incrementado e cont1 é zerado
        else if(freq>120.0 || freq<40.0){
            cont2++;
            cont1 = 0;
        } 
        // Se a frequência estiver entre 60 e 100, ambos os contadores são zerados
        else {
            cont1 = 0;
            cont2 = 0;
        }
    }
}
