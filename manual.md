### Manual do Usuário — Interpretador de Instruções (Arduino Mega 2560)

**Resumo rápido**  
O protótipo implementa um interpretador de instruções com **modo LOAD** (entrada de programa) e **modo RUN** (execução passo a passo). O modo LOAD é ativado pelo caractere `#` e permite armazenar instruções em memória de programa; o RUN inicia a execução controlada pelo contador de programa.

---

### 1. Visão geral do funcionamento

- **Memória de programa**: vetor `programa[16]` que armazena até 16 instruções em formato (opcode, operando).
- **Memória de dados**: vetor `MEM[16]` para armazenar valores via `STORE`/`LOADM`.
- **Registradores simulados**: **PC** (contador de programa), **IR** (registrador de instrução), **ACC** (acumulador), **FLAG_Z** (flag zero).
- **Entradas/Saídas**: teclado matricial 4×4, display de 7 segmentos (pinos 22–29), LEDs (42–44), buzzer (45), sensor ultrassônico TRIG/ECHO (40/41).

---

### 2. Como operar o protótipo — passo a passo

#### 2.1 Iniciar e entrar em LOAD

1. Abra o **Serial Monitor** (baud 9600).
2. Pressione `#` no teclado. O Serial mostrará **Início LOAD** e cada tecla pressionada será ecoada.
3. Digite instruções usando o formato delimitado por `#`:
   - **Formato por instrução**: `#opcode#operando#`
   - **Operando opcional**: para instruções sem operando use `#opcode##` (duplo `#`).
   - Exemplo: `#2#6#` → armazena `LOADK 6`. O Serial exibirá o mnemônico correspondente.
4. **HALT obrigatório**: inclua `#15#` como última instrução do programa; ao armazenar `15` o LOAD é finalizado automaticamente e o Serial mostrará **Fim LOAD**.

#### 2.2 Iniciar RUN e executar passo a passo

1. Após **Fim LOAD**, pressione `A` para **Início RUN**. O sistema define `PC = 0` e fica aguardando comandos.
2. Pressione `*` para executar **uma** instrução (um ciclo) por vez. A cada `*` o Serial imprime o estado: **PC**, **IR**, **ACC**, **FLAG_Z** e conteúdo relevante de `MEM`.
3. Quando a instrução `HALT` for executada, a execução é interrompida e novos `*` são ignorados até novo `A`.
4. Pressione `B` a qualquer momento para **reiniciar o sistema** (limpa programa, MEM e registradores).

---

### 3. Conjunto de instruções (resumo e exemplos de uso)

Cada instrução é informada por seu **opcode decimal**. Abaixo o mnemônico, comportamento e exemplo de entrada no LOAD.

| Opcode | Mnemônico  | O que faz                                             | Exemplo no LOAD                 |
| -----: | ---------- | ----------------------------------------------------- | ------------------------------- |
|      0 | **NOP**    | Não faz nada                                          | `#0##`                          |
|      1 | **READ**   | Lê sensor ultrassônico; resultado em **ACC**          | `#1##`                          |
|      2 | **LOADK**  | Carrega constante em **ACC**                          | `#2#6#` → ACC = 6               |
|      3 | **ADDK**   | Soma constante a **ACC**                              | `#3#3#` → ACC += 3              |
|      4 | **SUBK**   | Subtrai constante de **ACC**                          | `#4#2#` → ACC -= 2              |
|      5 | **CMPK**   | Compara ACC com constante; atualiza **FLAG_Z**        | `#5#10#` → FLAG_Z = (ACC == 10) |
|      6 | **LEDON**  | Liga LED especificado pelo operando (1,2 ou 3)        | `#6#1#` → liga LED1             |
|      7 | **LEDOFF** | Desliga LED especificado (1,2 ou 3)                   | `#7#1#` → desliga LED1          |
|      8 | **BUZON**  | Liga buzzer                                           | `#8##`                          |
|      9 | **BUZOFF** | Desliga buzzer                                        | `#9##`                          |
|     10 | **DISP**   | Exibe valor atual de **ACC** no display (1 dígito)    | `#10##` (mostra ACC)            |
|     11 | **ALERT**  | Avalia ACC (distância) e aciona saídas conforme faixa | `#11##`                         |
|     12 | **BINC**   | Imprime opcode binário (4 bits) no Serial             | `#12##`                         |
|     13 | **STORE**  | MEM[X] = ACC (X = operando 0..15)                     | `#13#2#` → MEM[2] = ACC         |
|     14 | **LOADM**  | ACC = MEM[X] (X = operando 0..15)                     | `#14#2#` → ACC = MEM[2]         |
|     15 | **HALT**   | Encerra execução                                      | `#15##` (final do programa)     |

