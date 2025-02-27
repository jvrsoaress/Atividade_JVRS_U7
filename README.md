<img width=100% src="https://capsule-render.vercel.app/api?type=waving&color=02A6F4&height=120&section=header"/>
<h1 align="center">Embarcatech Atividade - U4C8 </h1>

## Objetivo do Projeto


O projeto "Detector de Falhas em Isoladores Elétricos com Feedback Visual e Serial" é um sistema embarcado desenvolvido para simular a detecção de falhas em isoladores elétricos, componentes essenciais em sistemas de distribuição de energia. 
Utilizando a placa BitDogLab, o objetivo principal é demonstrar como técnicas avançadas de análise podem ser adaptadas a um sistema embarcado de baixa complexidade, servindo como uma ferramenta educacional e funcional para monitoramento de isoladores.

## 🗒️ Lista de requisitos

- **Uso de interrupções**: Todas as funcionalidades relacionadas aos botões devem ser implementadas utilizando rotinas de interrupção (IRQ); 
- **Debouncing**: É obrigatório implementar o tratamento do bouncing dos botões via software; 
- **Utilização do Display 128 x 64**: A utilização de fontes maiúsculas e minúsculas demonstrará o domínio do uso de bibliotecas, o entendimento do princípio de funcionamento do display, bem como, a utilização do protocolo I2C.
- **Envio de informação pela UART**: Visa observar a compreensão sobre a comunicação serial via UART. 
- **Organização do código**: O código deve estar bem estruturado e comentado para facilitar o entendimento.

## 🛠 Tecnologias

1. **Microcontrolador**: Raspberry Pi Pico W (na BitDogLab).
2. **Display OLED SSD1306**: 128x64 pixels, conectado via I2C (GPIO 14 - SDA, GPIO 15 - SCL).
3. **LEDs RGB**:
- Vermelho: GPIO 13 (PWM).
- Verde: GPIO 11 (PWM).
4. **Joystick**:
- Eixo Y (VRY): GPIO 26 (ADC).
- Eixo X (VRX): GPIO 27 (ADC, usado no modo Dados).
- Botão (SW): GPIO 22 (interrupção).
5. **Botões**:
- Botão A: GPIO 5 (interrupção).
- Botão B: GPIO 6 (interrupção).
6. **Buzzer**: GPIO 10 (PWM)
7. **Linguagem de Programação:** C  
8. **Frameworks:** Pico SDK


## 🔧 Funcionalidades Implementadas:

**Funções dos Componentes**
   
- Display: Exibe o menu inicial e as telas de cada modo (Teste e Dados).
- LEDs: Indicam estados (verde = OK, vermelho = falha/crítico).
- Joystick: Controla valores (Teste: condição do isolador; Dados: tensão simulada).
- Botões: Navegam entre modos e telas.
- Buzzer: Emite alerta agudo (4000Hz) em condições críticas.

## 🔧 Fluxograma Geral:

1.	Inicialização: Configura GPIO, ADC, PWM, I2C e interrupções.
2.	Menu Inicial: Exibe opções e espera entrada do usuário.
3.	Modo Teste:
- Lê eixo Y, calcula porcentagem, controla LEDs/buzzer, atualiza display, conta falhas.
- Alterna entre ativo e pausado com botão A.
4.	Modo Dados:
- Tela 1: Lê eixo X, calcula tensão, controla LEDs/buzzer, atualiza display, conta falhas.
- Tela 2: Exibe histórico de falhas.
- Alterna entre Tela 1 e Tela 2 com botão B.
5.	Volta ao Menu: Pressão longa (3s) em A ou B.



## 🚀 Passos para Compilação e Upload  

1. **Clonar o repositório** 

- sh
- git clone seu repositorio


2. **Configurar e compilar o projeto**  

`mkdir build && cd build`
`cmake ..`
`make`

3. **Transferir o firmware para a placa**

- Conectar a placa BitDogLab ao computador
- Copiar o arquivo .uf2 gerado para o drive da placa

4. **Testar o projeto**

🛠🔧🛠🔧🛠🔧


## 🎥 Demonstração: 

- Para ver o funcionamento do projeto, acesse o vídeo de demonstração gravado por José Vinicius em: [https://youtu.be/YmlDdGBSoDU](https://youtu.be/nGrDLF12td4)

## 💻 Desenvolvedor
 
<table>
  <tr>
    <td align="center"><img style="" src="https://avatars.githubusercontent.com/u/191687774?v=4" width="100px;" alt=""/><br /><sub><b> José Vinicius </b></sub></a><br />👨‍💻</a></td>
  </tr>
</table>
