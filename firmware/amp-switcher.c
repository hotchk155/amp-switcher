//
// HEADER FILES
//
#include <system.h>
#include <memory.h>
#include "amp-switcher.h"
#include "chars.h"


// PIC CONFIG BITS
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000


//
// MACRO DEFS
//
// MIDI message bytes
#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc
#define MIDI_SYSEX_BEGIN     	0xf0
#define MIDI_SYSEX_END     		0xf7
#define MIDI_CC_NRPN_HI 		99
#define MIDI_CC_NRPN_LO 		98
#define MIDI_CC_DATA_HI 		6
#define MIDI_CC_DATA_LO 		38

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




#define K_CHAN1 0
#define K_CHAN2 7
#define K_CHAN3 1
#define K_CHAN4 6
#define K_CHAN5 5
#define K_CHAN6 3
#define K_CHAN7 4
#define K_CHAN8 2

#define K_BUTTON1	16
#define K_BUTTON2	17
#define K_BUTTON3	18

#define K_MAX_BITS	19 // the key status registers are 32-bit but how many in use?



//
// TYPE DEFS
//
//
// GLOBAL DATA
//


// timer stuff
volatile byte ms_tick = 0;
volatile unsigned int timer_init_scalar = 0;

// define the buffer used to receive MIDI input
#define SZ_RXBUFFER 			64		// size of MIDI receive buffer (power of 2)
#define SZ_RXBUFFER_MASK 		0x3F	// mask to keep an index within range of buffer
volatile byte rx_buffer[SZ_RXBUFFER];	// the MIDI receive buffer
volatile byte rx_head = 0;				// buffer data insertion index
volatile byte rx_tail = 0;				// buffer data retrieval index

// State flags used while receiving MIDI data
byte midi_status = 0;					// current MIDI message status (running status)
byte midi_num_params = 0;				// number of parameters needed by current MIDI message
byte midi_params[2];					// parameter values of current MIDI message
char midi_param = 0;					// number of params currently received
byte midi_ticks = 0;					// number of MIDI clock ticks received
byte sysex_state = SYSEX_NONE;			// whether we are currently inside a sysex block



volatile byte ui_state;

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
// INTERRUPT HANDLER 
void interrupt( void )
{
	
	/////////////////////////////////////////////////////////////
	// TIMER 0 INTERRUPT
	
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
				
				P_SRO_EN = 0; // enable shift registers
				
				ui_state = UI_DIGIT1;
				break;			
		}	
	}
		
	/////////////////////////////////////////////////////
	// UART RECEIVE
	if(pir1.5)
	{	
		byte b = rcreg;
		byte next_head = (rx_head + 1)&SZ_RXBUFFER_MASK;
		if(next_head != rx_tail) {
			rx_buffer[rx_head] = b;
			rx_head = next_head;
		}
		pir1.5 = 0;
	}		
}


////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void init_usart()
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

