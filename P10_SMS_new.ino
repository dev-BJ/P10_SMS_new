#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <SoftwareSerial.h>
#include <DMD.h>
#include <Arial_Black_16.h>
#include <EEPROM.h>
#include <Thread.h>

#define ROW 2
#define COL 1
#define SCROLL_LEN 32 * 2
#define A_SEC 3
#define DEFAULT_TEXT "Welcome to Mechatronics Engineering Department."
#define DATA_ADDR 0
#define RX 5
#define TX 4
#define MAX_LEN 400
#define SCROLL_SPEED 3

// TX 5, RX 4

DMD dmd(ROW, COL);
SoftwareSerial sms(RX, TX);
Thread myThread = Thread();

int disp_x = SCROLL_LEN + 5;
volatile uint8_t scrollTimer = 0;
String disp_text = DEFAULT_TEXT;
int disp_text_size = disp_text.length();

void setupTimer() {
  TCCR1B = 0x03;  // No prescaling
  TIMSK1 = 0x01;
  sei();
}

void scrollScreen() {
  disp_text_size = disp_text.length();
  // Update scroll; Range 1 - 10; Change to alter scroll speed
  if (scrollTimer >= A_SEC * SCROLL_SPEED) {
    disp_x--;
    dmd.selectFont(Arial_Black_16);
    // Serial.println(disp_text_size);
    dmd.drawString(disp_x, 0, disp_text.c_str(), disp_text_size, GRAPHICS_NORMAL);
    if (disp_x <= ~(10 * disp_text_size) + 50) {
      dmd.clearScreen(true);
      disp_x = SCROLL_LEN + 5;
    }

    scrollTimer = 0;
  }
}

void save_str_to_eeprom(int address, const String& data) {
  int length = data.length();
  EEPROM.write(address, length);  // Save the length of the string first

  for (int i = 0; i < length; i++) {
    EEPROM.write(address + 1 + i, data[i]);  // Save each character
  }
}

String read_str_from_eeprom(int address) {
  int length = EEPROM.read(address);  // Read the length of the string
  String result = "";

  for (int i = 0; i < length; i++) {
    char c = EEPROM.read(address + 1 + i);
    result += c;
  }

  return result;
}

void clear_eeprom() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

void handleSms() {
  while (sms.available() > 0) {
    PORTD = (1 << PD3);
    String new_msg = sms.readString();
    Serial.println("Msg: " + new_msg);

    if (new_msg.indexOf("default") >= 0) {
      disp_text = DEFAULT_TEXT;
      disp_x = SCROLL_LEN + 5;
      clear_eeprom();
      dmd.clearScreen(true);
    } else if (new_msg.indexOf("+CMT") >= 0) {

      disp_text = new_msg.substring(new_msg.lastIndexOf("\"") + 3, new_msg.length());  // The magic happens here

      if (disp_text != "") {
        Serial.println("New Message: " + disp_text);
        // save display text for later use
        clear_eeprom();
        save_str_to_eeprom(DATA_ADDR, disp_text);
      }
      disp_x = SCROLL_LEN + 5;
      dmd.clearScreen(true);
    }
    PORTD &= ~(1 << PD3);
  }
}

void setup_sms() {
  sms.println(F("AT"));
  updateSerial();
  sms.println(F("AT+CSQ"));
  updateSerial();
  sms.println(F("AT+CMGF=1"));
  updateSerial();
  sms.println(F("AT+CNMI=2,2,0,0,0"));
  updateSerial();
  sms.println(F("AT+CMGR=1"));
  updateSerial();
}

void updateSerial() {
  _delay_ms(500);
  while (sms.available()) {
    Serial.write(sms.read());
  }
}

void lastMsg() {
  if (EEPROM.read(DATA_ADDR) != 0) {
    disp_text = read_str_from_eeprom(DATA_ADDR);
  }
}

void setup() {
  setupTimer();
  Serial.begin(9600);
  sms.begin(2400);
  _delay_ms(10);
  setup_sms();
  _delay_ms(10);
  dmd.clearScreen(true);
  DDRD = (1 << PD3);
  lastMsg();
}

void loop() {
  handleSms();
  scrollScreen();
}

ISR(TIMER1_OVF_vect) {
  dmd.scanDisplayBySPI();
  // scrollScreen();
  scrollTimer++;
}
