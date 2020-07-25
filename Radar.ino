#include <Stepper.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define NEOPIN 12
#define ECHO 6
#define TRIGGER 7
#define MAX_LED 60
#define RANGE_LED 27
#define FADER 255
#define MAX_BR 55

bool clockwise = true;
int stepping = 0;
int led = 0;
long dauer = 0;
long entfernung = 0;
int led_matrix[RANGE_LED + 1][4];
uint32_t color;

Stepper myStepper = Stepper(2048, 8, 10, 9, 11);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, NEOPIN, NEO_GRB + NEO_KHZ800);

void setup() {
  for ( int i = 0; i > RANGE_LED; i++ ) {
    set_led( i, 0, 0, 0, 0);
  }
  myStepper.setSpeed(5);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  Serial.begin(9600);
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  strip.begin();
  strip.show();
}

void loop() {
  stepper();
  distance();
  ring();
  show_me_the_light();
}

void stepper()  {
  if ( clockwise )  {
    stepping = stepping + 1;
    myStepper.step(1);
    if ( stepping == 917 ) {
      clockwise = false;
    }
  } else  {
    stepping = stepping - 1;
    myStepper.step(-1);
    if ( stepping == 0 ) {
      clockwise = true;
    }
  }
}

void set_led( int led_index, unsigned int hue, unsigned int sat, unsigned int value, int counter ) {
  led_matrix[ led_index ][0] = hue;
  led_matrix[ led_index ][1] = sat;
  led_matrix[ led_index ][2] = value;
  led_matrix[ led_index ][3] = counter;
}

void ring() {
  led = (stepping / (MAX_LED - RANGE_LED)) + 1;
  if ( entfernung > 299 ) {
    set_led( led, 21845, 255, MAX_BR, FADER);
  } else {
    if ( entfernung > 199 && entfernung < 300  )  {
      set_led( led, 14922, 255, MAX_BR, FADER);
    } else if ( entfernung > 99 && entfernung < 200  )  {
      set_led( led, 5000, 255, MAX_BR, FADER);
    } else if ( entfernung > 49 && entfernung < 100  )  {
      set_led( led, 2500, 255, MAX_BR, FADER);
    } else  {
      set_led( led, 0, 255, MAX_BR, FADER);
    }
  }
}

void show_me_the_light()  {
  strip.clear();
  for ( int i = 1; i <= RANGE_LED + 1; i++ )  {
    if ( led_matrix[ i ][ 3 ] > 0 ) {
      led_matrix[ i ][ 3 ] = led_matrix[ i ][ 3 ] - 1;
      if ( led_matrix[1][3] != FADER - 1 )  {
        if ( led_matrix[i][2] > 0 )
          led_matrix[i][2] = led_matrix[i][2] - 1;
        color = strip.ColorHSV( led_matrix[ i ][ 0 ], led_matrix[ i ][ 1 ], led_matrix[ i ][ 2 ]);
      } else  {
        color = strip.ColorHSV( led_matrix[ i ][ 0 ], led_matrix[ i ][ 1 ], led_matrix[ i ][ 2 ] );
      }
      strip.setPixelColor( i, color);
    }
  }
  strip.show();
}

void distance() {
  digitalWrite(TRIGGER, LOW);
  delay(5);
  digitalWrite(TRIGGER, HIGH);
  delay(10);
  digitalWrite(TRIGGER, LOW);
  dauer = pulseIn(ECHO, HIGH);
  entfernung = (dauer / 2) * 0.03432;
}