////////////////////////////////////////////////////////////
// GET MESSAGES FROM MIDI INPUT
byte midi_in()
{
	// loop until there is no more data or
	// we receive a full message
	for(;;)
	{
		// usart buffer overrun error?
		if(rcsta.1)
		{
			rcsta.4 = 0;
			rcsta.4 = 1;
		}
		
		// check for empty receive buffer
		if(rx_head == rx_tail)
			return 0;
		
		// read the character out of buffer
		byte ch = rx_buffer[rx_tail];
		++rx_tail;
		rx_tail&=SZ_RXBUFFER_MASK;

		// REALTIME MESSAGE
		if((ch & 0xf0) == 0xf0)
		{
			switch(ch)
			{
			// REALTIME MESSAGES
			case MIDI_SYNCH_TICK:
			case MIDI_SYNCH_START:
			case MIDI_SYNCH_CONTINUE:
			case MIDI_SYNCH_STOP:
				return ch;		
/*				
			// START OF SYSEX	
			case MIDI_SYSEX_BEGIN:
				sysex_state = SYSEX_ID0; 
				break;
			// END OF SYSEX	
			case MIDI_SYSEX_END:
				switch(sysex_state) {
				case SYSEX_IGNORE: // we're ignoring a syex block
				case SYSEX_NONE: // we weren't even in sysex mode!					
					break;			
				case SYSEX_PARAMH:	// the state we'd expect to end in
					P_LED1 = 1; 
					P_LED2 = 1; 
					delay_ms(250); 
					delay_ms(250); 
					delay_ms(250); 
					delay_ms(250); 
					P_LED1 = 0; 
					P_LED2 = 0; 
					storage_write_patch();	// store to EEPROM 
					all_reset();
					break;
				default:	// any other state would imply bad sysex data
					P_LED1 = 0; 
					for(char i=0; i<10; ++i) {
						P_LED2 = 1; 
						delay_ms(100);
						P_LED2 = 0; 
						delay_ms(100);
					}
					all_reset();
					break;
				}
				sysex_state = SYSEX_NONE; 
				break;
*/				
			}
		}    
		// STATUS BYTE
		else if(!!(ch & 0x80))
		{
			// a status byte cancels sysex state
			sysex_state = SYSEX_NONE;
		
			midi_param = 0;
			midi_status = ch; 
			switch(ch & 0xF0)
			{
			case 0xA0: //  Aftertouch  1  key  touch  
			case 0xC0: //  Patch change  1  instrument #   
			case 0xD0: //  Channel Pressure  1  pressure  
				midi_num_params = 1;
				break;    
			case 0x80: //  Note-off  2  key  velocity  
			case 0x90: //  Note-on  2  key  veolcity  
			case 0xB0: //  Continuous controller  2  controller #  controller value  
			case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
			default:
				midi_num_params = 2;
				break;        
			}
		}    
		else 
		{
/*		
			switch(sysex_state) // are we inside a sysex block?
			{
			// SYSEX MANUFACTURER ID
			case SYSEX_ID0: sysex_state = (ch == MY_SYSEX_ID0)? SYSEX_ID1 : SYSEX_IGNORE; break;
			case SYSEX_ID1: sysex_state = (ch == MY_SYSEX_ID1)? SYSEX_ID2 : SYSEX_IGNORE; break;
			case SYSEX_ID2: sysex_state = (ch == MY_SYSEX_ID2)? SYSEX_PARAMH : SYSEX_IGNORE; break;
			// CONFIG PARAM DELIVERED BY SYSEX
			case SYSEX_PARAMH: nrpn_hi = ch; ++sysex_state; break;
			case SYSEX_PARAML: nrpn_lo = ch; ++sysex_state;break;
			case SYSEX_VALUEH: nrpn_value_hi = ch; ++sysex_state;break;
			case SYSEX_VALUEL: nrpn(nrpn_hi, nrpn_lo, nrpn_value_hi, ch); sysex_state = SYSEX_PARAMH; break;
			case SYSEX_IGNORE: break;			
			// MIDI DATA
			case SYSEX_NONE: */
				if(midi_status)
				{
					// gathering parameters
					midi_params[midi_param++] = ch;
					if(midi_param >= midi_num_params)
					{
						// we have a complete message.. is it one we care about?
						midi_param = 0;
						switch(midi_status&0xF0)
						{
						case 0x80: // note off
						case 0x90: // note on
						case 0xE0: // pitch bend
						case 0xB0: // cc
						case 0xD0: // aftertouch
							return midi_status; 
						}
					}
				}
			//}
		}
	}
	// no message ready yet
	return 0;
}






void ui_chan_led(byte which, byte state) 
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



void on_key_press(byte i) {
	switch(i) {
		case K_CHAN1: chan_click(0); break;
		case K_CHAN2: chan_click(1); break;
		case K_CHAN3: chan_click(2); break;
		case K_CHAN4: chan_click(3); break;
		case K_CHAN5: chan_click(4); break;
		case K_CHAN6: chan_click(5); break;
		case K_CHAN7: chan_click(6); break;
		case K_CHAN8: chan_click(7); break;
//		case K_BUTTON1: digit[0] = CHAR_A; break;
//		case K_BUTTON2: digit[0] = CHAR_B; break;
//		case K_BUTTON3: digit[0] = CHAR_C; break;
	}
}


typedef struct {
	byte midi_chan;
	byte amp_cc;
	byte cab_cc;
	byte fx_cc[NUM_FX_CHANNELS];
} DEVICE_CONFIG;
DEVICE_CONFIG config;

void init_config() {
	config.midi_chan = DEFAULT_MIDI_CHAN;
	config.amp_cc = DEFAULT_AMP_CC;
	config.cab_cc = DEFAULT_CAB_CC;
	config.fx_cc[0] = DEFAULT_FX0_CC;
	config.fx_cc[1] = DEFAULT_FX1_CC;
}





////////////////////////////////////////////////////////////
// HANDLE MIDI NOTE
void on_midi_note(byte chan, byte note, byte vel) 
{
	if(chan == config.midi_chan) {
		if(note >= NOTE_AMP_BASE && note < NOTE_AMP_MAX) {
			if(vel) {
				chan_select(AMP_BASE + note - NOTE_AMP_BASE);
			}
		}
		else if(note >= NOTE_CAB_BASE && note < NOTE_CAB_MAX) {
			if(vel) {
				chan_select(CAB_BASE + note - NOTE_CAB_BASE);
			}
		}
		else if(note >= NOTE_FX_BASE && note < NOTE_FX_MAX) {
			if(vel) {
				chan_select(FX_BASE + note - NOTE_FX_BASE);
			}
			else {
				chan_deselect(FX_BASE + note - NOTE_FX_BASE);
			}
		}
	}	
}

