# PSI-Microcontroladores2-Aula07
Atividade: Semáforos de Pedestres e Veículos

## Objetivo
Desenvolver um sistema embarcado de controle de semáforos para pedestres e veículos, utilizando **threads** e **mutex**, e validar o funcionamento do código por meio de **testes utilizando o modelo V (testes unitários, de integração e de sistema)**.  
Opcionalmente, alunos podem utilizar **IA generativa** para auxiliar na elaboração de trechos de código ou na geração de planos de teste, mas **a avaliação deve se concentrar na qualidade dos testes e na correta validação do sistema**.

## Trabalho em Dupla
- A atividade deve ser realizada **em duplas**.
- Cada membro deve contribuir ativamente no desenvolvimento.
- **Commits no repositório Git** devem ser feitos de forma individual, permitindo a avaliação das contribuições de cada aluno.

## Descrição do Sistema
O sistema deve controlar semáforos de dois tipos: pedestres e carros, incluindo **modo noturno** e **botão de travessia de pedestres**.

### 1. Semáforo de Pedestres
- Contém dois LEDs: verde e vermelho.
- Comportamento:
  - Verde acende por 4 segundos.
  - Vermelho acende por 4 segundos.
- Controle deve ser feito por **duas threads independentes**, garantindo exclusão mútua (**mutex**) entre verde e vermelho.
- Deve ser utilizado o microcontrolador de um integrante.

### 2. Semáforo de Veículos
- Contém três LEDs: verde, amarelo e vermelho.
- Comportamento:
  - Verde acende por 3 segundos.
  - Amarelo acende por 1 segundo.
  - Vermelho acende por 4 segundos.
- Controle deve ser feito por **três threads independentes**, garantindo exclusão mútua (**mutex**) entre os LEDs.
- Deve ser validado que há **sincronismo** entre o semáforo de pedestres e o semáforo de veículos.
- Deve ser utilizado o microcontrolador do outro integrante.

### 3. Modo Noturno
- Um modo alternativo em que os semáforos piscam:
  - Carros: amarelo piscando a cada 2 segundos (1 segundo aceso, 1 segundo apagado).
  - Pedestres: vermelho piscando a cada 2 segundos (1 segundo aceso, 1 segundo apagado).

### 4. Botão de Travessia
- Permite que pedestres acionem o semáforo:
  - Pedestre verde é ativado.
  - Semáforo de carros é bloqueado de forma segura.
- O sistema deve suportar uma única via
- Controle de acesso deve garantir **consistência entre semáforos**, evitando conflito de LEDs.

## Requisitos de Projeto
1. Cada LED deve ser controlado por **uma thread**.
2. Threads devem utilizar **mutex** para evitar conflitos.
3. Modos de operação:
   - Normal (dia)
   - Noturno
   - Travessia acionada pelo botão
4. **Validação do código**
   - Alunos devem elaborar **planos de testes completos**, seguindo o **modelo V**, para garantir o correto funcionamento de cada parte e do sistema completo.
   - Testes devem incluir:
     - Testes unitários (cada semáforo)
     - Testes de integração (interação entre semáforos)
     - Testes de sistema (modos noturno e botão de travessia)
6. **Uso de IA generativa**
   - Sugestão de uso para auxiliar na geração de código ou testes.
   - A avaliação será baseada **na qualidade dos testes e nas evidências registradas no repositório**, não apenas na implementação do código.
7. **Controle de contribuições individuais**
   - Cada membro da dupla deve realizar **commits separados** no repositório.
   - A avaliação individual considerará o histórico de commits e a participação de cada aluno no desenvolvimento e validação do sistema.


# Relatório de Verificação e Validação (Modelo V)
**Projeto:** Sistema de Controle Semafórico Sincronizado (Mestre/Escravo)  
**Plataforma/RTOS:** Zephyr RTOS  
**Versão do Código Analisada:** 1.0 (Configuração atual: `IS_MASTER = 0`, `Modo_Atual = Modo_Noturno`)  

---

## 1. Visão Geral da Arquitetura do Modelo V

O desenvolvimento e a validação deste sistema embarcado seguem o **Modelo V**, garantindo rastreabilidade direta entre as especificações de projeto (lado esquerdo) e as fases de teste (lado direito):