**Observação importante sobre operandos de I/O (LEDs, etc.)**  
Os operandos **não** representam pinos físicos do Arduino; representam **índices lógicos** definidos pelo projeto. Por exemplo, `LEDON 1` liga o LED identificado como **LED1** (conectado ao pino 42). O motivo é separar a **ISA** (instruções e operandos) da pinagem física: o operando indica qual dispositivo lógico acionar, e o código mapeia esse índice para o pino correspondente. Isso facilita a leitura do programa e mantém a abstração arquitetural exigida pelo trabalho.

---

### 4. Regras e comportamentos do display de 7 segmentos

- O display mostra **apenas um dígito decimal** (0–9).
- **Overflow**: se o valor a exibir for maior que 9, o display mostra `E` e o Serial imprime `OVERFLOW`.
- **Valor negativo**: se ACC < 0, o display mostra `-` e o Serial imprime `NEGATIVO`.
- **Fluxo típico para exibir**: primeiro carregue ou calcule um valor em **ACC** (por exemplo `LOADK`, `READ`, `ADDK`, `LOADM`), depois execute `DISP` para enviar esse valor ao display.

---

### 5. Comportamento da instrução ALERT

- Usa o valor em **ACC** (normalmente obtido por `READ`) para decidir saídas:
  - **ACC < 10 cm** → liga buzzer e LED de alerta;
  - **10 ≤ ACC < 20 cm** → liga LED de alerta apenas;
  - **ACC ≥ 20 cm** → mantém saídas desligadas.
- Execute `READ` antes de `ALERT` para garantir que ACC contenha a leitura mais recente.

---

### 6. Testes recomendados (baseados no Documento de Requisitos)

#### Teste 1 — Codificação (F02)

- **Objetivo**: mostrar que o sistema converte opcode para binário.
- **Procedimento**: em LOAD insira `#12##` (BINC). Finalize com `#15#`. Em RUN, pressione `*` até executar `BINC`. O Serial deve imprimir o opcode em 4 bits.

#### Teste 2 — Leitura do sensor (F05)

- **Objetivo**: `READ` carrega distância em **ACC**.
- **Procedimento**: em LOAD insira `#1##` seguido de `#10##` (opcional DISP) e `#15#`. Em RUN, pressione `*` para executar `READ`; verifique no Serial `Distancia lida (cm):` e que **ACC** foi atualizado.

#### Teste 3 — Operações da ULA (F04)

- **Objetivo**: demonstrar `ADDK` e `SUBK`.
- **Procedimento**: carregar `#2#5#` (LOADK 5), `#3#3#` (ADDK 3), `#4#2#` (SUBK 2), `#10##` (DISP), `#15#`. Em RUN, execute passo a passo e observe ACC e display.

#### Teste 4 — Comparação (F04)

- **Objetivo**: `CMPK` atualiza `FLAG_Z`.
- **Procedimento**: carregue ACC com `LOADK` e depois `#5#valor#` (CMPK). Execute e verifique `FLAG_Z` no Serial.

#### Teste 5 — Memória (F09)

- **Objetivo**: `STORE` e `LOADM`.
- **Procedimento**: `LOADK 7` → `STORE 2` → `LOADK 0` → `LOADM 2` → verificar ACC = 7.

#### Teste 6 — Saída visual e sonora (F06 / F07)

- **Objetivo**: `LEDON`, `LEDOFF`, `BUZON`, `BUZOFF`.
- **Procedimento**: `#6#1#` (LEDON 1), `#7#1#` (LEDOFF 1), `#8##` (BUZON), `#9##` (BUZOFF).

