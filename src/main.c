/*
 * IR.c
 *
 * Created: 23.06.2019 12:52:58
 * Author : ich
 */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "irmp.h"
#include "irsnd.h"
#include "hcConfig.h"
#include "hcI2cSlave.h"

uint8_t irBuffer[30];

void timer1_init()
{
	OCR1A   =  (F_CPU / F_INTERRUPTS) - 1;                                  // compare value: 1/15000 of CPU frequency
	TCCR1B  = (1 << WGM12) | (1 << CS10);                                   // switch CTC Mode on, set prescaler to 1
	
	#ifdef TIMSK1
	TIMSK1  = 1 << OCIE1A;                                                  // OCIE1A: Interrupt by timer compare
	#else
	TIMSK   = 1 << OCIE1A;                                                  // OCIE1A: Interrupt by timer compare
	#endif
}

void ledCallback(uint8_t on)
{
	if (on && hcLedActive.custom) {
		HC_CUSTOM_LED_PORT |= (1 << HC_CUSTOM_LED_PIN);
	} else {
		HC_CUSTOM_LED_PORT &= ~(1 << HC_CUSTOM_LED_PIN);
	}
}

int main(void)
{	
	IRMP_DATA irmp_data;
	
	hcInitI2C();
	HC_CUSTOM_LED_PORT |= (1 << HC_CUSTOM_LED_PIN);
	_delay_ms(1000);
	irmp_init();
	irsnd_init();                                                          // initialize irmp
	timer1_init();                                                          // initialize timer1
	HC_CUSTOM_LED_PORT &= ~(1 << HC_CUSTOM_LED_PIN);
	irmp_set_callback_ptr(ledCallback);
	irsnd_set_callback_ptr(ledCallback);
	
	uint8_t i = 0;
	
    while (1) 
    {
	    if (irmp_get_data(&irmp_data)) {
			i = hcI2cDataChangedLength;
			
			if (i > 25) {
				i = 0;
			}
			
		    irBuffer[i++] = irmp_data.protocol;
		    irBuffer[i++] = irmp_data.address>>8;
		    irBuffer[i++] = irmp_data.address;
		    irBuffer[i++] = irmp_data.command>>8;
		    irBuffer[i++] = irmp_data.command;
			hcI2cDataChangedLength = i;
		}
    }
}

uint8_t getChangedData()
{
	uint8_t i = 0;
	
	for (; i < hcI2cDataChangedLength; i++) {
		hcI2cWriteBuffer[i] = irBuffer[i];
	}
	
	hcI2cDataChangedLength = 0;
	
	return i+1;
}

void setData()
{
	IRMP_DATA irmpData;
	
	switch (hcI2cReadBuffer[0]) {
		case 0:
			irmpData.protocol = hcI2cReadBuffer[2];
			irmpData.address = (hcI2cReadBuffer[3]<<8);
			irmpData.address += hcI2cReadBuffer[4];
			irmpData.command = (hcI2cReadBuffer[5]<<8);
			irmpData.command += hcI2cReadBuffer[6];
			irmpData.flags = 0;
			
			irsnd_send_data(&irmpData, FALSE);
			_delay_ms(10);
			break;
	}
}

ISR(TIMER1_COMPA_vect)                     // Timer1 output compare A interrupt service routine, called every 1/15000 sec
{
	if (!irsnd_ISR()) {
	    irmp_ISR();                                                        // call irmp ISR
	}
}