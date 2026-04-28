// INTEGRANTES DO GRUPO:
// 202502831285 Marcos Paulo Lopes de Assunção TA
// 202403191156 Caio Domingues TA
// 202307164607 Ricardo Santos Lima TA
// 202403088886 Bernardo Meierles TA
// 202501001041 Felipe Maia TA


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
#define TRIG 6
#define ECHO 7

#define LED1 4
#define LED2 3
#define LED3 2

#define BUZZER 5

// display (a-g)
int displayPins[7] = { 22, 23, 24, 25, 26, 27, 28 };

// ============================
// ARQUITETURA
// ============================
int MEM[16];
int PC = 0;
byte IR = 0;
int ACC = 0;
bool FLAG_Z = false;
bool EXECUTANDO = false;

// ============================
// PROGRAMA
// ============================
struct InstrucaoInterpretada {
  bool valida;
  int opcode;
  String operando;
};

InstrucaoInterpretada programa[16];
int tamanhoPrograma = 0;

// ============================
bool modoLOAD = false;
String bufferEntrada = "";

// ============================
const char* MNEMONICOS[16] = {
  "NOP", "READ", "LOADK", "ADDK", "SUBK", "CMPK",
  "LEDON", "LEDOFF", "BUZON", "BUZOFF",
  "DISP", "ALERT", "BINC", "STORE", "LOADM", "HALT"
};

// ============================
// DISPLAY (0-9, E, -)
// ============================
byte numeros[12][7] = {
  { 1, 1, 1, 1, 1, 1, 0 },  //0
  { 0, 1, 1, 0, 0, 0, 0 },  //1
  { 1, 1, 0, 1, 1, 0, 1 },  //2
  { 1, 1, 1, 1, 0, 0, 1 },  //3
  { 0, 1, 1, 0, 0, 1, 1 },  //4
  { 1, 0, 1, 1, 0, 1, 1 },  //5
  { 1, 0, 1, 1, 1, 1, 1 },  //6
  { 1, 1, 1, 0, 0, 0, 0 },  //7
  { 1, 1, 1, 1, 1, 1, 1 },  //8
  { 1, 1, 1, 1, 0, 1, 1 },  //9
  { 1, 0, 0, 1, 1, 1, 1 },  //E
  { 0, 0, 0, 0, 0, 0, 1 }   // -
};

// ============================
// SETUP
// ============================
void setup() {
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  for (int i = 0; i < 7; i++) pinMode(displayPins[i], OUTPUT);

  Serial.println("Sistema pronto");
}

// ============================
// LOOP
// ============================
void loop() {
  char tecla = teclado.getKey();

  if (tecla) {

    // ============================
    // CONTROLE DO MODO LOAD (#)
    // ============================
    if (tecla == '#') {
      modoLOAD = !modoLOAD;

      if (modoLOAD) {
        Serial.println("LOAD ON");
        tamanhoPrograma = 0;
      } else {
        Serial.println("LOAD OFF");
      }

      bufferEntrada = "";
      return;
    }

    // ============================
    // COMANDO RUN (A)
    // ============================
    if (tecla == 'A') {
      PC = 0;
      EXECUTANDO = true;
      Serial.println("RUN");
      return;
    }

    // ============================
    // EXECUÇÃO PASSO A PASSO (*)
    // ============================
    if (tecla == '*' && EXECUTANDO) {
      cicloInstrucao();
      return;
    }

    // ============================
    // MODO LOAD (entrada de instruções)
    // ============================
    if (modoLOAD) {

      // D = ENTER (confirma instrução)
      if (tecla == 'D') {

        InstrucaoInterpretada inst = interpretarInstrucao(bufferEntrada);

        if (inst.valida && tamanhoPrograma < 16) {
          programa[tamanhoPrograma++] = inst;
          Serial.println(MNEMONICOS[inst.opcode]);
        } else {
          Serial.println("ERRO");
        }

        bufferEntrada = "";
      } else {
        // Monta a instrução
        bufferEntrada += tecla;
      }
    }
  }
}

