#include <sdk/config.h>
#include <stdio.h>

#include <arch/board/board.h>
#include <arch/chip/pin.h>

#define LED0 PIN_I2S1_BCK
#define LED1 PIN_I2S1_LRCK
#define LED2 PIN_I2S1_DATA_IN
#define LED3 PIN_I2S1_DATA_OUT

int main(int argc, char *argv[])
{
  board_gpio_write(LED0, -1);
  board_gpio_config(LED0, 0, 0, true, PIN_FLOAT);
  board_gpio_write(LED1, -1);
  board_gpio_config(LED1, 0, 0, true, PIN_FLOAT);
  board_gpio_write(LED2, -1);
  board_gpio_config(LED2, 0, 0, true, PIN_FLOAT);
  board_gpio_write(LED3, -1);
  board_gpio_config(LED3, 0, 0, true, PIN_FLOAT);

  while(1)
  {
    sleep(2);
    board_gpio_write(LED0, 1);
    board_gpio_write(LED1, 1);
    board_gpio_write(LED2, 1);
    board_gpio_write(LED3, 1);
    sleep(2);
    board_gpio_write(LED0, 0);
    board_gpio_write(LED1, 0);
    board_gpio_write(LED2, 0);
    board_gpio_write(LED3, 0);
  }
  return 0;
}
