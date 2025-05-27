#include "wifi_conn.h"       // Cabeçalho com a declaração da função de conexão Wi-Fi
#include "pico/cyw43_arch.h" // Biblioteca para controle do chip Wi-Fi CYW43 no Raspberry Pi Pico W
#include <stdio.h>
#include "lwip/apps/mqtt.h" // Biblioteca MQTT do lwIP
#include "mqtt_comm.h"      // Header file com as declarações locais
#include "lwipopts.h"       // Configurações customizadas do lwIP
#include <time.h>
#include "pico/stdlib.h"
#include <stdint.h>
#include "include/ssd1306.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

const int A = 5;
const int B = 6;

uint32_t ultima_timestamp_recebida = 0; // Usando uint32_t para maior clareza

ssd1306_t display;

bool wifi_connection = false;

/* void wrap_text(const char *input, char output[][17], int *num_lines)
{
    int input_len = strlen(input);
    int line_start = 0;
    int line_end = 0;
    *num_lines = 0;

    while (line_start < input_len && *num_lines < 5)
    {
        // Encontra o último espaço dentro do limite da linha (16 char)
        line_end = line_start + 16;
        if (line_end > input_len)
        {
            line_end = input_len;
        }
        else
        {
            while (line_end > line_start && input[line_end] != ' ')
            {
                line_end--;
            }
            if (line_end == line_start)
            {
                line_end = line_start + 16; // Quebra a linha à força caso nenhum espaço seja encontrado
            }
        }

        // Copia a linha pra saída
        strncpy(output[*num_lines], &input[line_start], line_end - line_start);
        output[*num_lines][line_end - line_start] = '\0'; // Término nulo para a linha

        (*num_lines)++;
        line_start = line_end + 1; // Segue para a próxima linha
    }
} */

/* void render_wrapped_text(uint8_t *ssd, const char *text, struct render_area *frame_area, int index_scroll)
{
    char lines[5][17]; // Buffer para guardar as linhas formatadas (até 5 linhas, 16 chars cada)
    int num_lines = 0;

    // Formata o texto nas linhas
    wrap_text(text, lines, &num_lines);

    // Limpa o display
    memset(ssd, 0, ssd1306_buffer_length);

    // Renderiza a primeira linha, vazia
    ssd1306_draw_string(ssd, 0, 0, "");

    // Renderiza o texto da tarefa
    int y = 8; // inicia na segunda linha
    for (int i = 0; i < num_lines && i < 3; i++)
    {
        ssd1306_draw_string(ssd, 0, y, lines[i]);
        y += 8; // Move para a próxima linha
    }

    // Atualiza o display OLED
    render_on_display(ssd, frame_area);
} */

void wifi_init()
{
    // Inicializa o driver Wi-Fi (CYW43). Retorna 0 se for bem-sucedido.
    if (cyw43_arch_init())
    {
        printf("Erro ao iniciar Wi-Fi\n");
        wifi_connection = false;
        return;
    }
}

/**
 * Função: connect_to_wifi
 * Objetivo: Inicializar o chip Wi-Fi da Pico W e conectar a uma rede usando SSID e senha fornecidos.
 */
void connect_to_wifi(const char *ssid, const char *password)
{
    // Habilita o modo estação (STA) para se conectar a um ponto de acesso.
    cyw43_arch_enable_sta_mode();

    // Tenta conectar à rede Wi-Fi com um tempo limite de 30 segundos (30000 ms).
    // Utiliza autenticação WPA2 com criptografia AES.
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Erro ao conectar\n"); // Se falhar, imprime mensagem de erro.
        wifi_connection = false;
    }
    else
    {
        printf("Conectado ao Wi-Fi\n"); // Se conectar com sucesso, exibe confirmação.
        wifi_connection = true;
    }
}

/* Variável global estática para armazenar a instância do cliente MQTT
 * 'static' limita o escopo deste arquivo */
static mqtt_client_t *client;

/* Callback de conexão MQTT - chamado quando o status da conexão muda
 * Parâmetros:
 *   - client: instância do cliente MQTT
 *   - arg: argumento opcional (não usado aqui)
 *   - status: resultado da tentativa de conexão */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        printf("Conectado ao broker MQTT com sucesso!\n");
    }
    else
    {
        printf("Falha ao conectar ao broker, código: %d\n", status);
    }
}

/* Função para configurar e iniciar a conexão MQTT
 * Parâmetros:
 *   - client_id: identificador único para este cliente
 *   - broker_ip: endereço IP do broker como string (ex: "192.168.1.1")
 *   - user: nome de usuário para autenticação (pode ser NULL)
 *   - pass: senha para autenticação (pode ser NULL) */
void mqtt_setup(const char *client_id, const char *broker_ip, const char *user, const char *pass)
{
    ip_addr_t broker_addr; // Estrutura para armazenar o IP do broker

    // Converte o IP de string para formato numérico
    if (!ip4addr_aton(broker_ip, &broker_addr))
    {
        printf("Erro no IP\n");
        return;
    }

    // Cria uma nova instância do cliente MQTT
    client = mqtt_client_new();
    if (client == NULL)
    {
        printf("Falha ao criar o cliente MQTT\n");
        return;
    }

    // Configura as informações de conexão do cliente
    struct mqtt_connect_client_info_t ci = {
        .client_id = client_id, // ID do cliente
        .client_user = user,    // Usuário (opcional)
        .client_pass = pass     // Senha (opcional)
    };

    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, NULL);

    // Inicia a conexão com o broker
    // Parâmetros:
    //   - client: instância do cliente
    //   - &broker_addr: endereço do broker
    //   - 1883: porta padrão MQTT
    //   - mqtt_connection_cb: callback de status
    //   - NULL: argumento opcional para o callback
    //   - &ci: informações de conexão
    mqtt_client_connect(client, &broker_addr, 1883, mqtt_connection_cb, NULL, &ci);
}

