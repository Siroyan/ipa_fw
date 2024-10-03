#include <Arduino.h>
#include <TinyWireM.h>

#define ADJ_PULSE_PIN 1
#define SRC_PULSE_PIN 4

#define NOW 0
#define PRV 1
#define RISE 0
#define FALL 1

void display_digits3(uint16_t);
void display_digits4(uint16_t);
void init_lcd(void);
void send_command(byte);
void send_char(byte);

float coeff = 0.f;

bool prv_src_pulse_flag = false;
bool now_src_pulse_flag = false;

unsigned long prev_polling_time_micro = 0;
unsigned long prev_display_time_micro = 0;

unsigned long src_pulse_time[2] = {0, 0};
unsigned long src_pulse_width = 0;
unsigned long adj_pulse_width = 0;

void setup() {
  pinMode(ADJ_PULSE_PIN, OUTPUT);
  pinMode(SRC_PULSE_PIN, INPUT);
  digitalWrite(ADJ_PULSE_PIN, LOW);
  init_lcd();
}

void loop() {
  if (micros() - prev_polling_time_micro > 10) {
    prev_polling_time_micro = micros();
    prv_src_pulse_flag = now_src_pulse_flag;
    now_src_pulse_flag = digitalRead(SRC_PULSE_PIN);
    if (now_src_pulse_flag != prv_src_pulse_flag) {
      if (now_src_pulse_flag) { // L -> H
        digitalWrite(ADJ_PULSE_PIN, HIGH);
        src_pulse_time[RISE] = prev_polling_time_micro;
      } else {              // H -> L
        src_pulse_time[FALL] = prev_polling_time_micro;
        src_pulse_width = (src_pulse_time[FALL] - src_pulse_time[RISE]);
        adj_pulse_width = src_pulse_width * coeff;
      }
    }
    if (src_pulse_time[RISE] + adj_pulse_width < micros()) {
      digitalWrite(ADJ_PULSE_PIN, LOW);
    }
  }
  if (micros() - prev_display_time_micro > 3000000) {
    prev_display_time_micro = micros();
    uint16_t val = 0;
    val = analogRead(3);
    coeff = (val + 252.f) / 505.f;
    send_command(B10000000 + 4);
    display_digits3(val);
    send_command(B11000000 + 0);
    send_char('S');
    display_digits3(uint16_t(src_pulse_width / 1000));
    send_char('A');
    display_digits3(uint16_t(adj_pulse_width / 1000));
  }
}

void display_digits3(uint16_t num) {
  uint8_t d100 = num / 100;
  uint8_t d10  = (num - d100 * 100) / 10;
  uint8_t d1   = num - d100 * 100 - d10 * 10;
  send_char(0x30 + d100);
  send_char(0x30 + d10);
  send_char(0x30 + d1);
}

void init_lcd() {
  TinyWireM.begin();
  send_command(B00111100); //Function set
  send_command(B00001000); //Display off
  send_command(B00000001); //Display clear
  send_command(B00000111); //Entry mode set
  
  send_command(B10000000); //set DDRAM address
  send_char('V');
  send_char('a');
  send_char('l');
  send_char('=');
  
  send_command(B00001100); //Display on
}

void send_command(byte command_data) {
  TinyWireM.beginTransmission(0x3C);
  TinyWireM.send(0x00);
  TinyWireM.send(command_data);
  TinyWireM.endTransmission();
}

void send_char(byte char_data) {
  TinyWireM.beginTransmission(0x3C);
  TinyWireM.send(0x40);
  TinyWireM.send(char_data);
  TinyWireM.endTransmission();
}