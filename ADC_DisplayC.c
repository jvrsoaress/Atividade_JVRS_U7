#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

// definições para I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

// definições dos pinos
#define SENSOR_PIN 26  // eixo Y (vertical)
#define VRX_PIN 27     // eixo X (usado em Dados)
#define VRY_PIN 26     // eixo Y (reutilizando GPIO 26)
#define SW_PIN 22      // botão do joystick
#define LED_PIN_RED 13
#define LED_PIN_BLUE 12
#define LED_PIN_GREEN 11
#define BUTTON_A 5      // modo teste e pausa
#define BUTTON_B 6      // modo dados
#define BUZZER_PIN 10   // buzzer no GPIO 10

// valor central do joystick
#define POS_CENTRAL 2048

// estados do menu
enum { MENU_INICIAL, MODO_TESTE, DADOS_TENSAO, DADOS_FALHAS } estado = MENU_INICIAL;

// variáveis globais
bool monitoramento_ativo = true;
bool modo_exibicao = false; // modo 1: barra + texto, modo 2: só mensagem
bool barra_visivel = false; // controla a barra na tela 1 do Dados
uint32_t last_time = 0;
uint32_t last_print_time = 0;
uint32_t falhas = 0;        // contador de falhas
uint32_t button_a_press_time = 0; // tempo de pressão do botão A
uint32_t button_b_press_time = 0; // tempo de pressão do botão B
bool button_a_pressed = false;
bool button_b_pressed = false;

// inicializa PWM
uint pwm_init_gpio(uint gpio, uint wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_enabled(slice_num, true);
    return slice_num;
}

// interrupção
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time > 200) { // 200ms debouncing
        last_time = current_time;
        if (estado == MENU_INICIAL) {
            if (gpio == BUTTON_A) {
                estado = MODO_TESTE;
                printf("Botão A pressionado - Modo Teste\n");
            } else if (gpio == BUTTON_B) {
                estado = DADOS_TENSAO;
                printf("Botão B pressionado - Modo Dados (Tensão)\n");
            }
        } else if (estado == MODO_TESTE && gpio == BUTTON_A) {
            monitoramento_ativo = !monitoramento_ativo;
            printf("Botão A pressionado - %s\n", monitoramento_ativo ? "Ativo" : "Pausado");
        } else if (estado == MODO_TESTE && gpio == SW_PIN) {
            modo_exibicao = !modo_exibicao;
            printf("Botão Joystick pressionado - Modo Exibição %d\n", modo_exibicao);
        } else if (estado == DADOS_TENSAO && gpio == BUTTON_B) {
            estado = DADOS_FALHAS;
            barra_visivel = false; // reseta a barra ao mudar de tela
            printf("Botão B pressionado - Modo Dados (Falhas)\n");
        } else if (estado == DADOS_TENSAO && gpio == SW_PIN) {
            barra_visivel = !barra_visivel; // alterna a visibilidade da barra
            printf("Botão Joystick pressionado - Barra %s\n", barra_visivel ? "Visível" : "Invisível");
        } else if (estado == DADOS_FALHAS && gpio == BUTTON_B) {
            estado = DADOS_TENSAO;
            printf("Botão B pressionado - Volta pra Modo Dados (Tensão)\n");
        }
    }
}

