#include <Keypad.h>

const byte ROWS = 4;
const byte COLUMNS = 4;

char keys[ROWS][COLUMNS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 29, 31, 33, 35 };     // Linhas do teclado
byte colPins[COLUMNS] = { 37, 39, 41, 43 };  // Colunas do teclado

Keypad teclado = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLUMNS);

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

String bufferSeq = "";  // acumula sequência de teclas até formar instrução

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }  // espera Serial
  Serial.println();
  Serial.println("=== Parser de instrucoes via teclado (delimitador '#') ===");
  Serial.println("Digite sequencias como: #3##12#  -> ADDK 12");
  Serial.println("Pressione teclas no teclado matricial...");
}

void loop() {
  char tecla = teclado.getKey();

  if (tecla) {
    Serial.print("Tecla: ");
    Serial.println(tecla);

    // Acumula no buffer
    bufferSeq += tecla;

    // Mostra buffer atual
    Serial.print("Buffer: ");
    Serial.println(bufferSeq);

    // Se buffer começa e termina com '#' e tem mais que 1 char, tenta interpretar a instrução
    if (bufferSeq.length() > 1 && bufferSeq.charAt(0) == '#' && bufferSeq.charAt(bufferSeq.length() - 1) == '#') {
      interpretarInstrucao(bufferSeq);
      bufferSeq = "";  // limpa para próxima instrução
      Serial.println("--- Pronto para próxima instrução ---");
    }
  }

  delay(10);  // Evita sobrecarga do teclado
}

// Função que interpreta uma sequência delimitada por '#' e extrai opcode e operando.
// Exemplo: "#3##12#"  -> opcode = 3, operando = "12"
void interpretarInstrucao(const String& sequencia) {
  // --- Preparação de variáveis ---
  int tamanho = sequencia.length();  // comprimento da string recebida
  String tokenAtual = "";            // acumula caracteres entre '#'s
  String tokens[4];                  // armazena tokens não vazios (opcode, operando, ...)
  int contadorTokens = 0;            // quantos tokens válidos foram coletados

  // --- Varre cada caractere da sequência e separa por '#' ---
  for (int i = 0; i < tamanho; ++i) {
    char c = sequencia.charAt(i);

    if (c == '#') {
      // Encontramos um delimitador '#'
      if (tokenAtual.length() > 0) {
        // Se havia caracteres acumulados, salvamos como token
        if (contadorTokens < 4) {
          tokens[contadorTokens] = tokenAtual;
          contadorTokens++;
        }
        tokenAtual = "";  // zera para começar novo token
      } else {
        // tokenAtual vazio significa que havia "##" ou '#' no início/fim:
        // ignoramos (não criamos token vazio)
      }
    } else {
      // Caractere normal: acumula no token atual
      tokenAtual += c;
    }
  }

  // --- Se sobrou um token no final (sequência não terminou com '#') ---
  if (tokenAtual.length() > 0 && contadorTokens < 4) {
    tokens[contadorTokens] = tokenAtual;
    contadorTokens++;
  }

  // --- Se não houve tokens válidos, aborta ---
  if (contadorTokens == 0) {
    Serial.println("Instrução vazia detectada. Ignorando.");
    return;
  }

  // --- Interpreta o primeiro token como opcode (decimal) ---
  String textoOpcode = tokens[0];
  bool opcodeEhNumerico = isNumeric(textoOpcode);  // função utilitária (ver abaixo)

  if (!opcodeEhNumerico) {
    Serial.print("Opcode inválido (não numérico): ");
    Serial.println(textoOpcode);
    return;
  }

  int opcode = textoOpcode.toInt();  // converte para inteiro

  // Valida intervalo do opcode (0..15)
  if (opcode < 0 || opcode > 15) {
    Serial.print("Opcode fora do intervalo 0-15: ");
    Serial.println(opcode);
    return;
  }

  // --- Recupera mnemônico a partir do opcode (array MNEMONICOS definido globalmente) ---
  const char* mnemonico = MNEMONICOS[opcode];

  // --- Se houver segundo token, trata como operando ---
  String textoOperando = "";
  if (contadorTokens >= 2) {
    textoOperando = tokens[1];
  }

  // --- Impressão formatada para debug / verificação ---
  Serial.println("=== Instrução reconhecida ===");
  Serial.print("Opcode (decimal): ");
  Serial.println(opcode);
  Serial.print("Mnemônico: ");
  Serial.println(mnemonico);

  if (textoOperando.length() > 0) {
    Serial.print("Operando (texto): ");
    Serial.println(textoOperando);

    if (isNumeric(textoOperando)) {
      Serial.print("Operando (decimal): ");
      Serial.println(textoOperando.toInt());
    } else {
      Serial.println("Operando não é numérico (mantido como string).");
    }
  } else {
    Serial.println("Sem operando.");
  }

  // --- Aqui você pode chamar a rotina que executa a instrução ---
  // executeInstruction(opcode, textoOperando);
}

// Função utilitária: verifica se uma String representa um número inteiro (opcionalmente com sinal)
bool isNumeric(const String& s) {
  if (s.length() == 0) return false;
  for (unsigned int i = 0; i < s.length(); ++i) {
    char c = s.charAt(i);
    if (i == 0 && (c == '+' || c == '-')) {
      if (s.length() == 1) return false;  // apenas sinal não é número
      continue;
    }
    if (c < '0' || c > '9') return false;
  }
  return true;
}
