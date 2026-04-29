/*
202502831285 Marcos Paulo Lopes de Assunção - TA
202403191156 Caio Domingues - TA
202307164607 Ricardo Santos Lima - TA
202403088886 Bernardo Meierles - TA
202501001041 Felipe Maia - TA
*/

/*
  Interpretador de instruções para Arduino Mega 2560
  - Implementa elementos arquiteturais: MEM, PC, IR, ACC, FLAG_Z, EXECUTANDO
  - Modo LOAD ativado por '#' (entrada de instruções com delimitadores '#')
  - RUN com tecla 'A', execução passo a passo com '*', reset com 'B'
  - Cada tecla pressionada é exibida no Serial Monitor para feedback
  - Pinagem conforme Documento de Requisitos Técnicos
*/

#include <Keypad.h>

// ============================
// TECLADO (PINAGEM OFICIAL)
// ============================
const byte ROWS = 4;
const byte COLS = 4;

char mapaTeclas[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 30, 31, 32, 33 };
byte colPins[COLS] = { 34, 35, 36, 37 };

Keypad teclado = Keypad(makeKeymap(mapaTeclas), rowPins, colPins, ROWS, COLS);

// ============================
// PINOS (OBRIGATÓRIO)
// ============================
#define TRIG 40
#define ECHO 41

#define LED1 42
#define LED2 43
#define LED3 44

#define BUZZER 45

// display (a-g + ponto decimal)
int displayPins[8] = { 22, 23, 24, 25, 26, 27, 28, 29 };

// ============================
// ELEMENTOS ARQUITETURAIS OBRIGATÓRIOS
// ============================
// Memória de dados simulada (vetor de 16 posições)
int MEM[16];

// Program Counter (PC)
int PC = 0;

// Instruction Register (IR) armazena opcode atual (0-15)
byte IR = 0;

// Acumulador (ACC)
int ACC = 0;

// Flag de zero (FLAG_Z)
bool FLAG_Z = false;

// Indica se o programa está em execução (após RUN e antes de HALT)
bool EXECUTANDO = false;

// ============================
// ESTRUTURAS DE PROGRAMA (memória de programa)
// ============================
struct InstrucaoInterpretada {
  bool valida;
  int opcode;
  String operando;  // string numérica opcional (índice de MEM, constante, etc.)
};

InstrucaoInterpretada programa[16];  // memória de programa (16 instruções)
int tamanhoPrograma = 0;

// ============================
// ESTADOS DE CONTROLE
// ============================
bool modoLOAD = false;                // true quando em modo LOAD
String bufferEntrada = "";            // buffer que acumula teclas durante LOAD
bool haltInseridoNoPrograma = false;  // indica se HALT (15) já foi inserido no programa

// ============================
// MNEMÔNICOS (para exibição)
// ============================
const char* MNEMONICOS[16] = {
  "NOP", "READ", "LOADK", "ADDK", "SUBK", "CMPK",
  "LEDON", "LEDOFF", "BUZON", "BUZOFF",
  "DISP", "ALERT", "BINC", "STORE", "LOADM", "HALT"
};

// ============================
// MAPA DO DISPLAY DE 7 SEGMENTOS
// Índices: 0-9, 10 = E (overflow), 11 = - (negativo)
// Cada linha: a,b,c,d,e,f,g,dp (1 = segmento ligado)
// ============================
byte numeros[12][8] = {
  { 1, 1, 1, 1, 1, 1, 0, 0 },  // 0
  { 0, 1, 1, 0, 0, 0, 0, 0 },  // 1
  { 1, 1, 0, 1, 1, 0, 1, 0 },  // 2
  { 1, 1, 1, 1, 0, 0, 1, 0 },  // 3
  { 0, 1, 1, 0, 0, 1, 1, 0 },  // 4
  { 1, 0, 1, 1, 0, 1, 1, 0 },  // 5
  { 1, 0, 1, 1, 1, 1, 1, 0 },  // 6
  { 1, 1, 1, 0, 0, 0, 0, 0 },  // 7
  { 1, 1, 1, 1, 1, 1, 1, 0 },  // 8
  { 1, 1, 1, 1, 0, 1, 1, 0 },  // 9
  { 1, 0, 0, 1, 1, 1, 1, 0 },  // E (representação de erro/overflow)
  { 0, 0, 0, 0, 0, 0, 1, 0 }   // - (apenas segmento g)
};

