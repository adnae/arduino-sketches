#include <ArduinoOTA.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>


const char* WLAN_SSID     = "DeviousCity";
const char* WLAN_PASSWORD = "CollaredDream!!1";
const char* WLAN_HOSTNAME = "testbombe";

const char* OTA_PASSWORD  = "ticktick";

String LCD_Line1;
String LCD_Line2;
String LCD_Line3;
String LCD_Line4;

char KEYPAD_Layout[4][4] = { {'1', '2', '3', 'A'}, {'4', '5', '6', 'B'}, {'7', '8', '9', 'C'}, {'*', '0', '#', 'D'} };
byte KEYPAD_row[4] = {0, 1, 2, 3};
byte KEYPAD_col[4] = {4, 5, 6, 7};
char KEYPAD_getChar;

const uint8_t SEGMENT_data[] = {0xff, 0xff, 0xff, 0xff};
const uint8_t SEGMENT_blank[] = {0x00, 0x00, 0x00, 0x00};
const uint8_t SEGMENT_done[] = { SEG_B | SEG_C | SEG_D | SEG_E | SEG_G, SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, SEG_C | SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F | SEG_G };

const unsigned int PIN_RFID_RST     = 27;
const unsigned int PIN_SPI_SCK      = 14;
const unsigned int PIN_SPI_MISO     = 12;
const unsigned int PIN_SPI_MOSI     = 13;
const unsigned int PIN_SPI_SS       = 15;
const unsigned int PIN_SEGMENT1_CLK = 16;
const unsigned int PIN_SEGMENT1_DIO = 17;
const unsigned int PIN_SEGMENT2_CLK = 18;
const unsigned int PIN_SEGMENT2_DIO = 19;
const unsigned int PIN_STARTBUTTON  = 25;

const unsigned int CONFIG_delay       = 1000;
bool               CONFIG_startStatus = false;
bool               CONFIG_setupStatus = false;
char               BOMB_disarmCode[11];
unsigned long      BOMB_countdown;
unsigned int       BOMB_hours;
unsigned int       BOMB_minutes;
unsigned int       BOMB_seconds;
volatile int       interruptCounter;
int                totalInterruptCounter;
 
hw_timer_t * timer                  = NULL;
portMUX_TYPE timerMux               = portMUX_INITIALIZER_UNLOCKED;

LiquidCrystal_I2C lcd(0x27, 20, 4);
Keypad_I2C pad( makeKeymap(KEYPAD_Layout), KEYPAD_row, KEYPAD_col, 4, 4, 0x20);
MFRC522 mfrc522(PIN_SPI_SS, PIN_RFID_RST);
MFRC522::MIFARE_Key key;
TM1637Display seg01 = TM1637Display(PIN_SEGMENT1_CLK, PIN_SEGMENT1_DIO);
TM1637Display seg02 = TM1637Display(PIN_SEGMENT2_CLK, PIN_SEGMENT2_DIO);

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
 
}

void OTA()  {
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setHostname(WLAN_HOSTNAME);

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  lcdprint("OTA ............. OK");
  delay( CONFIG_delay );
}

