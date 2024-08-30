#include "gpio.h"
#include "twim.h"
#include "usart.h"

#define TWIM_INSTANCE &AVR32_TWIM0
#define TWIM_SPEED 100000

int wm8805 = 0x3A;
uint8_t error, SR;
int SPDSTAT, INTSTAT;
int fs = 0;
bool toggle = 0;

void setup() {
  const twim_options_t twim_options = {
      .pba_hz = 12000000,
      .speed = TWIM_SPEED,
      .chip = wm8805,
      .smbus = false,
  };
  twim_master_init(TWIM_INSTANCE,
                   &twim_options); // Initialize USART for serial communication
  static const gpio_map_t USART_GPIO_MAP = {
      {AVR32_USART1_RXD_PIN, AVR32_USART1_RXD_FUNCTION},
      {AVR32_USART1_TXD_PIN, AVR32_USART1_TXD_FUNCTION}};
  gpio_enable_module(USART_GPIO_MAP,
                     sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

  usart_options_t usart_options = {
      .baudrate = 115200,
      .charlength = 8,
      .paritytype = USART_NO_PARITY,
      .stopbits = USART_1_STOPBIT,
      .channelmode = USART_NORMAL_CHMODE,
  };
  usart_init_rs232(
      &AVR32_USART1, &usart_options,
      12000000); // Setup GPIO for WM8805 reset pin (assuming GPIO pin 2)
  gpio_configure_pin(AVR32_PIN_PA02, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);

  delay_ms(1000); // for debugging, gives me time to fire up a serial terminal
  error = twim_probe(TWIM_INSTANCE, wm8805);

  if (error == TWIM_SUCCESS) {
    usart_write_line(&AVR32_USART1, "WM8805 found\r\n");
  } else {
    usart_write_line(&AVR32_USART1, "WM8805 not found\r\n");
  }

  usart_write_line(&AVR32_USART1, "Device ID: ");
  uint8_t c = ReadRegister(wm8805, 1);
  usart_putchar(&AVR32_USART1, c);

  c = ReadRegister(wm8805, 0);
  usart_putchar(&AVR32_USART1, c);

  usart_write_line(&AVR32_USART1, " Rev. ");
  c = ReadRegister(wm8805, 2);
  usart_putchar(&AVR32_USART1, c);
  usart_write_line(&AVR32_USART1, "\r\n");

  DeviceInit(wm8805);

  delay_ms(500);
}

void loop() {
  INTSTAT = 0;
  while (INTSTAT == 0) {
    delay_ms(100);
    INTSTAT = ReadRegister(wm8805, 11);
  }
  // Handle interrupt and other logic
  // Code logic remains mostly unchanged but uses different delay and UART
  // functions
}

uint8_t ReadRegister(int devaddr, int regaddr) {
  uint8_t data = 0;
  twim_write(TWIM_INSTANCE, &regaddr, 1, devaddr, false);
  twim_read(TWIM_INSTANCE, &data, 1, devaddr);
  return data;
}

void WriteRegister(int devaddr, int regaddr, uint8_t dataval) {
  uint8_t buffer[2] = {regaddr, dataval};
  twim_write(TWIM_INSTANCE, buffer, 2, devaddr, true);
}

void DeviceInit(int devaddr) {
  WriteRegister(devaddr, 0, 0);

  WriteRegister(devaddr, 7, 0x04);
  WriteRegister(devaddr, 8, 0x30);
  WriteRegister(devaddr, 10, 0x7E);
  WriteRegister(devaddr, 27, 0x0A);
  WriteRegister(devaddr, 28, 0xCA);

  WriteRegister(devaddr, 6, 7);
  WriteRegister(devaddr, 5, 0x36);
  WriteRegister(devaddr, 4, 0xFD);
  WriteRegister(devaddr, 3, 0x21);

  WriteRegister(devaddr, 9, 0);
  WriteRegister(devaddr, 30, 0);

  WriteRegister(devaddr, 8, 0x1B); // Select Input 4 (TOSLINK)
}

void delay_ms(uint32_t ms) {
  while (ms--) {
    for (uint32_t i = 0; i < 1000; i++) {
      asm volatile("nop");
    }
  }
}