// ============================
// SETUP
// ============================
void setup() {
  Serial.begin(9600);

  // pinos do sensor
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // pinos de saída
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  for (int i = 0; i < 8; i++) pinMode(displayPins[i], OUTPUT);

  // inicializa memória e variáveis arquiteturais
  for (int i = 0; i < 16; i++) MEM[i] = 0;
  PC = 0;
  IR = 0;
  ACC = 0;
  FLAG_Z = false;
  EXECUTANDO = false;
  modoLOAD = false;
  tamanhoPrograma = 0;
  bufferEntrada = "";
  haltInseridoNoPrograma = false;

  Serial.println("Sistema pronto. Pressione '#' para iniciar LOAD.");
}

// ============================
// LOOP principal
// - captura tecla, ecoa no Serial, e trata comandos
// ============================
void loop() {
  char tecla = teclado.getKey();

  if (tecla) {
    // ecoa cada tecla no Serial para feedback imediato
    Serial.print("Tecla: ");
    Serial.println(tecla);

    // Se não estiver em LOAD e tecla '#' inicia LOAD
    if (!modoLOAD && tecla == '#') {
      iniciarLOAD();
      return;
    }

    // Se estiver em LOAD, tratamos entrada de instruções por delimitadores '#'
    if (modoLOAD) {
      tratarTeclaEmLOAD(tecla);
      return;
    }

    // Se não estiver em LOAD, tratamos comandos globais:
    // 'A' -> RUN (inicia execução, PC=0)
    // '*' -> passo a passo (executa próxima instrução) somente se EXECUTANDO == true
    // 'B' -> reset / limpar programa e voltar ao estado inicial
    if (tecla == 'A') {
      iniciarRUN();
      return;
    }

    if (tecla == '*') {
      if (EXECUTANDO) {
        cicloInstrucao();  // executa uma instrução
      } else {
        Serial.println("Programa não está em execução. Pressione 'A' para iniciar modo RUN.");
      }
      return;
    }

    if (tecla == 'B') {
      reiniciarSistema();
      return;
    }

    // outras teclas (C, D, números) fora do LOAD são ignoradas ou podem ser usadas futuramente
  }
}

// ============================
// Funções de controle de modos
// ============================
void iniciarLOAD() {
  modoLOAD = true;
  bufferEntrada = "";
  tamanhoPrograma = 0;
  haltInseridoNoPrograma = false;
  EXECUTANDO = false;
  PC = 0;
  Serial.println("Início LOAD");
  Serial.println("Digite instruções no formato: #opcode#operando# (operando opcional). Ex: #2#6# -> LOADK 6");
  Serial.println("Obs: para instrução sem operando use #opcode## (duplo #). HALT = #15##. Após inserir HALT, o LOAD será finalizado automaticamente.");
}

void finalizarLOAD() {
  modoLOAD = false;
  Serial.println("Fim LOAD");
  Serial.print("Programa carregado com ");
  Serial.print(tamanhoPrograma);
  Serial.println(" instruções.");

  // mostra programa carregado em mnemônicos
  for (int i = 0; i < tamanhoPrograma; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(MNEMONICOS[programa[i].opcode]);
    if (programa[i].operando.length() > 0) {
      Serial.print(" ");
      Serial.print(programa[i].operando);
    }
    Serial.println();
  }
  Serial.println("Pressione a tecla 'A' para iniciar o modo RUN.");
}

void iniciarRUN() {
  if (tamanhoPrograma == 0) {
    Serial.println("Nenhum programa carregado. Entre em LOAD e carregue instruções.");
    return;
  }
  PC = 0;
  EXECUTANDO = true;
  Serial.println("Início RUN");
  Serial.println("Pressione '*' para executar a próxima instrução (passo a passo).");
}

void reiniciarSistema() {
  // limpa programa e variáveis, volta ao estado inicial
  tamanhoPrograma = 0;
  for (int i = 0; i < 16; i++) MEM[i] = 0;
  PC = 0;
  IR = 0;
  ACC = 0;
  FLAG_Z = false;
  EXECUTANDO = false;
  modoLOAD = false;
  bufferEntrada = "";
  haltInseridoNoPrograma = false;
  Serial.println("Sistema reiniciado. Pressione '#' para iniciar LOAD.");
}

