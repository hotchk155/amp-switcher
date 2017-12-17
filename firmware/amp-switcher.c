//
// HEADER FILES
//
#include <system.h>
#include "chars.h"

// PIC CONFIG BITS
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000

//
// TYPE DEFS
//
typedef unsigned char byte;

//
// MACRO DEFS
//


#define P_BUTTON1	porta.3
#define P_BUTTON2	porta.4
#define P_BUTTON3	porta.5
#define P_SRI_DAT	portc.6

#define P_SRI_LOAD	latc.3
#define P_SR_SCK	latb.4
#define P_SRO_EN	latb.5
#define P_SRO_RCK1	latb.6
#define P_SRO_RCK3	latc.7
#define P_SRO_RCK2	latb.7
#define P_SRO_DAT	latc.2

#define P_LED1		lata.0 // YELLOW
#define P_LED2		lata.1 // BLUE
#define P_DIGIT1	latc.1
#define P_DIGIT2	latc.0
#define P_DIGIT3	lata.2

				  //	0b76543210
#define MASK_TRISA	  	0b00111000
#define MASK_TRISB		0b00000000
#define MASK_TRISC		0b01110000

#define MASK_WPUA	  	0b00111000





#define TIMER_0_INIT_SCALAR			5	// Timer 0 is an 8 bit timer counting at 250kHz
#define KEY_DEBOUNCE_MS				50
#define CABLE_DETECT_DEBOUNCE_MS	200

#define K_BUTTON1	16
#define K_BUTTON2	17
#define K_BUTTON3	18


//
// GLOBAL DATA
//

// timer stuff
volatile byte ms_tick = 0;
volatile unsigned int timer_init_scalar = 0;

// define the buffer used to receive MIDI input
#define SZ_RXBUFFER 20
volatile byte rxBuffer[SZ_RXBUFFER];
volatile byte rxHead = 0;
volatile byte rxTail = 0;


#define NUM_CHANNELS 8

//volatile byte chan_switches = 0;
//volatile byte cable_detect = 0;

enum {
	UI_DIGIT1,
	UI_DIGIT2,
	UI_DIGIT3
};
volatile byte ui_state;


typedef struct {
	byte pending;
	unsigned long key_state;
	unsigned long cd_state;
} INPUT_STATE;

typedef struct {
	byte pending;
	byte digit[3];
	byte chan_leds[2];
	byte relays;
} OUTPUT_STATE;

volatile INPUT_STATE panel_input = {0};
volatile INPUT_STATE ui_panel_input = {0};

volatile OUTPUT_STATE output_state = {0};
volatile OUTPUT_STATE ui_output_state = {0};


byte load_sr(byte dat) {
	byte result = 0;
	byte mask = 0x01;
	for(int i=0; i<8; ++i) {
		result<<=1;
		if(!P_SRI_DAT)
			result |= 1;
	
		P_SR_SCK = 0;
		P_SRO_DAT = !!(dat & mask);
		P_SR_SCK = 1;
		mask <<= 1;
	}
	return result;
}


////////////////////////////////////////////////////////////
// INTERRUPT HANDLER CALLED WHEN CHARACTER RECEIVED AT 
// SERIAL PORT OR WHEN TIMER 1 OVERLOWS
void interrupt( void )
{
	// timer 0 rollover ISR. Maintains the count of 
	// "system ticks" that we use for key debounce etc
	if(intcon.2)
	{
		tmr0 = TIMER_0_INIT_SCALAR;
		ms_tick = 1;
		intcon.2 = 0;

		switch(ui_state) {
			/////////////////////////////////////////////////////////////
			case UI_DIGIT1:
				// turn off all the display source transistors
				P_DIGIT1 = 1;
				P_DIGIT2 = 1;
				P_DIGIT3 = 1;

				if(output_state.pending) {
					ui_output_state = output_state;
					output_state.pending = 0;
				}

				// load the data for the first digit and grab the
				// first set of switch inputs (cable detect)
				P_SRO_RCK1 = 0;
				load_sr(~ui_output_state.digit[0]);	
				P_SRO_RCK1 = 1;
				
				// turn on the source drive for the first segment
				P_DIGIT1 = 0;
				ui_state = UI_DIGIT2;
				break;
							
			/////////////////////////////////////////////////////////////
			case UI_DIGIT2:
				P_DIGIT1 = 1;
				
				P_SRO_RCK1 = 0;
				load_sr(~ui_output_state.digit[1]);	
				P_SRO_RCK1 = 1;
				P_DIGIT2 = 0;
				ui_state = UI_DIGIT3;
				break;
				
			/////////////////////////////////////////////////////////////
			case UI_DIGIT3:
				P_SRI_LOAD = 0;
				P_SRI_LOAD = 1;
			
				P_DIGIT2 = 1;
			
				P_SRO_RCK1 = 0;
				P_SRO_RCK2 = 0;
				P_SRO_RCK3 = 0;
				
				ui_panel_input.cd_state = load_sr(ui_output_state.relays);
				ui_panel_input.key_state = load_sr(ui_output_state.chan_leds[0]);
				if(!P_BUTTON1) 
					ui_panel_input.key_state |= (((unsigned long)1)<<K_BUTTON1);
				if(!P_BUTTON2) 
					ui_panel_input.key_state |= (((unsigned long)1)<<K_BUTTON2);
				if(!P_BUTTON3) 
					ui_panel_input.key_state |= (((unsigned long)1)<<K_BUTTON3);				
				if(!panel_input.pending) {
					panel_input = ui_panel_input;
					panel_input.pending = 1;
				}
				load_sr(ui_output_state.chan_leds[1]);
				load_sr(~ui_output_state.digit[2]);	
				P_SRO_RCK1 = 1;
				P_SRO_RCK2 = 1;
				P_SRO_RCK3 = 1;
				P_DIGIT3 = 0;
				ui_state = UI_DIGIT1;
				break;			
		}	
	}
		
	// serial rx ISR
	if(pir1.5)
	{	
		// get the byte
		byte b = rcreg;
		
		// calculate next buffer head
		byte nextHead = (rxHead + 1);
		if(nextHead >= SZ_RXBUFFER) 
		{
			nextHead -= SZ_RXBUFFER;
		}
		
		// if buffer is not full
		if(nextHead != rxTail)
		{
			// store the byte
			rxBuffer[rxHead] = b;
			rxHead = nextHead;
		}		
	}
}


