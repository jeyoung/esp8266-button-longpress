#include <stdio.h>
#include <stdint.h>

#include "mem.h"
#include "osapi.h"
#include "uart.h"
#include "user_interface.h"

#include "main.h"

#define PIN_BUTTON 2
#define PIN_LED 0

struct Board
{
    uint32_t pin_button;
    uint32_t pin_led;

    uint16_t button_bounces;
    uint32_t button_timestamp;

    uint16_t button_down;
    uint16_t button_longpress;
    uint16_t button_up;
};

static os_timer_t os_timer = {0};
static struct Board board = {0};

static ICACHE_FLASH_ATTR void board_init(struct Board *board, uint32_t pin_button, uint32_t pin_led)
{
    uart_init(BIT_RATE_921600, BIT_RATE_921600);
    gpio_init();

    board->pin_button = pin_button;
    board->pin_led = pin_led;
    
    board->button_bounces = 0xFFFF;
    board->button_timestamp = 0;

    board->button_down = 0;
    board->button_longpress = 0;
    board->button_up = 0;

    GPIO_DIS_OUTPUT(board->pin_button);
}

static ICACHE_FLASH_ATTR void board_read(struct Board *board)
{
    uint32_t now = system_get_time();

    board->button_bounces = (board->button_bounces << 1) | (uint16_t)GPIO_INPUT_GET(board->pin_button);
    board->button_up = board->button_down && (board->button_bounces > 0xFF00);
    board->button_longpress = board->button_down && now - board->button_timestamp > 3000000;
    board->button_down = board->button_bounces < 0xFF00;
    if (board->button_down) {
	if (!board->button_timestamp)
	    board->button_timestamp = now;
    } else
	board->button_timestamp = 0;
}

static ICACHE_FLASH_ATTR void board_led(struct Board *board)
{
    GPIO_OUTPUT_SET(board->pin_led, board->button_longpress);
}

static void main_on_timer(void *arg)
{
    board_read(&board);
    board_led(&board);
}

void ICACHE_FLASH_ATTR user_init(void)
{
    board_init(&board, GPIO_ID_PIN(PIN_BUTTON), GPIO_ID_PIN(PIN_LED));

    os_timer_disarm(&os_timer);
    os_timer_setfn(&os_timer, &main_on_timer, (void *)NULL);
    os_timer_arm(&os_timer, 1, 1);
}