/* Callback de confirmação de publicação
 * Chamado quando o broker confirma recebimento da mensagem (para QoS > 0)
 * Parâmetros:
 *   - arg: argumento opcional
 *   - result: código de resultado da operação */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
    if (result == ERR_OK)
    {
        printf("Publicação MQTT enviada com sucesso!\n");
    }
    else
    {
        printf("Erro ao publicar via MQTT: %d\n", result);
    }
}

/* Função para publicar dados em um tópico MQTT
 * Parâmetros:
 *   - topic: nome do tópico (ex: "sensor/temperatura")
 *   - data: payload da mensagem (bytes)
 *   - len: tamanho do payload */
void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len)
{
    // Envia a mensagem MQTT
    err_t status = mqtt_publish(
        client,              // Instância do cliente
        topic,               // Tópico de publicação
        data,                // Dados a serem enviados
        len,                 // Tamanho dos dados
        0,                   // QoS 0 (nenhuma confirmação)
        0,                   // Não reter mensagem
        mqtt_pub_request_cb, // Callback de confirmação
        NULL                 // Argumento para o callback
    );

    if (status != ERR_OK)
    {
        printf("mqtt_publish falhou ao ser enviada: %d\n", status);
    }
}

// Função de callback para lidar com mensagens chegando
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    printf("Publish chegou para o topico: '%s' (tamanho %u)\n", topic, (unsigned int)tot_len);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    char message[len + 1];

    memcpy(message, data, len);
    message[len] = '\0';

    printf("Recebeu mensagem MQTT: %s\n", message);

    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 32, 1, message);
    ssd1306_show(&display);
}

void mqtt_comm_subscribe(const char *topic)
{
    // Faz o set do callback
    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, NULL);

    err_t status = mqtt_subscribe(client, topic, 0, NULL, NULL);

    if (status != ERR_OK)
    {
        printf("mqtt_subscribe falhou: %d\n", status);
    }
    else
    {
        printf("Inscrito no topico: %s\n", topic);
    }
}

/**
 * Função para aplicar cifra XOR (criptografia/decifração)
 *
 * @param input  Ponteiro para os dados de entrada (texto claro ou cifrado)
 * @param output Ponteiro para armazenar o resultado (deve ter tamanho >= len)
 * @param len    Tamanho dos dados em bytes
 * @param key    Chave de 1 byte (0-255) para operação XOR
 *
 * Funcionamento:
 * - Aplica operação XOR bit-a-bit entre cada byte do input e a chave
 * - XOR é reversível: mesma função para cifrar e decifrar
 * - Criptografia fraca (apenas para fins didáticos ou ofuscação básica)
 */
void xor_encrypt(const uint8_t *input, uint8_t *output, size_t len, uint8_t key)
{
    // Loop por todos os bytes dos dados de entrada
    for (size_t i = 0; i < len; ++i)
    {
        // Operação XOR entre o byte atual e a chave
        // Armazena resultado no buffer de saída
        output[i] = input[i] ^ key;
    }
}

void printout_stuff()
{
    // Mensagem original a ser enviada
    const char *mensagem = "26.5";
    // Buffer para mensagem criptografada (16 bytes)
    uint8_t criptografada[16];

    // Criptografa a mensagem usando XOR com chave 42
    xor_encrypt((uint8_t *)mensagem, criptografada, strlen(mensagem), 42);

    // Publica a mensagem criptografada
    mqtt_comm_publish("escola/sala1/temperatura", criptografada, strlen(mensagem));

    char buffer[256];
    sprintf(buffer, "{\"valor\":26.5,\"ts\":%lu}", time(NULL));
    mqtt_comm_publish("escola/sala1/temperatura", buffer, strlen(buffer));

    mqtt_comm_publish("escola/sala1/temperatura", mensagem, strlen(mensagem));
}

void driver_i2c_init() 
{
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void display_init(ssd1306_t *disp) 
{
    driver_i2c_init();

    disp->external_vcc=false;
    ssd1306_init(disp, 128, 64, 0x3C, I2C_PORT);
    ssd1306_clear(disp);
}

int main()
{
    stdio_init_all();

    char ssid[] = "";
    char password[] = "";

    wifi_init();
    connect_to_wifi(ssid, password);
    char broker_ip[] = "";

    mqtt_setup("bitdog1", broker_ip, "aluno", "senha123");

    display_init(&display);

    gpio_init(A);
    gpio_set_dir(GPIO_IN, A);
    gpio_pull_up(A);

    gpio_init(B);
    gpio_set_dir(GPIO_IN, B);
    gpio_pull_up(B);

    sleep_ms(3000);

    while (true)
    {
        if(!gpio_get(A))
        {
            if(!wifi_connection) connect_to_wifi(ssid, password);
            mqtt_unsubscribe(client, "escola/sala1/temperatura", NULL, 0);
            printout_stuff();
        }

        if(!gpio_get(B))
        {
            if(!wifi_connection) connect_to_wifi(ssid, password);
            mqtt_comm_subscribe("escola/sala1/temperatura");
        }

        sleep_ms(300);
    }
}
