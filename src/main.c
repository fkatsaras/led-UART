#include <platform.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <uart.h>
#include "leds.h"
#include "queue.h"
#include "timer.h"
#include "gpio.h"

#define BUFF_SIZE 128
#define TIMER_INTERVAL 500000 // Timer interval set to 500000 us = 0.5 seconds
#define BUTTON_PIN  PC_13  // The push button is connected to PC_13

static int count = 0;

static int LED_STATE = 0;

Queue rx_queue;

void uart_rx_isr(uint8_t rx);  // UART Receive ISR prototype
void disable_timer(int led_state); // Function prototype for disable_timer
void toggle_led(); // Function prototype for toggle_led

void uart_rx_isr(uint8_t rx) {  // UART Receive ISR
    if ((rx >= '0' && rx <= '9') || rx == 0x7F || rx == '\r') {
        // Store the received character
        queue_enqueue(&rx_queue, rx);
    }
}

void disable_timer(int led_state) { // Function to disable timer and keep the current led state
    leds_set(led_state, 0, 0);
    timer_disable();
}

void toggle_led() { // Function to enable timer and toggle led state
    LED_STATE = !LED_STATE;
    leds_set(LED_STATE, 0, 0);
}

// Callback function to handle button press
void button_pressed_callback() {
    if (LED_STATE == 1) { 
        leds_set(!LED_STATE,0, 0); // Turn on the LED
				
    } else {
        leds_set(LED_STATE,0, 0); // Turn off the LED
    }
		count++;
		char str[100];
		sprintf(str, "%d", count); // Convert integer to string
		uart_print("Button Counter ");
		uart_print(str);
		uart_print("\n\r");
}

int main() {
    uint8_t rx_char = 0;
    char aem[BUFF_SIZE];
    uint32_t buff_index;
    
    queue_init(&rx_queue, 128); // Set up UART
    uart_init(57600);
    uart_set_rx_callback(uart_rx_isr);
    uart_enable();
    __enable_irq();
    
    leds_init(); // Initialize leds
    
    // Setup timer
    timer_init(TIMER_INTERVAL); // Initialize timer with specified interval
    timer_set_callback(toggle_led); // Set up the timer callback function to toggle the led each 0.5 sec
	
		//Set up Button
		gpio_set_mode(BUTTON_PIN, Input); // Set button pin as input
		// Set interrupt trigger for the button
    gpio_set_trigger(BUTTON_PIN, Rising); // Trigger interrupt on rising edge (button press)
		// Set the callback function for the button
    gpio_set_callback(BUTTON_PIN, button_pressed_callback);
    
    uart_print("\r");
    while(1) {
        uart_print("Enter your AEM and press enter: ");
        buff_index = 0;
        do {
            while (!queue_dequeue(&rx_queue, &rx_char))
                __WFI();
            if (rx_char == 0x7F) { // Handle backspace character
                if (buff_index > 0) {
                    buff_index--;
                    uart_tx(rx_char);
                }
            } else {
                // Store and echo the received character back
                aem[buff_index++] = (char)rx_char;
                uart_tx(rx_char);
            }
        } while (rx_char != '\r' && buff_index < BUFF_SIZE);
        // Replace the last character with \0
        aem[buff_index - 1] = '\0';
        uart_print("\r\n");
				
        
        if (buff_index < BUFF_SIZE) {
            // Get buffer's last digit and convert to int
            int last_digit = aem[strlen(aem) - 1] - '0';
            
            if (last_digit % 2 == 0) {  // If AEM is even, disable timer and keep current led state 
                uart_print("Even AEM, LED toggle OFF\r\n");                
                disable_timer(LED_STATE);
            } else { // If not , enable timer and toggle led every 0.5 seconds
                uart_print("Odd AEM, LED toggle ON\r\n");
                timer_enable();
            }
        } else {
            uart_print("Stop trying to overflow my buffer! I resent that!\r\n");
        }
    }
}


// *******************************ARM University Program Copyright © ARM Ltd 2016*************************************
