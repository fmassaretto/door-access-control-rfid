#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include "./EnvVariables.h"
#include "./lib/LedIndicator/LedIndicator.h"

#include "./lib/PinoutsBoards/arduino_nano_pinouts.h"  // You can choose pinouts definition for your board

MFRC522 mfrc522(SS_PIN, RST_PIN);
Preferences preferences; // To save cards to FLASH memory
LedIndicator ledIndicator(LED_GREEN, LED_RED);

uint16_t cardsCount = 0;
bool debug = true;
bool clearAllCardsInMemory = false;

void setup() {
  if(debug) Serial.begin(9600);

  preferences.begin(WORKSPACE_NAME, false);

  if(clearAllCardsInMemory) preferences.clear(); // To clear all data in memory

  cardsCount = preferences.getUInt("cardsCount", 0);

  saveMasterCardToMemory();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(RELAY_DOOR_PIN, OUTPUT);

  keepDoorClosed();

  rfidInit();
  preferences.end();
}

void saveMasterCardToMemory() {
  if(!preferences.isKey(MASTER_CARD_ID)){
    preferences.putString(MASTER_CARD_ID, "Master Card");
    preferences.putUInt("cardsCount", ++cardsCount);
  }
}

void rfidInit() {
  SPI.begin();                        // Init SPI bus
  mfrc522.PCD_Init();                 // Init MFRC522
  delay(4);                           // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  if(debug) Serial.println("Scan PICC to see UID, SAK, type, and data blocks...");
}

void loop() {
  if (!isCardPresent()) {
    return;
  }

  preferences.begin(WORKSPACE_NAME, false);
  String cardId = cardIdRead();

  if(isMasterCard(cardId)) {
    int status = processCard();

    addNewCardIndicator(status);
  } else {
    if(debug) Serial.print("Debugger => loop() - Cards id: ");
    if(debug) Serial.println(cardId);

    if (isCardAllowed(cardId)) {
      ledIndicator.indicate(ledIndicator.indicatorType.SUCCESS_TAP);
      openTheDoor();
    } else {
      ledIndicator.indicate(ledIndicator.indicatorType.FAILED_TAP);
    }
  }

  keepDoorClosed();

  delay(1000);

  if(debug) showCardsInMemory();

  mfrc522.PICC_HaltA(); 
  preferences.end();

  if(debug) Serial.println("RFID sensor is ready to read a card!");
  if(debug) Serial.flush();
}

void addNewCardIndicator(int status) {
  if(debug) Serial.println();

  if(status == 200) {
    if(debug) Serial.println("Success: Card added!");
    ledIndicator.indicate(ledIndicator.indicatorType.CARD_ADDED_SUCCESS);
  } else if(status == 202){
    if(debug) Serial.println("Success: Card removed!");
    ledIndicator.indicate(ledIndicator.indicatorType.CARD_REMOVED);
  } else if(status == 204) {
    if(debug) Serial.println("Failed: Card does not exist!");
    ledIndicator.indicate(ledIndicator.indicatorType.CARD_DOES_NOT_EXIST);
  } else if(status == 409) {
    if(debug) Serial.println("Failed: Card already exists!");
    ledIndicator.indicate(ledIndicator.indicatorType.CARD_ALREADY_EXISTS);
  } else if(status == 400) {
    if(debug) Serial.println("Failed: Master card tapped!");
    ledIndicator.indicate(ledIndicator.indicatorType.MASTER_CARD_NOT_PERMITTED);
  } else {
    if(debug) Serial.println("Error: Cannot have status code: " + status);
  }
}

void showCardsInMemory() {
  Serial.println("-------------------------------");
  Serial.print("Cards in memory (Including Master Card): ");
  Serial.println(preferences.getUInt("cardsCount"));
  Serial.println("-------------------------------");
}

bool isCardPresent() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  return true;
}

String cardIdRead() {
  String cardId;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardId += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardId.trim();
  
  return cardId;
}

bool isCardAllowed(String cardId) {
  return preferences.isKey(cardId.c_str());
}

bool isMasterCard(String cardId) {
  return cardId.equals(MASTER_CARD_ID);
}

bool isCardAlreadyExists(String cardId) {
  return preferences.isKey(cardId.c_str());
}

int processCard() {
  if(debug) Serial.println("Scan the new card that you want to add...");
  
  ledIndicator.indicate(ledIndicator.indicatorType.ENTER_NEW_CARD_CONDITION);

  waitForCard();

  String cardId = cardIdRead();

  if(isMasterCard(cardId)) {
    // master card tapped again, so can remove a card from database
    return removeCard();
  }

  if(isCardAlreadyExists(cardId.c_str())) {
    return 409;
  }

  // add card only if the card do not already exists
  return addNewCard(cardId);
}

int removeCard() {
  int cardCount = preferences.getUInt("cardsCount");

  if(debug) Serial.println("Scan the card that you want to remove...");

  waitForCard();

  String cardId = cardIdRead();

  if(debug) Serial.println("Trying to remove card ID: " + cardId);

  if(isCardAlreadyExists(cardId.c_str())) {
    int newCardCount = cardCount - 1;

    preferences.remove(cardId.c_str());
    preferences.putUInt("cardsCount", newCardCount);
    
    return 202;
  }

  if(debug) Serial.println("Card ID: " + cardId + " does not exists!");

  return 400;
}

void waitForCard() {
  while(!isCardPresent()) {
    ledIndicator.indicate(ledIndicator.indicatorType.WAITING_CARD);
  }
}

int addNewCard(String cardId) {
  int cardCount = preferences.getUInt("cardsCount");
  int newCardCount = cardCount + 1;

  String cardName = "Card " + String(newCardCount);

  preferences.putString(cardId.c_str(), cardName.c_str());
  preferences.putUInt("cardsCount", newCardCount);

  return 200;
}

void openTheDoor() {
  digitalWrite(RELAY_DOOR_PIN, HIGH);
  // Using this electric lock: intelbras fx 2000 and the guide
  // recommends to keep on for around 1 second (1000ms)
  delay(1100); 
}

void keepDoorClosed() {
  digitalWrite(RELAY_DOOR_PIN, LOW);
}
