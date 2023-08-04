#include <Arduino.h>
#include <avr/io.h>
#include <string.h>

#define rev_limiter 11000 // Rpm value
#define ignition_cut_time 20000 // >Microseconds< // 1ms = 1000μs
#define trigger_coil_angle 27
#define RPM_0    10 // This curve is linear from 1000 RPM to 4000.
#define RPM_250  10
#define RPM_500  10
#define RPM_750  10
#define RPM_1000 10
#define RPM_1250 11.4
#define RPM_1500 12.8
#define RPM_1750 14.2
#define RPM_2000 15.6
#define RPM_2250 17
#define RPM_2500 18.5
#define RPM_2750 19.9
#define RPM_3000 21.3
#define RPM_3250 22.7
#define RPM_3500 24.1
#define RPM_3750 25.5
#define RPM_4000 27 // After this point, the curve becomes flat

////////////////////////////////////////////////////////////////////////////
////////////////////////////Serial Communcation/////////////////////////////
////////////////////////////////////////////////////////////////////////////

void uart_init() {
  UBRR0 = 8; // For 16MHz and baud rate of 115200
  UCSR0B = (1 << TXEN0) | (1 << RXEN0); // Enable transmitter and receiver
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data, no parity, 1 stop bit
}

void uart_transmit_char(uint8_t data) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = data;
}

void uart_transmit_string(const char* string) {
  for (int i = 0; string[i] != '\0'; i++)
    uart_transmit_char(string[i]);
}

void uart_transmit_uint(uint8_t data) {
  char buffer[4]; // Buffer to hold the string representation of the number
  snprintf(buffer, sizeof(buffer), "%u", data); // Convert uint8_t to string
  uart_transmit_string(buffer); // Transmit the string over UART
}

void uart_transmit_uint(uint16_t data) {
  char buffer[6]; // Buffer to hold the string representation of the number
  snprintf(buffer, sizeof(buffer), "%u", data); // Convert uint16_t to string
  uart_transmit_string(buffer); // Transmit the string over UART
}

void uart_transmit_uint(uint32_t data) {
  char buffer[11]; // Buffer to hold the string representation of the number
  snprintf(buffer, sizeof(buffer), "%lu", data); // Convert uint32_t to string
  uart_transmit_string(buffer); // Transmit the string over UART
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////Main Function///////////////////////////////
////////////////////////////////////////////////////////////////////////////

int main() {
  float ignition_map[17] = { RPM_0, RPM_250, RPM_500, RPM_750, RPM_1000, RPM_1250, RPM_1500, RPM_1750, RPM_2000, RPM_2250, RPM_2500, RPM_2750, RPM_3000, RPM_3250, RPM_3500, RPM_3750, RPM_4000 };
  float angle_difference;
  uint8_t led = 0, map_index;
  uint16_t rpm = 0, angle_difference2; 
  uint32_t pulse_interval, delay_time;

  PORTB = 0b00000000;
  DDRB = 0b00100010;
  DDRD = 0b01000000;
  TCCR1B = 0b00000011; // 64 prescaler

  uart_init();

  while (true) {
    while ((PINB & 0b00000001) == 0b00000000);
    while ((PINB & 0b00000001) == 0b00000001);

    // Sets the value of the timer to pulse_interval >> time between 2 signals >microseconds<
    pulse_interval = static_cast<uint32_t>(TCNT1) * 4; 

    // Reset the timer
    TCNT1 = 0;
     
    // Convert pulse_interval >in microseconds< into rpm
    rpm = 60000000 / pulse_interval;
    
    // Rounds the rpm value
    map_index = round(rpm / 250.0);
    
    // Safety measure to limit the advence to 16 degrees >map_index 9< if the rpm reading exceeds 11000rpm (in case of an rpm sensor failure)
    if (map_index > 44) {
      map_index = 9; //
    }
    // Caps the value of map_index to 16 because thats the last value in the ignition table above >RPM_4000 27<
    else if (map_index > 16) {
    map_index = 16;
    }
    
    // Calculate the delay >in microseconds< needed to ignite at the specified advence angle in ignition_map
    angle_difference = trigger_coil_angle - ignition_map[map_index];
    delay_time = pulse_interval / 360 * angle_difference;
    angle_difference2 = angle_difference * 100;
    
    //////// Rev limiter and ignition ////////
    // Waits the amount of time specified in delay_time
    while (TCNT1 < delay_time)
    
    // Lighting the led up on pin 13 every x revolutions
    led++;
    if (led == 8) {
      PORTB = 0b00100000;
      led = 0;
    }
    // Check if RPM exceeds the rev_limiter threshold
    if (rpm > rev_limiter) {
      // Keep ignition off for ignition_cut_time
      while (TCNT1 < ignition_cut_time);
    } 
    else {
      PORTB = 0b00000010; // Ignition on pin 9
    }
    // Time during which pin 9 will be high >microseconds<
    while (TCNT1 < delay_time + 25)
    PORTB = 0b00000000;
    
    // Transmit the value of map_index through serial >UART<
    uart_transmit_uint(angle_difference2);
    uart_transmit_string("\n");
    uart_transmit_uint(map_index);
    uart_transmit_string("\n");
    uart_transmit_uint(rpm);
    uart_transmit_string("\n");
    uart_transmit_string("\n");
  }
}

//to do : 2 step