////////////////////////////////////////////////////////////
// HANDLE MIDI CC MESSAGE
void on_midi_cc(byte chan, byte cc, byte value) 
{
	if(chan == config.midi_chan) {
		if(cc == config.amp_cc && value < NUM_AMP_CHANNELS) {
			chan_select(value + AMP_BASE);
		}
		if(cc == config.cab_cc && value < NUM_CAB_CHANNELS) {
			chan_select(value + CAB_BASE);
		}
		for(int i=0; i < NUM_FX_CHANNELS; ++i) {
			if(cc == config.fx_cc[i]) {
				if(value) {
					chan_select(i + FX_BASE);
				}
				else {
					chan_deselect(i + FX_BASE);
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////
// HANDLE MIDI PROGRAM CHANGE
void on_midi_pgm_change(byte chan, byte pgm) 
{
}


////////////////////////////////////////////////////////////
// MAIN
void main()
{ 
	
	// osc control / 16MHz / internal
	osccon = 0b01111010;
	
	apfcon0.7 = 1; // RX pin is RC5
	apfcon0.2 = 1; // TX pin is RC4
	
	// configure io
	porta  = 0b00000000;
	portb  = 0b00100000; // shift regs disabled
	portc  = 0b00000000;
	trisa = MASK_TRISA;              	
	trisb = MASK_TRISB;              	
    trisc = MASK_TRISC;              
	ansela = 0b00000000;
	anselb = 0b00000000;
	anselc = 0b00000000;


	option_reg.7 = 0;
	wpua.3=1;
	wpua.4=1;
	wpua.5=1;

	// turn off all the display source transistors
	P_DIGIT1 = 1;
	P_DIGIT2 = 1;
	P_DIGIT3 = 1;
	ui_state = UI_DIGIT1;

	memset(&output_state, 0, sizeof(output_state));
	output_state.digit[0] = CHAR_C;
	output_state.digit[1] = CHAR_H;
	output_state.digit[2] = CHAR_1;
	output_state.pending = 1;

	panel_input.pending = 0;	


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

	

	unsigned long key_state = 0;
	unsigned long cd_state = 0;
	unsigned long last_key_state = 0;
	unsigned long last_cd_state = 0;
	byte key_debounce = 0;
	byte cd_debounce = 0;


	init_usart();
	init_config();
	chan_init();
	chan_update();

	for(;;)
	{	
	
		//////////////////////////////////////////////////////////////////////
		// PROCESS PANEL INPUT
		//////////////////////////////////////////////////////////////////////
	
		// If there are panel inputs waiting for us then grab them...
		if(panel_input.pending) {
			key_state = panel_input.key_state;
			cd_state = panel_input.cd_state;
			panel_input.pending = 0;
		}

		// once a millisecond we process panel input
		if(ms_tick)
		{
			ms_tick = 0;
			
			// First deal with input from buttons
			if(key_debounce) {
				// debouncing keys
				--key_debounce;
			}
			else {				
				// which keys have been pressed?
				unsigned long key_press = key_state & ~last_key_state;
				if(key_press) {
					unsigned long mask = 0x01;
					for(int i=0; i<K_MAX_BITS; ++i) {					
						if(mask & key_press) {
							// call keypress handler for each pressed key
							on_key_press(i);
						}
						mask<<=1;
					}
					// ignore key events for debounce period
					key_debounce = KEY_DEBOUNCE_MS;
				}
				last_key_state = key_state;
			}

			// Next deal with cable detect events
			if(cd_debounce) {
				// apply a debounce time
				--cd_debounce;
			}
			else {
				// which inputs have been connected or disconnected?
				unsigned int cd_change = cd_state ^ last_cd_state;
				if(cd_change) {
					unsigned int mask = 0x80;
					for(int i=0; i<NUM_CHANNELS; ++i) {					
						if(mask & cd_change) {
							if(mask & cd_state) {
								// cable connected 
								chan_connect(i);
							}
							else {
								// cable disconnected
								chan_disconnect(i);
							}
						}
						mask>>=1;
					}
					// ignore changes for debounce period
					cd_debounce = CABLE_DETECT_DEBOUNCE_MS;
				}
				last_cd_state = cd_state;
			}
		}			

		//////////////////////////////////////////////////////////////////////		
		// PROCESS MIDI INPUT
		//////////////////////////////////////////////////////////////////////
		// poll for incoming MIDI data
		byte msg = midi_in();		
		switch(msg & 0xF0) {
		
		case 0x80: // MIDI NOTE OFF
			on_midi_note(msg&0x0F, midi_params[0], 0);
			break;
		case 0x90: // MIDI NOTE ON
			on_midi_note(msg&0x0F, midi_params[0], midi_params[1]);
			break;		
		case 0xB0: // CONTINUOUS CONTROLLER
			on_midi_cc(msg&0x0F, midi_params[0], midi_params[1]);
			break;
		case 0xC0: // PROGRAM CHANGE
			on_midi_pgm_change(msg&0x0F, midi_params[0]);
			break;
		}
		
	}
}