// ============================
// Tratamento de teclas em modo LOAD
// - aceita delimitadores '#' para separar opcode e operando
// - formato esperado por instrução: #opcode#operando#  (operando opcional)
// - se opcode == 15 (HALT) e instrução inserida, após armazenar HALT o LOAD é finalizado
// ============================
void tratarTeclaEmLOAD(char tecla) {
  // sempre ecoamos tecla (já feito no loop)
  // acumulamos no buffer
  bufferEntrada += tecla;

  // tentamos parsear quantas instruções completas existirem no buffer
  bool instrParseada = true;
  while (instrParseada) {
    instrParseada = tentarParsearInstrucaoDoBuffer();
  }
}

// Tenta extrair a primeira instrução completa do bufferEntrada.
// Retorna true se extraiu (e armazenou) uma instrução, false caso contrário.
bool tentarParsearInstrucaoDoBuffer() {
  // Procurar padrão: '#' ... '#' ... '#'
  int len = bufferEntrada.length();
  int idxStart = bufferEntrada.indexOf('#');
  if (idxStart == -1) return false;

  // procurar próximo '#' após idxStart
  int idxMid = bufferEntrada.indexOf('#', idxStart + 1);
  if (idxMid == -1) return false;  // ainda não fechou opcode

  // procurar terceiro '#' após idxMid
  int idxEnd = bufferEntrada.indexOf('#', idxMid + 1);
  if (idxEnd == -1) return false;  // ainda não fechou possivel operando

  // agora temos pelo menos '# opcode # operando #'
  String opcodeStr = bufferEntrada.substring(idxStart + 1, idxMid);
  String operandoStr = bufferEntrada.substring(idxMid + 1, idxEnd);

  // Se o usuário quiser instrução sem operando, ele pode digitar duplo '#' (operando vazio)
  // Validamos opcode
  opcodeStr.trim();
  operandoStr.trim();

  // Remover a parte processada do buffer
  bufferEntrada = bufferEntrada.substring(idxEnd + 1);

  // Validar opcode numérico
  if (!ehNumerico(opcodeStr)) {
    Serial.print("Instrução inválida (opcode não numérico): ");
    Serial.println(opcodeStr);
    return true;  // removemos do buffer para evitar loop infinito
  }

  int opcode = opcodeStr.toInt();
  if (opcode < 0 || opcode > 15) {
    Serial.print("Opcode fora do intervalo (0-15): ");
    Serial.println(opcode);
    return true;
  }

  // Monta estrutura de instrução
  InstrucaoInterpretada inst;
  inst.valida = true;
  inst.opcode = opcode;
  inst.operando = operandoStr;  // pode ser vazio

  // Armazena se houver espaço
  if (tamanhoPrograma < 16) {
    programa[tamanhoPrograma++] = inst;
    // Exibe mnemônico no Serial com operando (se houver)
    Serial.print("Mnemônico Correspondente: ");
    Serial.print(MNEMONICOS[opcode]);
    if (inst.operando.length() > 0) {
      Serial.print(" ");
      Serial.print(inst.operando);
    }
    Serial.println();
  } else {
    Serial.println("Erro: memoria de programa cheia.");
  }

  // Se opcode for HALT (15), marcamos e finalizamos LOAD
  if (opcode == 15) {
    haltInseridoNoPrograma = true;
    // Ao inserir HALT, finalizamos o LOAD imediatamente
    finalizarLOAD();
  }

  return true;
}

// ============================
// CICLO DE INSTRUCAO (UC)
// - busca instrucao em programa[PC]
// - carrega IR
// - decodifica e executa
// - atualiza PC, ACC, MEM, FLAG_Z, saídas
// ============================
void cicloInstrucao() {
  if (!EXECUTANDO) {
    Serial.println("Nao em execucao.");
    return;
  }

  if (PC < 0 || PC >= tamanhoPrograma) {
    Serial.println("PC fora do programa ou fim do programa. Execucao interrompida.");
    EXECUTANDO = false;
    return;
  }

  InstrucaoInterpretada inst = programa[PC];
  IR = inst.opcode;

  // Executa instrução
  executarInstrucao(IR, inst.operando);

  // Mostra estado após execução
  mostrarEstado();

  // Se foi HALT, interrompe execução
  if (IR == 15) {
    EXECUTANDO = false;
    Serial.println("HALT processado. Execucao encerrada.");
    return;
  }

  // Atualiza PC para próxima instrução
  PC++;
}

