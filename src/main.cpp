#include <Arduino.h>

const int rouge = 25;
const int vert = 26;
const int bleu = 27;
const int bouton = 18;

int couleur = 0;

void setup() {
  pinMode(rouge, OUTPUT);
  pinMode(vert, OUTPUT);
  pinMode(bleu, OUTPUT);
  pinMode(bouton, INPUT_PULLUP);
}

void loop() {
  // Eteindre les trois couleurs
  digitalWrite(rouge, LOW);
  digitalWrite(vert, LOW);
  digitalWrite(bleu, LOW);

  // Allumer la couleur choisie
  if (couleur == 0) digitalWrite(rouge, HIGH);
  if (couleur == 1) digitalWrite(vert, HIGH);
  if (couleur == 2) digitalWrite(bleu, HIGH);

  // Changer de couleur avec le bouton
  if (digitalRead(bouton) == LOW) {
    couleur = (couleur + 1) % 3;
    delay(250);
    while (digitalRead(bouton) == LOW) {
      delay(10);
    }
  }
}