/*
  Sketch para o teclado matricial 4x4 e interpretação de instruções
  - interpretação separada da execução
  - a função interpretarInstrucao retorna um objeto da classe InstrucaoInterpretada, sem efeitos colaterais
  - a função executarInstrucao usa switch e delega para handlers
*/

#include <Keypad.h>

// ---------------------------
// Configuração do teclado
// ---------------------------
const byte ROWS = 4;
const byte COLS = 4;

// Mapa físico das teclas (ordem: linhas x colunas)
char mapaTeclas[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

// Pinos do Arduino usados para as linhas e colunas do teclado
byte rowPins[ROWS] = { 29, 31, 33, 35 };  // pinos das linhas
byte colPins[COLS] = { 37, 39, 41, 43 };  // pinos das colunas

// Instância da biblioteca Keypad
Keypad teclado = Keypad(makeKeymap(mapaTeclas), rowPins, colPins, ROWS, COLS);

// -----------------------------
// Tabela de mnemônicos (0..15)
// -----------------------------
const char* MNEMONICOS[16] = {
  "NOP",     // 0
  "READ",    // 1
  "LOADK",   // 2
  "ADDK",    // 3
  "SUBK",    // 4
  "CMPK",    // 5
  "LEDON",   // 6
  "LEDOFF",  // 7
  "BUZON",   // 8
  "BUZOFF",  // 9
  "DISP",    // 10
  "ALERT",   // 11
  "BINC",    // 12
  "STORE",   // 13
  "LOADM",   // 14
  "HALT"     // 15
};

// --------------------------------------
// Buffer global para entrada do teclado
// --------------------------------------
String bufferEntrada = "";  // acumula sequência de teclas até formar instrução completa

// ---------------------------------------------
// Struct que representa instrução interpretada
// ---------------------------------------------
struct InstrucaoInterpretada {
  bool valida;      // true se parsing e validação OK
  int opcode;       // opcode decimal (0..15)
  String operando;  // operando como texto (pode ficar vazio)
};

// ---------------------------
// Protótipos de funções
// ---------------------------
InstrucaoInterpretada interpretarInstrucao(const String& sequencia);
bool ehNumerico(const String& s);
void executarInstrucao(int opcode, const String& operando);

// Handlers (esqueleto) para manter executarInstrucao enxuta
void handlerNOP();
void handlerREAD();
void handlerLOADK(const String& operando);
void handlerADDK(const String& operando);
// Adicione outros handlers conforme for implementando

// --------------
// setup
// --------------
void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }  // espera Serial
  Serial.println();
  Serial.println("=== Interpretador de instrucoes via teclado (delimitador '#') ===");
  Serial.println("Exemplo: #3##12#  -> ADDK 12");
  Serial.println("Pressione teclas no teclado matricial...");
}

// ----------
// loop
// ----------
void loop() {
  char tecla = teclado.getKey();

  if (tecla) {
    // Eco da tecla e atualização do buffer
    Serial.print("Tecla: ");
    Serial.println(tecla);

    bufferEntrada += tecla;

    Serial.print("Buffer atual: ");
    Serial.println(bufferEntrada);

    // Condição para considerar instrução completa:
    // começa com '#' e termina com '#' e tem mais de 1 caractere
    if (bufferEntrada.length() > 1 && bufferEntrada.charAt(0) == '#' && bufferEntrada.charAt(bufferEntrada.length() - 1) == '#') {

      // Interpreta a sequência e recebe um struct com dados
      InstrucaoInterpretada instrucao = interpretarInstrucao(bufferEntrada);
      // Limpa buffer para próxima instrução
      bufferEntrada = "";

      if (instrucao.valida) {
        // Chama rotina de execução
        executarInstrucao(instrucao.opcode, instrucao.operando);
        Serial.println("--- Pronto para próxima instrução ---");
      } else {
        Serial.println("Interpretação falhou; instrução não executada.");
      }
    }
  }

  delay(10);  // pequena pausa para reduzir sobrecarga
}