// ============================
// EXECUÇÃO DAS INSTRUÇÕES (ULA + E/S)
// ============================
void executarInstrucao(int opcode, const String& op) {
  switch (opcode) {
    case 0:  // NOP
      // não faz nada
      break;

    case 1:  // READ - lê sensor e armazena em ACC
      ACC = lerSensor();
      break;

    case 2:  // LOADK - carrega constante em ACC
      if (ehNumerico(op)) ACC = op.toInt();
      else Serial.println("LOADK sem operando numerico.");
      break;

    case 3:  // ADDK - soma constante ao ACC
      if (ehNumerico(op)) ACC += op.toInt();
      else Serial.println("ADDK sem operando numerico.");
      break;

    case 4:  // SUBK - subtrai constante do ACC
      if (ehNumerico(op)) ACC -= op.toInt();
      else Serial.println("SUBK sem operando numerico.");
      break;

    case 5:  // CMPK - compara ACC com constante e atualiza FLAG_Z
      if (ehNumerico(op)) FLAG_Z = (ACC == op.toInt());
      else Serial.println("CMPK sem operando numerico.");
      break;

    case 6:  // LEDON - liga LED especificado por operando (1,2,3)
      if (ehNumerico(op)) controlarLED(op.toInt(), true);
      else Serial.println("LEDON sem operando numerico.");
      break;

    case 7:  // LEDOFF - desliga LED especificado por operando
      if (ehNumerico(op)) controlarLED(op.toInt(), false);
      else Serial.println("LEDOFF sem operando numerico.");
      break;

    case 8:  // BUZON - liga buzzer
      digitalWrite(BUZZER, HIGH);
      break;

    case 9:  // BUZOFF - desliga buzzer
      digitalWrite(BUZZER, LOW);
      break;

    case 10:  // DISP - exibe ACC no display (tratamento overflow/negativo)
      mostrarDisplay(ACC);
      break;

    case 11:  // ALERT - resposta automática por faixa de distância
      executarAlerta();
      break;

    case 12:  // BINC - exibe opcode binário da instrução atual no Serial
      imprimirOpcodeBinario(IR);
      break;

    case 13:  // STORE - armazena ACC em MEM[X]
      if (ehNumerico(op)) {
        int idx = op.toInt();
        if (idx >= 0 && idx < 16) {
          MEM[idx] = ACC;
          Serial.print("STORE: MEM[");
          Serial.print(idx);
          Serial.print("] = ");
          Serial.println(ACC);
        } else {
          Serial.println("STORE: indice de memoria fora do intervalo (0-15).");
        }
      } else {
        Serial.println("STORE sem operando numerico.");
      }
      break;

    case 14:  // LOADM - carrega ACC com MEM[X]
      if (ehNumerico(op)) {
        int idx = op.toInt();
        if (idx >= 0 && idx < 16) {
          ACC = MEM[idx];
          Serial.print("LOADM: ACC = MEM[");
          Serial.print(idx);
          Serial.print("] = ");
          Serial.println(ACC);
        } else {
          Serial.println("LOADM: indice de memoria fora do intervalo (0-15).");
        }
      } else {
        Serial.println("LOADM sem operando numerico.");
      }
      break;

    case 15:  // HALT - encerra execução (tratamento feito no cicloInstrucao)
      // garante saídas desligadas
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(BUZZER, LOW);
      break;

    default:
      Serial.print("Opcode desconhecido: ");
      Serial.println(opcode);
      break;
  }
}

// ============================
// FUNÇÕES AUXILIARES
// ============================