////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void initUSART()
{
	pir1.1 = 1;		//TXIF 		
	pir1.5 = 0;		//RCIF
	
	pie1.1 = 0;		//TXIE 		no interrupts
	pie1.5 = 1;		//RCIE 		enable
	
	baudcon.4 = 0;	// SCKP		synchronous bit polarity 
	baudcon.3 = 1;	// BRG16	enable 16 bit brg
	baudcon.1 = 0;	// WUE		wake up enable off
	baudcon.0 = 0;	// ABDEN	auto baud detect
		
	txsta.6 = 0;	// TX9		8 bit transmission
	txsta.5 = 1;	// TXEN		transmit enable
	txsta.4 = 0;	// SYNC		async mode
	txsta.3 = 0;	// SEDNB	break character
	txsta.2 = 0;	// BRGH		high baudrate 
	txsta.0 = 0;	// TX9D		bit 9

	rcsta.7 = 1;	// SPEN 	serial port enable
	rcsta.6 = 0;	// RX9 		8 bit operation
	rcsta.5 = 1;	// SREN 	enable receiver
	rcsta.4 = 1;	// CREN 	continuous receive enable
		
	spbrgh = 0;		// brg high byte
	spbrg = 31;		// brg low byte (31250)	
	
}

/*
////////////////////////////////////////////////////////////
// RUN MIDI THRU
void midiThru()
{
	// loop until there is no more data or
	// we receive a full message
	for(;;)
	{
		// buffer overrun error?
		if(rcsta.1)
		{
			rcsta.4 = 0;
			rcsta.4 = 1;
		}
		// any data in the buffer?
		if(rxHead == rxTail)
		{
			// no data ready
			return;
		}
		
		// read the character out of buffer
		byte q = rxBuffer[rxTail];
		if(++rxTail >= SZ_RXBUFFER) 
			rxTail -= SZ_RXBUFFER;
		
		// external clock mode
		if(MODE_EXTCLOCK == _mode) 
		{		
			if(_options & OPTION_THRUANIMATE) {
				// animate and send
				duty[q%6] = q%INITIAL_DUTY;
				send(q);
			}
			else {
				// blink the four edge LEDs in time 
				// with the external MIDI beat clock
				byte d = 0;
				switch(q) {
					case 0xF8:
						if(!tickCount) {
							d = maxDuty;
						}
						if(++tickCount>=24) {
							tickCount = 0;
						}
						break;
					case 0xFA:
						tickCount = 0;
						break;									
				}
				duty[0] = d;
				duty[1] = d;
				duty[4] = d;
				duty[5] = d;				
				
				P_LED2 = 1;
				P_LED3 = 1;
				send(q);
				P_LED2 = 0;
				P_LED3 = 0;				
			}
		}
		// should we indicate thru traffic with flickering LEDs?
		else 
		{					
		
			// Check for MIDI realtime message (e.g. clock)
			if((q & 0xF8) == 0xF8)
			{
				// if we are not passing realtime messages then skip it
				if(!(_options & OPTION_PASSREALTIMEMSG))
					continue;
			}
			else
			{
				// if we are not passing non-realtime messages then skip it
				if(!(_options & OPTION_PASSOTHERMSG))
					continue;
			}
				
			// flicker and send
			P_LED2 = 1;
			P_LED3 = 1;
			send(q);
			P_LED2 = 0;
			P_LED3 = 0;
		}
	}		
}
*/





