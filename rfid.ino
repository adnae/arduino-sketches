#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>

//#define RST_PIN 9 // Configurable, see typical pin layout above
//#define SS_PIN 10 // Configurable, see typical pin layout above
const int RST_PIN = 16; // Reset pin
const int SS_PIN = 17; // Slave select pin

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("                      ");
  lcd.setCursor(0, 1);
  lcd.print("                      ");
  lcd.setCursor(0, 2);
  lcd.print("Dataterminal #1002392");
  lcd.setCursor(0, 3);
  lcd.print(" Starbase Computer! ");
}

void loop() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    lcd.setCursor(0, 0);
    lcd.print("Scanned ID ....");
    lcd.setCursor(0, 1);
    lcd.print( mfrc522.uid.uidByte[0] );
    lcd.print( mfrc522.uid.uidByte[1] );
    lcd.print( mfrc522.uid.uidByte[2] );
    lcd.print( mfrc522.uid.uidByte[3] );
    lcd.print( " found    " );
    lcd.setCursor(0, 2);
    lcd.print("Dataterminal #1002392");
    lcd.setCursor(0, 3);
    lcd.print(" Starbase Computer! ");

    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
}