// Lê sensor ultrassônico HC-SR04 e retorna distância em cm
int lerSensor() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long dur = pulseIn(ECHO, HIGH, 30000);  // timeout 30ms para evitar bloqueio
  if (dur == 0) {
    Serial.println("Sensor: sem leitura (timeout).");
    return -1;
  }
  int dist = (int)(dur * 0.034 / 2.0);

  Serial.print("Distancia lida (cm): ");
  Serial.println(dist);

  return dist;
}

// Executa a instrução ALERT conforme faixas de distância usando ACC como valor lido
void executarAlerta() {
  // assume que ACC já contém a leitura mais recente (READ) ou valor manual
  if (ACC < 0) {
    // leitura inválida
    digitalWrite(LED1, LOW);
    digitalWrite(BUZZER, LOW);
    Serial.println("ALERT: leitura invalida.");
    return;
  }

  if (ACC < 10) {
    digitalWrite(LED1, HIGH);  // LED de alerta
    digitalWrite(BUZZER, HIGH);
    Serial.println("ALERT: distancia < 10cm -> buzzer + LED");
  } else if (ACC < 20) {
    digitalWrite(LED1, HIGH);
    digitalWrite(BUZZER, LOW);
    Serial.println("ALERT: 10cm <= distancia < 20cm -> LED apenas");
  } else {
    digitalWrite(LED1, LOW);
    digitalWrite(BUZZER, LOW);
    Serial.println("ALERT: distancia >= 20cm -> sem alerta");
  }
}

// Controla LEDs 1,2,3 por índice (1,2,3)
void controlarLED(int indice, bool ligar) {
  int pino = -1;
  if (indice == 1) pino = LED1;
  else if (indice == 2) pino = LED2;
  else if (indice == 3) pino = LED3;
  else {
    Serial.println("LED: indice invalido (use 1,2 ou 3).");
    return;
  }

  digitalWrite(pino, ligar ? HIGH : LOW);
  Serial.print("LED ");
  Serial.print(indice);
  Serial.print(ligar ? " ligado" : " desligado");
  Serial.println();
}

// Mostra valor no display de 7 segmentos com tratamento de overflow e negativo
void mostrarDisplay(int valor) {
  if (valor > 9) {
    Serial.println("OVERFLOW: valor maior que 9. Exibindo 'E' no display.");
    desenhar(10);  // E
    return;
  }
  if (valor < 0) {
    Serial.println("NEGATIVO: valor negativo. Exibindo '-' no display.");
    desenhar(11);  // -
    return;
  }
  desenhar(valor);
}

// Desenha número (0-9) ou E(10) ou -(11) no display
void desenhar(int n) {
  if (n < 0 || n > 11) return;
  for (int i = 0; i < 8; i++) {
    digitalWrite(displayPins[i], numeros[n][i] ? HIGH : LOW);
  }
}

// Exibe estado interno no Serial (PC, IR, ACC, FLAG_Z, MEM relevante)
void mostrarEstado() {
  Serial.print("PC: ");
  Serial.print(PC);
  Serial.print(" | IR: ");
  Serial.print(MNEMONICOS[IR]);
  // se instrução tem operando, mostra também (quando aplicável)
  if (PC >= 0 && PC < tamanhoPrograma) {
    String op = programa[PC].operando;
    if (op.length() > 0) {
      Serial.print(" ");
      Serial.print(op);
    }
  }
  Serial.print(" | ACC: ");
  Serial.print(ACC);
  Serial.print(" | FLAG_Z: ");
  Serial.println(FLAG_Z ? 1 : 0);

  // mostra conteúdo relevante da memória (ex.: primeiras 4 posições)
  Serial.print("MEM[0..3]: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(MEM[i]);
    if (i < 3) Serial.print(", ");
  }
  Serial.println();
}

// Imprime opcode em binário de 4 bits no Serial
void imprimirOpcodeBinario(byte opcode) {
  String s = "";
  for (int i = 3; i >= 0; i--) {
    s += ((opcode >> i) & 1) ? '1' : '0';
  }
  Serial.print("Opcode binario (4 bits): ");
  Serial.println(s);
}

// Verifica se string é numérica (apenas dígitos)
bool ehNumerico(const String& s) {
  if (s.length() == 0) return false;
  for (unsigned int i = 0; i < s.length(); i++) {
    if (!isDigit(s[i])) return false;
  }
  return true;
}