/*
void refresh() 
{
	P_DIGIT1 = 1;
	P_DIGIT2 = 1;
	P_DIGIT3 = 1;
			

//	P_SR_SCK = 0;
	P_SRI_LOAD = 0;
	P_SRI_LOAD = 1;
//	P_SR_SCK = 1;

	P_SRO_RCK1 = 0;
	cd_state = load_sr(~digit[0]);	
	P_SRO_RCK1 = 1;
	P_DIGIT1 = 0;
	delay_ms(1);
	P_DIGIT1 = 1;

	P_SRO_RCK1 = 0;
	key_state = load_sr(~digit[1]);	
	P_SRO_RCK1 = 1;
	P_DIGIT2 = 0;
	delay_ms(1);
	P_DIGIT2 = 1;

	P_SRO_RCK1 = 0;
	load_sr(~digit[2]);	
	P_SRO_RCK1 = 1;
	P_DIGIT3 = 0;
	delay_ms(1);
	P_DIGIT3 = 1;	
	
	P_SRO_RCK2 = 0;
	P_SRO_RCK3 = 0;
	load_sr(relays);
	load_sr(chan_leds[0]);
	load_sr(chan_leds[1]);
	load_sr(0);
	P_SRO_RCK2 = 1;
	P_SRO_RCK3 = 1;
	
	//P_SRI_LOAD = 0;
	
	if(!P_BUTTON1) 
		key_state |= (((unsigned long)1)<<K_BUTTON1);
	if(!P_BUTTON2) 
		key_state |= (((unsigned long)1)<<K_BUTTON2);
	if(!P_BUTTON3) 
		key_state |= (((unsigned long)1)<<K_BUTTON3);
	
}
*/


#define LED_OFF		0x00
#define LED_GREEN	0x01
#define LED_RED		0x02
#define LED_MASK	0x03

void set_chan_led(byte which, byte state) 
{
	which = 7-which;
	byte shift = (which%4)*2;
	if(which < 4) {
		output_state.chan_leds[0] &= ~(LED_MASK<<shift);
		output_state.chan_leds[0]  |= (state<<shift);		
	}
	else {
		which %= 4;
		output_state.chan_leds[1] &= ~(LED_MASK<<shift);
		output_state.chan_leds[1]  |= (state<<shift);		
	}
}

#define K_CHAN1 0
#define K_CHAN2 7
#define K_CHAN3 1
#define K_CHAN4 6
#define K_CHAN5 5
#define K_CHAN6 3
#define K_CHAN7 4
#define K_CHAN8 2


enum {
	CH_AMP, 
	CH_CAB, 
	CH_FX
};

enum {
	IS_DISCONNECTED,
	IS_CONNECTED,
	IS_SELECTED
};

typedef struct {
	byte type;
	byte status;
} CHANNEL_INFO;

CHANNEL_INFO chan[NUM_CHANNELS];

void init_channels() {	
	int i;
	for(i=0; i<=3; ++i) {
		chan[i].type = CH_AMP;
		//chan[i].status = IS_DISCONNECTED;
		chan[i].status = IS_CONNECTED;
	}
	for(i=4; i<=6; ++i) {
		chan[i].type = CH_CAB;
		chan[i].status = IS_DISCONNECTED;
		chan[i].status = IS_CONNECTED;
	}
	for(i=7; i<=7; ++i) {
		chan[i].type = CH_FX;
		chan[i].status = IS_DISCONNECTED;
		chan[i].status = IS_CONNECTED;
	}
}

void refresh_channels() {
	output_state.relays = 0;
	for(int i=0; i<NUM_CHANNELS; ++i) {
		switch(chan[i].status) {
			case IS_SELECTED:
				set_chan_led(i, LED_RED);
				output_state.relays |= ((unsigned long)1)<<i;
				break;
			case IS_CONNECTED:
				set_chan_led(i, LED_GREEN);
				break;
			default:
	 			set_chan_led(i, LED_OFF);
				break;
		}
	}
	output_state.pending = 1;
}

void select_channel(byte which) {
	switch(chan[which].status)
	{ 
		case IS_DISCONNECTED:
			return;
		case IS_SELECTED:
			chan[which].status = IS_CONNECTED;
			break;
		case IS_CONNECTED:
			for(int i=0; i<NUM_CHANNELS; ++i) {
				if(chan[i].status == IS_SELECTED && chan[i].type == chan[which].type) {
					chan[i].status = IS_CONNECTED;
				}	
			}
			chan[which].status = IS_SELECTED;					
			break;
	}
	refresh_channels();
}