int main() {
    stdio_init_all();

    // configuração do ADC
    adc_init();
    adc_gpio_init(SENSOR_PIN);
    adc_gpio_init(VRX_PIN);

    // configuração dos botões
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // configuração dos LEDs e buzzer
    uint pwm_wrap = 4096;
    uint pwm_slice_red = pwm_init_gpio(LED_PIN_RED, pwm_wrap);
    uint pwm_slice_blue = pwm_init_gpio(LED_PIN_BLUE, pwm_wrap);
    uint pwm_slice_green = pwm_init_gpio(LED_PIN_GREEN, pwm_wrap);
    uint pwm_slice_buzzer = pwm_init_gpio(BUZZER_PIN, 31250); // 4000Hz

    // configuração I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // inicialização do display OLED
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // configuração das interrupções
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(SW_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);

    bool falha_registrada_teste = false;
    bool falha_registrada_dados = false;

    while (true) {
        // leitura do joystick (eixo Y e X)
        adc_select_input(0); // GPIO 26 (VRY)
        uint16_t vry_value = adc_read();
        adc_select_input(1); // GPIO 27 (VRX)
        uint16_t vrx_value = adc_read();

        // verificar pressão longa dos botões A e B
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (!gpio_get(BUTTON_A)) { // botão A pressionado (baixo)
            if (!button_a_pressed) {
                button_a_press_time = current_time;
                button_a_pressed = true;
            } else if (current_time - button_a_press_time >= 3000 && estado != MENU_INICIAL) {
                estado = MENU_INICIAL;
                monitoramento_ativo = true;
                modo_exibicao = false;
                printf("Botão A pressionado por 3s - Volta ao Menu\n");
            }
        } else {
            button_a_pressed = false;
        }
        if (!gpio_get(BUTTON_B)) { // botão B pressionado (baixo)
            if (!button_b_pressed) {
                button_b_press_time = current_time;
                button_b_pressed = true;
            } else if (current_time - button_b_press_time >= 3000 && estado != MENU_INICIAL) {
                estado = MENU_INICIAL;
                printf("Botão B pressionado por 3s - Volta ao Menu\n");
            }
        } else {
            button_b_pressed = false;
        }

        // lógica por estado
        ssd1306_fill(&ssd, false);

        if (estado == MENU_INICIAL) {
            ssd1306_draw_string(&ssd, " APERTE O BOTAO", 0, 20);
            ssd1306_draw_string(&ssd, "   A  TESTE", 0, 40);
            ssd1306_draw_string(&ssd, "   B  DADOS", 0, 50);
            pwm_set_gpio_level(LED_PIN_RED, 0);
            pwm_set_gpio_level(LED_PIN_GREEN, 0);
            pwm_set_gpio_level(LED_PIN_BLUE, 0);
            pwm_set_gpio_level(BUZZER_PIN, 0);
        } 
        else if (estado == MODO_TESTE) {
            // calcula a distância do centro e mapeia pra 0-100
            int distancia_do_centro = (vry_value > POS_CENTRAL) ? (vry_value - POS_CENTRAL) : (POS_CENTRAL - vry_value);
            int porcentagem = (distancia_do_centro * 100) / 2048;
            int bar_width = (distancia_do_centro * 128) / 2048;

            // lógica de direção vertical
            bool para_cima = (vry_value < POS_CENTRAL);
            bool para_baixo = (vry_value > POS_CENTRAL);

            // controle dos LEDs e buzzer
            if (monitoramento_ativo) {
                if (para_cima) {
                    pwm_set_gpio_level(LED_PIN_RED, 4096);
                    pwm_set_gpio_level(LED_PIN_GREEN, 0);
                } else if (para_baixo) {
                    pwm_set_gpio_level(LED_PIN_RED, 0);
                    pwm_set_gpio_level(LED_PIN_GREEN, 4096);
                } else {
                    pwm_set_gpio_level(LED_PIN_RED, 0);
                    pwm_set_gpio_level(LED_PIN_GREEN, 0);
                }
                pwm_set_gpio_level(LED_PIN_BLUE, 0);

                // buzzer e contador de falhas
                if (para_cima && porcentagem > 80) {
                    pwm_set_wrap(pwm_slice_buzzer, 31250); // 4000Hz
                    pwm_set_gpio_level(BUZZER_PIN, 15625); // 50% duty cycle
                    if (!falha_registrada_teste) {
                        falhas++;
                        falha_registrada_teste = true;
                    }
                } else {
                    pwm_set_gpio_level(BUZZER_PIN, 0);
                    falha_registrada_teste = false;
                }
            } else {
                pwm_set_gpio_level(LED_PIN_RED, 0);
                pwm_set_gpio_level(LED_PIN_GREEN, 0);
                pwm_set_gpio_level(LED_PIN_BLUE, 0);
                pwm_set_gpio_level(BUZZER_PIN, 0);
            }

            // display no modo teste
            if (monitoramento_ativo) {
                if (modo_exibicao) {
                    if (para_baixo) {
                        ssd1306_draw_string(&ssd, "ISOLADOR OK!", 0, 24);
                    } else if (para_cima) {
                        if (porcentagem <= 80) {
                            ssd1306_draw_string(&ssd, "ISOLADOR RUIM", 0, 24);
                        } else {
                            ssd1306_draw_string(&ssd, "ESTADO CRITICO", 0, 24);
                        }
                    } else {
                        ssd1306_draw_string(&ssd, "NEUTRO", 0, 24);
                    }
                } else {
                    ssd1306_rect(&ssd, 20, 0, bar_width, 8, true, true);
                    char porcentagem_str[16];
                    snprintf(porcentagem_str, sizeof(porcentagem_str), "%d%%", porcentagem);
                    ssd1306_draw_string(&ssd, porcentagem_str, 0, 30);
                    if (para_baixo) {
                        ssd1306_draw_string(&ssd, "ISOLADOR OK!", 0, 40);
                    } else if (para_cima) {
                        if (porcentagem <= 80) {
                            ssd1306_draw_string(&ssd, "ISOLADOR RUIM", 0, 40);
                        } else {
                            ssd1306_draw_string(&ssd, "ESTADO CRITICO", 0, 40);
                        }
                    } else {
                        ssd1306_draw_string(&ssd, "NEUTRO", 0, 40);
                    }
                }
            } else {
                ssd1306_draw_string(&ssd, "    PAUSADO", 0, 40);
            }
        } 
        else if (estado == DADOS_TENSAO) {
            // modo dados - tela 1: Tensão (0-36kV)
            int tensao = (vrx_value * 36000) / 4095; // 0 a 36kV
            char tensao_str[16];
            snprintf(tensao_str, sizeof(tensao_str), "TENSAO: %dV", tensao);
            ssd1306_draw_string(&ssd, tensao_str, 0, 20);

            if (tensao > 32400) { // >90% (32.4kV)
                ssd1306_draw_string(&ssd, "NAO SUPORTA", 0, 40);
                pwm_set_wrap(pwm_slice_buzzer, 31250); // 4000Hz
                pwm_set_gpio_level(BUZZER_PIN, 15625); // 50% duty cycle
                pwm_set_gpio_level(LED_PIN_RED, 4096); // LED vermelho
                pwm_set_gpio_level(LED_PIN_GREEN, 0);
                if (!falha_registrada_dados) {
                    falhas++;
                    falha_registrada_dados = true;
                }
            } else {
                ssd1306_draw_string(&ssd, "SUPORTA", 0, 40);
                pwm_set_gpio_level(BUZZER_PIN, 0);
                pwm_set_gpio_level(LED_PIN_RED, 0);
                pwm_set_gpio_level(LED_PIN_GREEN, 4096); // LED verde
                falha_registrada_dados = false;
            }
            pwm_set_gpio_level(LED_PIN_BLUE, 0);

            // barra horizontal ao pressionar o joystick
            if (barra_visivel) {
                int bar_width = (vrx_value * 128) / 4095; // 0 a 128 pixels
                ssd1306_rect(&ssd, 30, 0, bar_width, 8, true, true); // barra na linha 30
            }
        } 
        else if (estado == DADOS_FALHAS) {
            // modo Dados - tela 2: histórico de falhas
            char falhas_str[16];
            snprintf(falhas_str, sizeof(falhas_str), "FALHAS: %lu", falhas);
            ssd1306_draw_string(&ssd, falhas_str, 0, 30);
            pwm_set_gpio_level(LED_PIN_RED, 0);
            pwm_set_gpio_level(LED_PIN_GREEN, 0);
            pwm_set_gpio_level(LED_PIN_BLUE, 0);
            pwm_set_gpio_level(BUZZER_PIN, 0);
        }

        ssd1306_send_data(&ssd);

        // UART a cada 1s
        if (current_time - last_print_time >= 1000) {
            printf("VRY: %u, VRX: %u, Estado: %d, Falhas: %lu\n", vry_value, vrx_value, estado, falhas);
            last_print_time = current_time;
        }

        sleep_ms(100);
    }

    return 0;
}