// -----------------------------------------------------
// Função interpretarInstrucao
// - recebe string como "#3##12#"
// - retorna InstrucaoInterpretada com opcode e operando (se houver)
// - não realiza execução, apenas valida e extrai tokens
// -----------------------------------------------------
InstrucaoInterpretada interpretarInstrucao(const String& sequencia) {
  InstrucaoInterpretada resultado;
  resultado.valida = false;
  resultado.opcode = 0;
  resultado.operando = "";

  // --- variáveis locais para interpretação ---
  int tamanho = sequencia.length();  // comprimento da string recebida
  String tokenAtual = "";            // acumula caracteres entre '#'s
  String tokens[2];                  // 1 opcode e 1 operando, no máximo
  int contadorTokens = 0;            // quantos tokens válidos foram coletados

  // --- percorre cada caractere da sequência e separa por '#' ---
  for (int i = 0; i < tamanho; ++i) {
    char c = sequencia.charAt(i);

    if (c == '#') {
      // delimitador encontrado: se havia algo em tokenAtual, salva
      if (tokenAtual.length() > 0) {
        if (contadorTokens < 2) {
          tokens[contadorTokens] = tokenAtual;
          contadorTokens++;
        }
        tokenAtual = "";  // zera para começar novo token
      } else {
        // tokenAtual vazio -> ocorreu "##" ou '#' no início/fim; ignorar
      }
    } else {
      // caractere normal: acumula no token atual
      tokenAtual += c;
    }
  }

  // --- se sobrou um token no final (caso a sequência não termine com '#') ---
  if (tokenAtual.length() > 0 && contadorTokens < 2) {
    tokens[contadorTokens] = tokenAtual;
    contadorTokens++;
  }

  // --- valida existência de pelo menos um token (opcode) ---
  if (contadorTokens == 0) {
    Serial.println("Instrução vazia detectada. Ignorando.");
    return resultado; // resultado.valida == false
  }

  // --- primeiro token deve ser opcode numérico ---
  String textoOpcode = tokens[0];
  if (!ehNumerico(textoOpcode)) {
    Serial.println("Opcode inválido (não numérico): ");
    Serial.println(textoOpcode);
    return resultado; // resultado.valida == false
  }

  int opcode = textoOpcode.toInt();  // converte para inteiro
  // valida intervalo do opcode (0..15)
  if (opcode < 0 || opcode > 15) {
    Serial.print("Opcode fora do intervalo 0-15: ");
    Serial.println(opcode);
    return resultado; // resultado.valida == false
  }

  // --- recupera mnemônico (opcional, para legibilidade e reuso) ---
  const char* mnemonico = MNEMONICOS[opcode];

  // --- se houver segundo token, trata como operando ---
  String textoOperando = "";
  if (contadorTokens >= 2) {
    textoOperando = tokens[1];
    textoOperando.trim(); // remove espaços acidentais
    // se o token ficou vazio após trim, considere que não há operando
    if (textoOperando.length() == 0) {
      textoOperando = "";
    }
  }

  // --- preenche struct de resultado com os valores válidos ---
  resultado.valida = true;
  resultado.opcode = opcode;
  resultado.operando = textoOperando;

  // --- debug / informação legível ---
  Serial.println("=== Instrução reconhecida ===");
  Serial.print("Opcode (decimal): ");
  Serial.println(resultado.opcode);
  Serial.print("Mnemônico: ");
  Serial.println(mnemonico);

  if (resultado.operando.length() > 0) {
    Serial.print("Operando (texto): ");
    Serial.println(resultado.operando);
    if (ehNumerico(resultado.operando)) {
      Serial.print("Operando (decimal): ");
      Serial.println(resultado.operando.toInt());
    } else {
      Serial.println("Operando não é numérico (mantido como string).");
    }
  } else {
    Serial.println("Sem operando.");
  }

  // --- Retorna o resultado já preenchido ---
  return resultado;

  // --- Aqui você pode chamar a rotina que executa a instrução ---
  // executeInstruction(opcode, textoOperando);
}

// ---------------------------
// Função booleana ehNumerico
// - verifica se a string representa um inteiro (opcionalmente com + ou -)
// ---------------------------
bool ehNumerico(const String& s) {
  if (s.length() == 0) return false;
  for (unsigned int i = 0; i < s.length(); ++i) {
    char c = s.charAt(i);
    if (i == 0 && (c == '+' || c == '-')) {
      if (s.length() == 1) return false; // apenas sinal não é número
      continue;
    }
    if (c < '0' || c > '9') return false;
  }
  return true;
}

// ---------------------------
// executarInstrucao
// - dispatcher principal: mantém switch enxuto e delega para handlers
// - cada handler deve ser pequeno e responsável por uma tarefa
// ---------------------------
void executarInstrucao(int opcode, const String& operando) {
  Serial.print("Executando opcode ");
  Serial.print(opcode);
  Serial.print(" (");
  Serial.print(MNEMONICOS[opcode]);
  Serial.println(")");

  if (operando.length() > 0) {
    Serial.print("Operando: ");
    Serial.println(operando);
  }

  switch (opcode) {
    case 0: // NOP
      handlerNOP();
      break;

    case 1: // READ
      handlerREAD();
      break;

    case 2: // LOADK
      handlerLOADK(operando);
      break;

    case 3: // ADDK
      handlerADDK(operando);
      break;

    // Exemplo: outros opcodes devem chamar seus handlers correspondentes
    // case 4: handlerSUBK(operando); break;
    // case 5: handlerCMPK(operando); break;
    // case 6: handlerLEDON(operando); break;
    // ...
    case 15: // HALT
      Serial.println("HALT recebido. (comportamento de parada não implementado)");
      break;

    default:
      Serial.println("Opcode não implementado ainda.");
      break;
  }
}

// ---------------------------
// Handlers (esqueleto) - mantenha cada um pequeno e focado
// ---------------------------
void handlerNOP() {
  Serial.println("NOP -> sem operação.");
}

void handlerREAD() {
  // Aqui você leria o sensor HC-SR04 e atualizaria ACC
  Serial.println("READ -> leitura de sensor solicitada (não implementada).");
}

void handlerLOADK(const String& operando) {
  if (!ehNumerico(operando)) {
    Serial.println("LOADK: operando inválido (não numérico).");
    return;
  }
  int valor = operando.toInt();
  // Exemplo: ACC = valor; (implemente ACC global)
  Serial.print("LOADK -> carregar constante ");
  Serial.println(valor);
}

void handlerADDK(const String& operando) {
  if (!ehNumerico(operando)) {
    Serial.println("ADDK: operando inválido (não numérico).");
    return;
  }
  int valor = operando.toInt();
  // Exemplo: ACC += valor; (implemente ACC global)
  Serial.print("ADDK -> somar constante ");
  Serial.println(valor);
}

// Adicione handlers para SUBK, CMPK, LEDON, LEDOFF, BUZON, BUZOFF, DISP, ALERT, BINC, STORE, LOADM, etc.