// ============================
// CICLO (UC)
// ============================
void cicloInstrucao() {

  if (PC >= tamanhoPrograma) {
    EXECUTANDO = false;
    return;
  }

  InstrucaoInterpretada inst = programa[PC];
  IR = inst.opcode;

  executarInstrucao(inst.opcode, inst.operando);

  mostrarEstado();

  if (IR == 15) {
    EXECUTANDO = false;
    Serial.println("HALT");
    return;
  }

  PC++;
}

// ============================
// EXECUÇÃO (ULA + CONTROLE)
// ============================
void executarInstrucao(int opcode, const String& op) {

  switch (opcode) {

    case 1: ACC = lerSensor(); break;

    case 2:
      if (ehNumerico(op)) ACC = op.toInt();
      break;

    case 3:
      if (ehNumerico(op)) ACC += op.toInt();
      break;

    case 4:
      if (ehNumerico(op)) ACC -= op.toInt();
      break;

    case 5:
      if (ehNumerico(op)) FLAG_Z = (ACC == op.toInt());
      break;

    case 6: digitalWrite(LED1, HIGH); break;

    case 7: digitalWrite(LED1, LOW); break;

    case 8: digitalWrite(BUZZER, HIGH); break;

    case 9: digitalWrite(BUZZER, LOW); break;

    case 10: mostrarDisplay(ACC); break;

    case 11: executarAlerta(); break;

    case 12: Serial.println(IR, BIN); break;

    case 13:
      if (ehNumerico(op)) MEM[op.toInt()] = ACC;
      break;

    case 14:
      if (ehNumerico(op)) ACC = MEM[op.toInt()];
      break;

    case 15:
      digitalWrite(LED1, LOW);
      digitalWrite(BUZZER, LOW);
      break;
  }
}

// ============================
// SENSOR
// ============================
int lerSensor() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long dur = pulseIn(ECHO, HIGH);
  int dist = dur * 0.034 / 2;

  Serial.print("Distancia: ");
  Serial.println(dist);

  return dist;
}

// ============================
// ALERT
// ============================
void executarAlerta() {
  if (ACC < 10) {
    digitalWrite(LED1, HIGH);
    digitalWrite(BUZZER, HIGH);
  } else if (ACC < 20) {
    digitalWrite(LED1, HIGH);
    digitalWrite(BUZZER, LOW);
  } else {
    digitalWrite(LED1, LOW);
    digitalWrite(BUZZER, LOW);
  }
}

// ============================
// DISPLAY + ERROS
// ============================
void mostrarDisplay(int valor) {

  if (valor > 9) {
    Serial.println("OVERFLOW");
    desenhar(10);
    return;
  }

  if (valor < 0) {
    Serial.println("NEGATIVO");
    desenhar(11);
    return;
  }

  desenhar(valor);
}

void desenhar(int n) {
  for (int i = 0; i < 7; i++)
    digitalWrite(displayPins[i], numeros[n][i]);
}

// ============================
// ESTADO
// ============================
void mostrarEstado() {
  Serial.print("PC: ");
  Serial.print(PC);
  Serial.print(" | IR: ");
  Serial.print(MNEMONICOS[IR]);
  Serial.print(" | ACC: ");
  Serial.print(ACC);
  Serial.print(" | FLAG_Z: ");
  Serial.println(FLAG_Z);
}

// ============================
// INTERPRETADOR
// ============================
InstrucaoInterpretada interpretarInstrucao(const String& seq) {

  InstrucaoInterpretada r;
  r.valida = false;

  String s = seq;

  s.replace("#", " ");
  s.replace("B", " ");

  s.trim();

  int sp = s.indexOf(' ');
  String op = (sp == -1) ? s : s.substring(0, sp);
  String arg = (sp == -1) ? "" : s.substring(sp + 1);

  if (!ehNumerico(op)) return r;

  int opcode = op.toInt();
  if (opcode < 0 || opcode > 15) return r;

  r.valida = true;
  r.opcode = opcode;
  r.operando = arg;

  return r;
}

bool ehNumerico(const String& s) {
  if (s.length() == 0) return false;
  for (int i = 0; i < s.length(); i++)
    if (!isDigit(s[i])) return false;
  return true;
}