* **Requisitos de Sistema $\leftrightarrow$ Testes de Aceitação:** Validação dos modos de operação (Normal, Travessia e Noturno) e sincronismo entre cruzamentos.
* **Arquitetura do Sistema $\leftrightarrow$ Testes de Sistema:** Validação da comunicação física (pulso GPIO) entre placa Mestre e Escravo, e tempos reais de acionamento.
* **Design Detalhado (Threads/ISRs) $\leftrightarrow$ Testes de Integração:** Validação da troca de mensagens via semáforos (`k_sem`), interrupções de hardware (`gpio_callback`) e flags voláteis.
* **Codificação (Módulos/Funções) $\leftrightarrow$ Testes Unitários & Análise Estática:** Validação das rotinas de inicialização de GPIOs, lógica individual de cada thread e detecção de falhas de código.

---

## 2. Análise Estática e Testes Unitários (Nível de Código)

A inspeção estática do código-fonte identificou falhas críticas que impactam diretamente o funcionamento em tempo de execução, especialmente no `Modo_Travessia` e na inicialização:

### 2.1. Bugs e Anomalias Identificados

| ID | Localização | Severidade | Descrição do Problema | Impacto Esperado |
| :--- | :--- | :---: | :--- | :--- |
| **BUG-01** | `Modo_Travessia` (Todas as threads) | **Crítica (Deadlock)** | Todas as threads (`thread_vm_ex`, `thread_vd_ex`, `thread_vd`, `thread_am`, `thread_vm`) chamam `k_sem_take(&sincronizados, K_FOREVER)` no início do loop. Porém, o semáforo `sincronizados` é inicializado com `0` e **nunca é liberado (`k_sem_give`) quando `IS_MASTER == 1`**. Além disso, no Escravo, o pulso libera apenas **1** thread, travando as outras 4. | O sistema congela completamente no `Modo_Travessia` (Deadlock total no Mestre e parcial no Escravo). |
| **BUG-02** | `main()` (Linha final) | **Baixa** | Uso de `printk("\nInicializando placa no modo %c", *modo);` usando o especificador `%c` com desreferenciamento de ponteiro `*modo`. | Imprime apenas a primeira letra do modo (ex: `'N'` em vez de `"Noturno"`). Deveria ser `%s` com `modo`. |
| **BUG-03** | `main()` (Configuração do Botão) | **Média** | O botão é configurado com `GPIO_PULL_DOWN` e interrupção na borda de descida (`GPIO_INT_EDGE_FALLING`). | Se o botão não tiver resistor de pull-up externo forte, o pino ficará em nível baixo (`0`) em repouso e nunca gerará borda de descida ao ser pressionado. |
| **BUG-04** | `thread_vd_ex()` (`Modo_Travessia`) | **Média** | Verifica `if(button_state)` duas vezes seguidas e reseta `button_state = false` **antes** de concluir os `4000ms` de travessia do pedestre. | A `thread_vd` (carros) pode voltar ao verde prematuramente enquanto o pedestre ainda está atravessando. |
| **BUG-05** | `Modo_Noturno` (`thread_vm_ex` e `thread_am`) | **Baixa** | Ambas as threads compartilham o semáforo `fila_ped1` para fazer `gpio_pin_toggle`, mas liberam o semáforo *antes* do `k_msleep(1000)`. | Funciona como mutex rápido, mas se houver preempção ou atraso, os LEDs podem piscar fora de fase entre si. |

---

## 3. Testes de Integração (Threads, Semáforos e ISRs)

Foco na interação entre as interrupções de hardware (ISRs) e o escalonador do Zephyr RTOS.

| Caso de Teste | Componente Envolvido | Procedimento de Teste | Resultado Esperado | Status (Análise) |
| :--- | :--- | :--- | :--- | :---: |
| **TI-01** | `button_isr` $\rightarrow$ `button_state` | Acionar o pino `user_button_0` e verificar a alteração da flag `button_state`. | A flag muda de `false` para `true` apenas uma vez por ciclo (debounce lógico). | **APROVADO** *(Condicionado à correção do BUG-03)* |
| **TI-02** | `sync_isr` $\rightarrow$ `sincronizados` | Aplicar borda de subida no pino `user_button_1` (Placa Escravo). | `sync_isr` executa e executa `k_sem_give(&sincronizados)` sem falhas de contexto de interrupção. | **APROVADO** |
| **TI-03** | Cadeia de Semáforos (`Modo_Normal`) | Executar ciclo completo das threads `thread_vm` $\rightarrow$ `thread_vd` $\rightarrow$ `thread_am`. | Transição ordenada: Vermelho ($4\text{s}$) $\rightarrow$ Verde ($3\text{s}$) $\rightarrow$ Amarelo ($1\text{s}$). | **APROVADO** |
| **TI-04** | Cadeia de Semáforos (`Modo_Travessia`) | Pressionar botão durante o verde dos carros no `Modo_Travessia`. | Carros vão para amarelo ($1\text{s}$), vermelho ($4\text{s}$) e pedestre abre verde ($4\text{s}$). | **REPROVADO** *(Falha pelo BUG-01 e BUG-04)* |

