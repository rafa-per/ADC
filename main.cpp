#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "energy_meter.h"

#define ADC_NUM_V 0
#define ADC_NUM_I 1
#define ADC_PIN_V (26 + ADC_NUM_V)
#define ADC_PIN_I (26 + ADC_NUM_I)
#define ADC_VREF 3.3
#define ADC_RANGE (1 << 12)
#define ADC_CONVERT (ADC_VREF / (ADC_RANGE - 1))
#define F_ADC 6000
#define F_REF 48000000


int main() {
    stdio_init_all();
    printf("Beep boop, listening...\n");
    adc_init();
    adc_gpio_init(ADC_PIN_V);
    adc_gpio_init(ADC_PIN_I);
    adc_select_input(ADC_NUM_V);
    adc_set_round_robin	(0x03); //https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga16d25be75e16f0671d3e3185b0b59771
    adc_set_clkdiv(float(F_REF)/F_ADC - 1); //https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga333f7ca46a11241d96d0a4c24f0d4bb0
    adc_fifo_setup(true,false,2,false,false); //https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga2a6fa32da04e65a027e14deaf32b8f10
    irq_set_exclusive_handler(ADC0_IRQ_FIFO, &energy_meter_isr);
    //*************************************************
    adc_irq_set_enabled(true);
    adc_run(true);    

    energy_meter_init();

    irq_set_enabled(ADC0_IRQ_FIFO, true); //
    vTaskStartScheduler();    
    
    return 0;
}
