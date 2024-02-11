/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example sketch/program showing how to read data from a PICC to serial.
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 *
 * Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
 * Reader on the Arduino SPI interface.
 *
 * When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
 * then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
 * you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
 * will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
 * when removing the PICC from reading distance too early.
 *
 * If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
 * So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
 * details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
 * keep the PICCs at reading distance until complete.
 *
 * @license Released into the public domain.
 *
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include "./EnvVariables.h"

#define RST_PIN D3  // Configurable, see typical pin layout above
#define SS_PIN D8   // Configurable, see typical pin layout above
#define LED_RED D4
#define LED_GREEN D1
#define RELAY_DOOR_PIN D2

MFRC522 mfrc522(SS_PIN, RST_PIN);
Preferences preferences; // To save cards to FLASH memory

uint16_t cardsCount = 0;
bool debug = true;

void setup() {
  if(debug) Serial.begin(9600);
  preferences.begin(WORKSPACE_NAME, false);
  // preferences.clear(); // To clear all data in memory, uncomment if want to reset the data
  cardsCount = preferences.getUInt("cardsCount", 0);

  saveMasterCardToMemory();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(RELAY_DOOR_PIN, OUTPUT);

  keepDoorClosed();

  rfidInit();
  preferences.end();
}

void saveMasterCardToMemory(){
  if(preferences.getString(MASTER_CARD_ID).isEmpty()){
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
  if (!isNewCardPresent()) {
    return;
  }

  preferences.begin(WORKSPACE_NAME, false);
  String cardId = cardIdRead();

  if(isMasterCard(cardId)) {
    int status = addNewCard(cardId);

    if(debug) Serial.println();
    if(debug) displayAddNewCardMessage(status);
  } else {
    if(debug) Serial.print("Debugger => loop() - Cards id: ");
    if(debug) Serial.println(cardId);

    if (isCardAllowed(cardId)) {
      cardIsAllowedLedIndicator();
      openTheDoor();
    } else {
      cardIsNotAllowedLedIndicator();
    }
  }

  keepDoorClosed();

  delay(1000);

  if(debug) Serial.print("Cards in memory: ");
  if(debug) Serial.println(preferences.getUInt("cardsCount"));
  if(debug) Serial.println();

  if(debug) Serial.println("RFID is ready to read a card!");
  if(debug) Serial.flush();
  preferences.end();
}

void displayAddNewCardMessage(int status) {
  if(status == -1) {
      if(debug) Serial.println("Failed: Card already exists!");
      cardAlreadyExistsLedIndicator();
    } else {
      if(debug) Serial.println("Success: Card added!");
      cardAddedSuccessLedIndicator();
    }
}

bool isNewCardPresent() {
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
  String cardIdFromMemory = preferences.getString(cardId.c_str());
  bool cardIdExists = preferences.isKey(cardId.c_str());

  return cardIdExists;
}

bool isMasterCard(String cardId) {
  return cardId.equalsIgnoreCase(MASTER_CARD_ID);
}

int addNewCard(String masterCardId) {
  if(debug) Serial.println("Scan the new card that you want to add...");
  enterAddCardLedIndicator();

  while(!isNewCardPresent()) {
    waitingNewCardLedIndicator();    
  }

  String cardID = cardIdRead();

  if(isMasterCard(cardID)) {
    masterCardNotPermittedLedIndicator();
    addNewCard(cardID);
  }

  bool cardIdExists = preferences.isKey(cardID.c_str());

  if(cardIdExists) {
    return -1;
  }

  // add card only if the card do not already exists
  int cardCount = preferences.getUInt("cardsCount");
  int newCardCount = cardCount + 1;
  String cardName = "Card " + String(newCardCount);

  preferences.putString(cardID.c_str(), cardName.c_str());
  preferences.putUInt("cardsCount", newCardCount);
  return 0;
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
void enterAddCardLedIndicator() {
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  delay(400);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, HIGH);
  delay(400);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  delay(400);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
}

void waitingNewCardLedIndicator() {
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  delay(400);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  delay(400);
}

void masterCardNotPermittedLedIndicator() {
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, HIGH);
  delay(400);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  delay(400);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, HIGH);
  delay(400);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
}

void cardAlreadyExistsLedIndicator() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
}

void cardAddedSuccessLedIndicator() {
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  delay(900);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  delay(100);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  delay(100);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
}
// Master Card: add new card

// Normal Operation
void cardIsAllowedLedIndicator() {
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
}

void cardIsNotAllowedLedIndicator() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
}
// Normal Operation

void openTheDoor() {
  digitalWrite(RELAY_DOOR_PIN, HIGH);
  delay(950);
}

void keepDoorClosed() {
  digitalWrite(RELAY_DOOR_PIN, LOW);
}