#### Teste 7 — ALERT (F10)

- **Objetivo**: demonstrar comportamento automático por faixa.
- **Procedimento**: faça `READ` para atualizar ACC e execute `ALERT`; repita aproximando/afastando o objeto do sensor para cobrir as três faixas.

#### Teste 8 — Display (F08), Overflow e Negativo (F05 / 2.5)

- **Objetivo**: mostrar exibição, overflow e negativo.
- **Procedimento**:
  - **Valor válido**: `LOADK 5` + `DISP` → display mostra 5.
  - **Overflow**: `LOADK 12` + `DISP` → display mostra `E` e Serial `OVERFLOW`.
  - **Negativo**: `LOADK 2` + `SUBK 5` + `DISP` → display mostra `-` e Serial `NEGATIVO`.

#### Teste 9 — HALT (F11)

- **Objetivo**: encerrar execução.
- **Procedimento**: inclua `#15#` no final do programa; em RUN execute até HALT; verifique que novos `*` não têm efeito até novo `A`.

---

### 7. Dicas e resolução de problemas comuns

- **Por que `LEDON 1` e não o pino?** — o operando identifica o **dispositivo lógico** (LED1, LED2, LED3). O mapeamento para pinos é fixo no sketch (LED1 → pino 42). Isso mantém a ISA independente da pinagem física.
- **Nada aparece no display após `DISP`** — verifique se **ACC** contém um valor válido antes de `DISP`. Use `LOADK` ou `READ`/`LOADM` para preencher ACC.
- **Teclas não são reconhecidas** — confira conexões do teclado (pinos 30–37) e que o Keypad está corretamente alimentado.
- **Sensor sem leitura (timeout)** — se `pulseIn` retornar 0 o Serial mostrará timeout; verifique fiação TRIG/ECHO e alimentação do sensor.
- **Programa não executa `*`** — confirme que pressionou `A` para iniciar RUN; `*` só executa instruções quando `EXECUTANDO == true`.
- **Memória cheia** — máximo 16 instruções; o sistema avisa se tentar armazenar mais.

---

### 8. Referência rápida de comandos no teclado

- `#` → alterna/entra em **LOAD**; durante LOAD é usado como delimitador de instrução; inserir `#15#` finaliza LOAD.
- `A` → inicia **RUN** (PC = 0).
- `*` → executa **uma** instrução (passo a passo) quando em RUN.
- `B` → reinicia/limpa sistema (apaga programa e MEM).
- Cada tecla pressionada é ecoada no Serial para facilitar a digitação.

---

### 9. Exemplo completo de sessão (exemplo prático)

1. Pressione `#` → Serial: **Início LOAD**.
2. Digite: `#2#5#` → Serial: `Mnemônico Correspondente: LOADK 5`.
3. Digite: `#3#3#` → Serial: `Mnemônico Correspondente: ADDK 3`.
4. Digite: `#10##` → Serial: `Mnemônico Correspondente: DISP`.
5. Digite: `#15#` → Serial: `Mnemônico Correspondente: HALT` e **Fim LOAD**.
6. Pressione `A` → Serial: **Início RUN**.
7. Pressione `*` → executa `LOADK 5`, Serial mostra estado.
8. Pressione `*` → executa `ADDK 3`, ACC = 8.
9. Pressione `*` → executa `DISP`, display mostra 8.
10. Pressione `*` → executa `HALT`, execução encerrada.

---

### 10. Observações finais para a apresentação

- Durante a demonstração, aponte no sketch onde estão implementados **MEM**, **PC**, **IR**, **ACC**, **UC** (funções de ciclo e controle) e **ULA** (funções aritméticas e comparação).
- Explique a separação entre **memória de programa** (`programa[]`) e **memória de dados** (`MEM[]`).
- Mostre os testes listados na seção 6 na ordem solicitada pelo professor.

---

Se quiser, eu transformo este manual em um arquivo `.txt` formatado pronto para download (com título, instruções e exemplos) ou adapto o texto para um `README.md` com imagens e diagramas de pinagem.
