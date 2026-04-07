/*
=====================
PARTE 1) CONFIGURANDO O TECLADO
=====================
*/

#include <Keypad.h>

const byte NUM_LINHAS 4
const byte NUM_COLUNAS 4 

char mapaTeclas[NUM_LINHAS][NUM_COLUNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pinosLinhas[NUM_LINHAS] = {0, 0, 0, 0};
byte pinosColunas[NUM_COLUNAS] = {0, 0, 0, 0};

Keypad teclado = Keypad(makeKeymap(mapaTeclas), pinosLinhas, pinosColunas, NUM_LINHAS, NUM_COLUNAS);


void setup() {
  Serial.begin(9600);

  Serial.println("Teclado pronto para receber input das teclas...");
}

void loop() {
  char teclaPressionada = teclado.getKey();

  if (teclaPressionada) {
    Serial.print("Tecla pressionada: ");
    Serial.println(teclaPressionada);
  }
}