void on_key_press(byte i) {
	switch(i) {
		case K_CHAN1: select_channel(0); break;
		case K_CHAN2: select_channel(1); break;
		case K_CHAN3: select_channel(2); break;
		case K_CHAN4: select_channel(3); break;
		case K_CHAN5: select_channel(4); break;
		case K_CHAN6: select_channel(5); break;
		case K_CHAN7: select_channel(6); break;
		case K_CHAN8: select_channel(7); break;
//		case K_BUTTON1: digit[0] = CHAR_A; break;
//		case K_BUTTON2: digit[0] = CHAR_B; break;
//		case K_BUTTON3: digit[0] = CHAR_C; break;
	}
}
byte cable_map[NUM_CHANNELS] = {7,6,5,4,3,2,1,0};

void on_cable_connect(byte i) {
	chan[cable_map[i]].status = IS_CONNECTED;
	refresh_channels();
}
void on_cable_disconnect(byte i) {
	chan[cable_map[i]].status = IS_DISCONNECTED;
	refresh_channels();
}



////////////////////////////////////////////////////////////
// MAIN
void main()
{ 
	
	// osc control / 16MHz / internal
	osccon = 0b01111010;
	
	// configure io
	trisa = MASK_TRISA;              	
	trisb = MASK_TRISB;              	
    trisc = MASK_TRISC;              
	ansela = 0b00000000;
	anselb = 0b00000000;
	anselc = 0b00000000;
	porta=0;
	portb=0;
	portc=0;

	option_reg.7 = 0;
	wpua.3=1;
	wpua.4=1;
	wpua.5=1;

	// turn off all the display source transistors
	P_DIGIT1 = 1;
	P_DIGIT2 = 1;
	P_DIGIT3 = 1;
	ui_state = UI_DIGIT1;

/*	// Configure timer 1 (controls tempo)
	// Input 4MHz
	// Prescaled to 500KHz
	tmr1l = 0;	 // reset timer count register
	tmr1h = 0;
	t1con.7 = 0; // } Fosc/4 rate
	t1con.6 = 0; // }
	t1con.5 = 1; // } 1:8 prescale
	t1con.4 = 1; // }
	t1con.0 = 1; // timer 1 on
	pie1.0 = 1;  // timer 1 interrupt enable
*/	
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 4MHz
	// 	prescaled 1/16 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	
	option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	option_reg.3 = 0; // timer 0 is prescaled
	option_reg.2 = 0; // }
	option_reg.1 = 1; // } 1/16 prescaler
	option_reg.0 = 1; // }
	intcon.5 = 1; 	  // enabled timer 0 interrrupt
	intcon.2 = 0;     // clear interrupt fired flag
	
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE

	// App loop
	P_SRO_EN = 0; // enable shift registers
	
	output_state.digit[0] = CHAR_C;
	output_state.digit[1] = CHAR_H;
	output_state.digit[2] = CHAR_1;
	output_state.pending = 1;
	

	unsigned long key_state = 0;
	unsigned long cd_state = 0;
	unsigned long last_key_state = 0;
	unsigned long last_cd_state = 0;
	byte key_debounce = 0;
	byte cd_debounce = 0;

	init_channels();
	refresh_channels();

	panel_input.pending = 0;	
	for(;;)
	{	
		// grab input status from the 
		if(panel_input.pending) {
			key_state = panel_input.key_state;
			cd_state = panel_input.cd_state;
			panel_input.pending = 0;
		}

		if(ms_tick)
		{
			ms_tick = 0;
			
			if(key_debounce) {
				--key_debounce;
			}
			else {
 
				unsigned long key_press = key_state & ~last_key_state;
				if(key_press) {
					unsigned long mask = 0x01;
					for(int i=0; i<20; ++i) {					
						if(mask & key_press) {
							on_key_press(i);
						}
						mask<<=1;
					}
					key_debounce = KEY_DEBOUNCE_MS;
				}
				last_key_state = key_state;
			}

			if(cd_debounce) {
				--cd_debounce;
			}
			else {
				unsigned int cd_change = cd_state ^ last_cd_state;
				if(cd_change) {
					unsigned int mask = 0x01;
					for(int i=0; i<NUM_CHANNELS; ++i) {					
						if(mask & cd_change) {
							if(mask & cd_state) {
								on_cable_connect(i);
							}
							else {
								on_cable_disconnect(i);
							}
						}
						mask<<=1;
					}
					cd_debounce = CABLE_DETECT_DEBOUNCE_MS;
				}
				last_cd_state = cd_state;
			}
		}			
	}
}