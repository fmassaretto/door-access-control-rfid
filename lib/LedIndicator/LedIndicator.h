#include <stdint.h>
#include <Arduino.h>

struct Indicator
{
    enum type {
      ENTER_NEW_CARD_CONDITION, 
      WAITING_CARD,
      MASTER_CARD_NOT_PERMITTED,
      CARD_ALREADY_EXISTS,
      CARD_ADDED_SUCCESS,
      SUCCESS_TAP, 
      FAILED_TAP
    };
};

class LedIndicator {
  private:
    uint8_t GREEN_LED;
    uint8_t RED_LED;
    void indicatorFactory(Indicator::type type);
    void enterAddCardLedIndicator();
    void waitingNewCardLedIndicator();
    void masterCardNotPermittedLedIndicator();
    void cardAlreadyExistsLedIndicator();
    void cardAddedSuccessLedIndicator();
    void cardIsAllowedLedIndicator();
    void cardIsNotAllowedLedIndicator();
  public:
    void indicate(Indicator::type type);
    Indicator indicatorType;

  LedIndicator(uint8_t greenLed, uint8_t redLed){
    GREEN_LED = greenLed;
    RED_LED = redLed;
  }
};

void LedIndicator::indicate(Indicator::type type) {
  indicatorFactory(type);
}

void LedIndicator::indicatorFactory(Indicator::type type) {
  switch (type) {
    case Indicator::ENTER_NEW_CARD_CONDITION:
      enterAddCardLedIndicator();
      break;
    case Indicator::WAITING_CARD:
      waitingNewCardLedIndicator();
      break;
    case Indicator::MASTER_CARD_NOT_PERMITTED:
      masterCardNotPermittedLedIndicator();
      break;
    case Indicator::CARD_ALREADY_EXISTS:
      cardAlreadyExistsLedIndicator();
      break;
    case Indicator::CARD_ADDED_SUCCESS:
      cardAddedSuccessLedIndicator();
      break;
    case Indicator::SUCCESS_TAP:
      cardIsAllowedLedIndicator();
      break;
    case Indicator::FAILED_TAP:
      cardIsNotAllowedLedIndicator();
      break;
  }
}

/*
* Table LED indication meaning:
* 1 - In the add new card program (only with the master card):
* * 1.1 - When enter the master card program to add new card will blink: Grenn Led (turn on for 400ms), Orange Led (turn on for 400ms), Red Led (turn on for 400ms)
* * 1.2 - Waiting a new card to tap on RFID sensor will blink: Green Led (turn on and off for 400ms) until tap a card
* * 1.3 - If the "new card" tapped is the master card will blink: Orange Led (turn on and off for 400ms) 4 times
* * 1.4 - If the new card already exists will turn on: Red Led (turn on for 1000ms) 1 time
* * 1.5 - If the new card was insert with success will: Green Led (turn on for 900ms) and blink 1 time for 100ms 
*
* 2 - Normal Operational Program
* * 2.1 - If the card is allowed will: Green Led (turn on for 1000ms) 1 time
* * 2.2 - If the card is NOT allowed will: Red Led (turn on for 1000ms) 1 time
*/

// Master Card: add new card
void LedIndicator::enterAddCardLedIndicator() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  delay(400);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(400);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  delay(400);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void LedIndicator::waitingNewCardLedIndicator() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  delay(400);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  delay(400);
}

void LedIndicator::masterCardNotPermittedLedIndicator() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(400);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  delay(400);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(400);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void LedIndicator::cardAlreadyExistsLedIndicator() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  delay(1000);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void LedIndicator::cardAddedSuccessLedIndicator() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  delay(900);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  delay(100);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  delay(100);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}
// Master Card: add new card

// Normal Operation
void LedIndicator::cardIsAllowedLedIndicator() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  delay(1000);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void LedIndicator::cardIsNotAllowedLedIndicator() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  delay(1000);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}
// Normal Operation