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

    // Se buffer começa e termina com '#' e tem mais que 1 char, tenta parsear
    if (bufferSeq.length() > 1 && bufferSeq.charAt(0) == '#' && bufferSeq.charAt(bufferSeq.length() - 1) == '#') {
      interpretarInstrucao(bufferSeq);
      bufferSeq = "";  // limpa para próxima instrução
      Serial.println("--- Pronto para próxima instrução ---");
    }
  }

  delay(10);  // Evita sobrecarga do teclado
}

// Função que faz o parsing da string completa (ex: "#3##12#")
void interpretarInstrucao(const String& seq) {
  // Divide por '#' e coleta tokens não vazios
  // Ex: "#3##12#" -> ["", "3", "", "12", ""] -> tokens: "3", "12"
  int comprimento = seq.length();
  String token = "";
  String tokens[4];  // esperamos no máximo opcode + 1 operando; mas deixei 4 para segurança
  int tcount = 0;

  for (int i = 0; i < comprimento; ++i) {
    char c = seq.charAt(i);
    if (c == '#') {
      if (token.length() > 0) {
        if (tcount < 4) tokens[tcount++] = token;
        token = "";
      } else {
        // token vazio entre hashes -> ignora
      }
    } else {
      token += c;
    }
  }
  // Caso haja token residual (não esperado se termina com '#')
  if (token.length() > 0 && tcount < 4) {
    tokens[tcount++] = token;
  }

  if (tcount == 0) {
    Serial.println("Instrução vazia detectada. Ignorando.");
    return;
  }

  // Primeiro token -> opcode (decimal)
  String opcodeStr = tokens[0];
  int opcode = opcodeStr.toInt();  // toInt retorna 0 se não-numérico; validaremos
  bool opcodeIsNumber = isNumeric(opcodeStr);

  if (!opcodeIsNumber) {
    Serial.print("Opcode inválido (não numérico): ");
    Serial.println(opcodeStr);
    return;
  }

  if (opcode < 0 || opcode > 15) {
    Serial.print("Opcode fora do intervalo 0-15: ");
    Serial.println(opcode);
    return;
  }

  const char* mnem = MNEMONICOS[opcode];

  // Operando (se houver)
  String operandoStr = "";
  if (tcount >= 2) {
    operandoStr = tokens[1];
  }

  // Saída formatada
  Serial.println("=== Instrução reconhecida ===");
  Serial.print("Opcode (decimal): ");
  Serial.println(opcode);
  Serial.print("Mnemônico: ");
  Serial.println(mnem);

  if (operandoStr.length() > 0) {
    Serial.print("Operando (texto): ");
    Serial.println(operandoStr);
    if (isNumeric(operandoStr)) {
      Serial.print("Operando (decimal): ");
      Serial.println(operandoStr.toInt());
    } else {
      Serial.println("Operando não é numérico (mantido como string).");
    }
  } else {
    Serial.println("Sem operando.");
  }

  // Aqui você pode chamar a função que executa a instrução, por exemplo:
  // executeInstruction(opcode, operandoStr);
}

// Função utilitária: verifica se string é composta só por dígitos (0-9) e opcionalmente sinal
bool isNumeric(const String& s) {
  if (s.length() == 0) return false;
  for (unsigned int i = 0; i < s.length(); ++i) {
    char c = s.charAt(i);
    if (i == 0 && (c == '+' || c == '-')) {
      if (s.length() == 1) return false;
      continue;
    }
    if (c < '0' || c > '9') return false;
  }
  return true;
}