void WLAN() {
  WiFi.mode(WIFI_STA);

  WiFi.begin(WLAN_SSID, WLAN_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcdprint("WLAN ............ OK");
  delay( CONFIG_delay );
}

void lcdprint(String lcdText) {
  lcd.clear();
  LCD_Line1 = LCD_Line2;
  LCD_Line2 = LCD_Line3;
  LCD_Line3 = LCD_Line4;

  lcd.setCursor(0, 0);
  lcd.print(LCD_Line1);
  lcd.setCursor(0, 1);
  lcd.print(LCD_Line2);
  lcd.setCursor(0, 2);
  lcd.print(LCD_Line3);

  LCD_Line4 = lcdText;
  LCD_Line4.replace("\r\n", "");
  lcd.setCursor(0, 3);
  lcd.print(LCD_Line4);
}

void lcdeffect_line1(String line)  {
  unsigned int startPoint = 0;   //set starting point
  unsigned int endPoint = 20;    //set ending point
  lcd.clear(); 

  for (unsigned int i = line.length() - 1; i >= 0; i--)  {
    startPoint = 0;

    for (unsigned int j = 0; j < endPoint; j++)  {
      lcd.setCursor(startPoint, 0);
      lcd.print(line[i]);

      delay(50);

      if (startPoint != endPoint - 1) {
        lcd.setCursor(startPoint, 0);
        lcd.print(' ');
      }
      startPoint++;
    }
    endPoint--;

    delay(50);
  }   
}

void welcome()  {
  lcd.setCursor(0, 0);
  lcd.print("Nuclear Fusion Bomb!");
  lcd.setCursor(0, 1);
  lcd.print("   Serial Number:   ");
  lcd.setCursor(0, 2);
  lcd.print("IGM-002-NFB-002-DFDM");
  lcd.setCursor(0, 3);
  lcd.print("Cerberus Weapon inc.");

  while ( !CONFIG_startStatus )  {
    ArduinoOTA.handle();
    seg01.setSegments(SEGMENT_data);
    seg02.setSegments(SEGMENT_data);
    delay( 500 );
    seg01.setSegments(SEGMENT_blank);
    seg02.setSegments(SEGMENT_blank);
    delay( 500 );

    int Push_button_state = digitalRead( 25 );
    if ( Push_button_state == HIGH )  { 
      Serial.println("HIGH");
    } else  {
      Serial.println("LOW");
    }


    if ( digitalRead(PIN_STARTBUTTON) == LOW ) {
      CONFIG_startStatus = true;
      CONFIG_setupStatus = true;
    }
  }
}

void sevenseg_init()  {
  seg01.clear();
  seg01.setBrightness(7);
  seg02.clear();
  seg02.setBrightness(7);  
  seg01.showNumberDec(1, false, 1, 0);
  delay( 500 );
  seg01.showNumberDec(12, false, 2, 0);
  delay( 500 );
  seg01.showNumberDec(123, false, 3, 0);
  delay( 500 );
  seg01.showNumberDec(1234, false, 4, 0);
  delay( 500 );
  seg02.showNumberDec(5, false, 1, 0);
  delay( 500 );
  seg02.showNumberDec(56, false, 2, 0);
  delay( 500 );
  seg02.showNumberDec(567, false, 3, 0);
  delay( 500 );
  seg02.showNumberDec(5678, false, 4, 0);
  delay( 500 );
  seg01.setSegments(SEGMENT_blank);
  seg02.setSegments(SEGMENT_blank);
  delay( 500 );
  seg01.setSegments(SEGMENT_data);
  seg02.setSegments(SEGMENT_data);
  delay( 500 );
  seg01.setSegments(SEGMENT_blank);
  seg02.setSegments(SEGMENT_blank);
  lcdprint("7 SEG DISPLAYS .. OK");
  delay( CONFIG_delay );  
}

void pad_init() {
  pad.begin( );
  lcdprint("Pad ............. OK");
  delay( CONFIG_delay );
}

void rfid_init()  {
  mfrc522.PCD_Init();
  delay( 10 );
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  lcdprint("RFID ............ OK");
  delay( CONFIG_delay );
}

void spi_init() {
  SPI.begin( PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SS);
  lcdprint("SPI ............. OK");
  delay( CONFIG_delay );
}

void wire_init()  {
  Wire.begin();
  Wire.setClock(400000);
  lcdprint("Wire ............ OK");
  delay( CONFIG_delay );
}

void serial_init()  {
  Serial.begin(115200);
  lcdprint("Serial Console .. OK");
  delay( CONFIG_delay );
}

void rfid_disarming() {
  if ( mfrc522.PICC_IsNewCardPresent() )  {
    mfrc522.PICC_ReadCardSerial();
    Serial.print("Die ID des RFID-TAGS lautet:");
    for (byte i = 0; i < mfrc522.uid.size; i++)  {
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void setBomb()  {
  lcd.setCursor(0, 0);
  lcd.print("Set the disarm Code:");
  lcd.setCursor(0, 1);
  lcd.print("[          ]        ");
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("D = ENTER           ");

  lcd.setCursor(1, 1);

  for ( int counter = 0; counter < 10; ) {
    KEYPAD_getChar = pad.getKey();
    if ( KEYPAD_getChar != NO_KEY ) {
      if ( KEYPAD_getChar == 'D' && counter > 3 ) break;

      if ( KEYPAD_getChar != 'A' && KEYPAD_getChar != '*' && KEYPAD_getChar != '#' && KEYPAD_getChar != 'B' && KEYPAD_getChar != 'C' && KEYPAD_getChar != 'D' ) {
        BOMB_disarmCode[counter] = KEYPAD_getChar;
        counter++;
        BOMB_disarmCode[counter] = '\0';
        lcd.print(KEYPAD_getChar);
      }
    }
  }

  lcd.setCursor(0, 2);
  lcd.print("Enter Countdown:   ");
  lcd.setCursor(0, 3);
  lcd.print("[    ] Minutes     ");
  lcd.setCursor(1, 3);

  char countd[5];
  for ( int counter = 0; counter < 5; ) {
    KEYPAD_getChar = pad.getKey();
    if ( KEYPAD_getChar != NO_KEY ) {
      if ( KEYPAD_getChar == 'D' && counter > 0 ) break;

      if ( KEYPAD_getChar != 'A' && KEYPAD_getChar != '*' && KEYPAD_getChar != '#' && KEYPAD_getChar != 'B' && KEYPAD_getChar != 'C' && KEYPAD_getChar != 'D' ) {
        countd[counter] = KEYPAD_getChar;
        counter++;
        countd[counter] = '\0';
        lcd.print(KEYPAD_getChar);
      }
    }
  }
  BOMB_countdown = atoi( countd );
  BOMB_hours = int( BOMB_countdown / 60 );
  BOMB_minutes = ( BOMB_countdown - ( BOMB_hours * 60 ) );
  BOMB_countdown = BOMB_countdown * 60;
  if ( BOMB_hours != 0 ) {
    seg01.showNumberDecEx(BOMB_hours, 0b10000000, false, 4, 0);
  }
  seg02.showNumberDecEx(BOMB_minutes, 0b11100000, true, 2, 0);
  seg02.showNumberDecEx(0, 0b11100000, true, 2, 2);
  CONFIG_setupStatus = false;

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);  


  lcd.setCursor(0, 0);
  lcd.print("   for disarming    ");
  lcd.setCursor(0, 1);
  lcd.print(" PLEASE INSERT CARD ");
  lcd.setCursor(0, 2);
  lcd.print("   for disarming    ");
  lcd.setCursor(0, 3);
  lcd.print("Cerberus Weapon inc.");
  lcd.setCursor(0, 0);
}

void setup() {
  lcd.init();
  lcd.backlight();
  lcdprint("Booting Kernel 1.1.6");
  delay( CONFIG_delay );

  serial_init();
  wire_init();
  spi_init();
  WLAN();
  OTA();
  pad_init();
  rfid_init();
  sevenseg_init();

  pinMode(PIN_STARTBUTTON, INPUT);

}

void loop() {
  if ( CONFIG_startStatus == false ) {
    welcome();
  }
  if ( CONFIG_setupStatus == true ) {
    setBomb();
  }

  if (interruptCounter > 0) {
 
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
 
    totalInterruptCounter++;
 
    BOMB_countdown--;
    BOMB_hours = int(BOMB_countdown / 60 / 60);
    BOMB_minutes = int( ( BOMB_countdown / 60) - ( BOMB_hours * 60 ));
    BOMB_seconds = int( BOMB_countdown - ( BOMB_hours * 60 * 60) - ( BOMB_minutes * 60 ));
    if ( BOMB_hours == 0 && BOMB_minutes == 0 && BOMB_seconds == 0 )  {
      //booom();
    }
    if ( BOMB_hours != 0 ) {
      seg01.showNumberDecEx(BOMB_hours, 0b10000000, false, 4, 0);
    } else  {
      seg01.setSegments(SEGMENT_blank);
    }
    seg02.showNumberDecEx(BOMB_minutes, 0b11100000, true, 2, 0);
    seg02.showNumberDecEx(BOMB_seconds, 0b11100000, true, 2, 2);

//    state = !state;

   }
  
  rfid_disarming();
  ArduinoOTA.handle();
}