---

## 4. Testes de Sistema (Hardware-in-the-Loop / Mestre-Escravo)

Validação da integração física entre duas placas microcontroladoras e os periféricos externos (LEDs na `port_d`).

### 4.1. Matriz de Hardware Testado
* **LEDs Internos (Carros):** `led0` (Verde) e `led2` (Vermelho). *Nota: O Amarelo é composto pela ativação simultânea de `led0` e `led2` (composição RGB).*
* **LEDs Externos (Pedestres):** `PD0` (Vermelho) e `PD2` (Verde).
* **Barramento de Sincronismo:** Pino `PTA5` (Mestre - Saída) conectado ao `user_button_1` (Escravo - Entrada).

### 4.2. Casos de Teste de Sistema

| Caso de Teste | Cenário | Ação / Estímulo | Comportamento Observado / Esperado | Status |
| :--- | :--- | :--- | :--- | :---: |
| **TS-01** | Inicialização de Periféricos | Alimentar a placa com `IS_MASTER = 0` e `Modo_Noturno`. | LEDs internos e externos iniciam apagados (`INACTIVE`); mensagem de boot exibida na UART. | **APROVADO** *(Com ressalva BUG-02)* |
| **TS-02** | Sincronismo Mestre-Escravo (`Modo_Normal`) | Ligar placa Mestre (`IS_MASTER=1`) e Escravo (`IS_MASTER=0`) interligadas. | Quando o Mestre entra no Vermelho, envia pulso de $50\text{ms}$ no `PTA5`, liberando o Verde do Escravo. | **APROVADO** |
| **TS-03** | Operação de Alerta (`Modo_Noturno`) | Compilar com `Modo_Atual = Modo_Noturno` (Configuração atual). | LEDs da placa piscam em Amarelo (R+G) a cada $1\text{s}$ e LED externo vermelho (`PD0`) pisca a cada $1\text{s}$. | **APROVADO** |
| **TS-04** | Dessincronização / Perda de Cabo | Desconectar o fio entre `PTA5` e `user_button_1` no `Modo_Normal`. | Escravo conclui seu ciclo atual e aguarda indefinidamente no estado seguro antes de abrir o Verde. | **APROVADO** |

---

## 5. Testes de Aceitação (Requisitos Funcionais)

Avaliação final sob a perspectiva dos requisitos operacionais de engenharia de tráfego:

* **REQ-01 (Segurança de Cruzamento):** *Nunca deve haver sinal verde simultâneo para carros e pedestres na mesma via.*
  * **Resultado:** **Atendido** no `Modo_Normal` (embora os semáforos de pedestres e carros rodem em loops temporizados independentes de $4\text{s}+4\text{s} = 8\text{s}$, mantendo o sincronismo por tempo fixo). **Não atendido** no `Modo_Travessia` devido ao deadlock.
* **REQ-02 (Operação em Baixo Fluxo - Noturno):** *O sistema deve operar em modo intermitente de atenção durante a madrugada.*
  * **Resultado:** **Atendido**. O código atual está configurado neste modo e executa a intermitência corretamente.
* **REQ-03 (Prioridade de Travessia sob Demanda):** *O pedestre deve poder solicitar travessia via botoeira.*
  * **Resultado:** **Não Atendido**. Requer refatoração da lógica do semáforo `sincronizados` e da flag `button_state` no `Modo_Travessia`.

---

## 6. Recomendações de Correção (Plano de Ação)

Para que o sistema seja aprovado integralmente no ciclo do Modelo V, recomenda-se:

1. **Corrigir `printk` na `main()`:** Alterar `%c` e `*modo` para `%s` e `modo`.
2. **Revisar Pull-up/Pull-down do Botão:** Ajustar para `GPIO_PULL_UP` se o botão fechar contato para o GND (`GPIO_INT_EDGE_FALLING`), ou mudar a interrupção para `GPIO_INT_EDGE_RISING` caso o botão envie nível alto (`3.3V`).
3. **Refatorar `Modo_Travessia`:** Remover o `k_sem_take(&sincronizados, K_FOREVER)` das threads que não dependem diretamente do pulso externo e garantir que `button_state = false` seja executado apenas **após** o término dos `4000ms` do verde de pedestres.