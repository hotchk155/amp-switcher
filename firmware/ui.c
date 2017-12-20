/////////////////////////////////////////////////////////////////////
// USER INTERFACE
#include <system.h>
#include "amp-switcher.h"
#include "chars.h"

#define SHOW_PATCH_MS 	3000
#define KB_REPEAT_DELAY_MS 	1000
#define KB_REPEAT_INTERVAL_MS 	30

enum {
	UI_IDLE,
	UI_SEL_LOAD,
	UI_SEL_SAVE,
	UI_SEL_PCNO
};

byte ui_pending = 0;
byte ui_digit[3] = {0};
byte ui_chan_leds[2] = {0};
byte ui_blink = 0;
byte ui_bank = 0;
byte ui_patch = 0;
byte ui_patch_char = 0;
byte ui_mode = UI_IDLE;
byte ui_data_val = 0;


int ui_show_patch = 0;
int ui_kb_repeat = 0;
byte ui_last_key = 0;

byte ui_bank_char(byte which) {
	switch(which) {
		case 0: return CHAR_A; 
		case 1: return CHAR_B; 
		case 2: return CHAR_C;	
		case 3: return CHAR_D;
	}
}
byte ui_digit(byte which) {
	switch(which) {
		case 0: return CHAR_0; 
		case 1: return CHAR_1; 
		case 2: return CHAR_2;	
		case 3: return CHAR_3;
		case 4: return CHAR_4;
		case 5: return CHAR_5;
		case 6: return CHAR_6;
		case 7: return CHAR_7;
		case 8: return CHAR_8;
		case 9: return CHAR_9;
	}
}

void ui_chan_led(byte which, byte state) {
	which = 7-which;
	byte shift = (which%4)*2;
	if(which < 4) {
		ui_chan_leds[0] &= ~(LED_MASK<<shift);
		ui_chan_leds[0]  |= (state<<shift);		
	}
	else {
		which %= 4;
		ui_chan_leds[1] &= ~(LED_MASK<<shift);
		ui_chan_leds[1]  |= (state<<shift);		
	}
	ui_pending = 1;
}

/////////////////////////////////////////////////////
// SAVE OR LOAD A PATCH 
// (depends on the current mode and references the 
// selected bank)
void ui_patch_click(byte which) {
	ui_patch = which;
	switch(ui_mode) {
		case UI_SEL_LOAD:
			ui_patch_char = CHAR_L;
			ui_show_patch = SHOW_PATCH_MS;
			ui_mode = UI_IDLE;
			break;
		case UI_SEL_SAVE:
			ui_mode = UI_SEL_PCNO;		
			break;
	}
}

void ui_key_press_IDLE(byte i, byte press) {
	if(press) {
		switch(i) {
			case K_CHAN1: chan_click(0); break;
			case K_CHAN2: chan_click(1); break;
			case K_CHAN3: chan_click(2); break;
			case K_CHAN4: chan_click(3); break;
			case K_CHAN5: chan_click(4); break;
			case K_CHAN6: chan_click(5); break;
			case K_CHAN7: chan_click(6); break;
			case K_CHAN8: chan_click(7); break;
			case K_BUTTON1: ui_mode = UI_SEL_LOAD; break;
		}
	}
}

void ui_key_press_SEL_PGM(byte i, byte press) {
	if(press) {
		
		switch(i) {
			case K_CHAN1: ui_patch_click(0); break;
			case K_CHAN2: ui_patch_click(1); break;
			case K_CHAN3: ui_patch_click(2); break;
			case K_CHAN4: ui_patch_click(3); break;
			case K_CHAN5: ui_patch_click(4); break;
			case K_CHAN6: ui_patch_click(5); break;
			case K_CHAN7: ui_patch_click(6); break;
			case K_CHAN8: ui_patch_click(7); break;
			case K_BUTTON2: 
				ui_mode = UI_SEL_SAVE;
				break;
			case K_BUTTON3: 
				if(++ui_bank >= NUM_PATCH_BANKS) {
					ui_bank = 0;
				}
				break;
		}
	}
	else {
		switch(i) {
			case K_BUTTON1: 
				ui_mode = UI_IDLE;
				break;
		}
	}
}

void ui_key_press_SEL_PCNO(byte i, byte press) {
	if(press) {		
		switch(i) {
			case K_BUTTON1: 
				ui_patch_char = CHAR_S;
				ui_show_patch = SHOW_PATCH_MS;
				ui_mode = UI_IDLE;
				break;
			case K_BUTTON2: 
				if(ui_data_val > 0) {
					--ui_data_val;
				}
				break;
			case K_BUTTON3: 
				if(ui_data_val < 127 ) {
					++ui_data_val;
				}
				break;
		}
	}
}

void ui_key_press(byte i, byte press) {
	if(press) {
		ui_last_key = i;
		ui_kb_repeat = KB_REPEAT_DELAY_MS;
	}
	else {
		ui_kb_repeat = 0;
	}
	switch(ui_mode) {
		case UI_IDLE:
			ui_key_press_IDLE(i, press);
			break;
		case UI_SEL_LOAD:
		case UI_SEL_SAVE:
			ui_key_press_SEL_PGM(i, press);
			break;
		case UI_SEL_PCNO:
			ui_key_press_SEL_PCNO(i, press);
			break;
	}
}


void ui_run() {
	byte d;
	ui_blink++;
	if(ui_show_patch) {
		--ui_show_patch;
	}
	
	if(ui_kb_repeat) {
		if(!--ui_kb_repeat) {
			switch(ui_mode) {
				case UI_SEL_PCNO:
					ui_key_press_SEL_PCNO(ui_last_key, 1);
			}		
			ui_kb_repeat = KB_REPEAT_INTERVAL_MS;
		}
	}
	
	
	// 7SEG
	switch(ui_mode) {
		case UI_IDLE:
			if(ui_show_patch) {
				output_state.digit[0] = ui_patch_char|SEG_DP;
				if(ui_blink & 0xc0) {				
					output_state.digit[1] = ui_bank_char(ui_bank);
					output_state.digit[2] = ui_digit(1 + ui_patch);
				}
				else {
					output_state.digit[1] = 0;
					output_state.digit[2] = 0;
				}
			}
			else {
				output_state.digit[0] = SEG_DP;
				output_state.digit[1] = SEG_DP;
				output_state.digit[2] = SEG_DP;
			}
			break;
		case UI_SEL_LOAD:
			output_state.digit[0] = CHAR_L|SEG_DP;
			output_state.digit[1] = ui_bank_char(ui_bank);
			output_state.digit[2] = SEG_G;
			break;
		case UI_SEL_SAVE:
			output_state.digit[0] = CHAR_S|SEG_DP;
			output_state.digit[1] = ui_bank_char(ui_bank);
			output_state.digit[2] = SEG_G;
			break;
		case UI_SEL_PCNO:		
			d = ui_data_val;
			output_state.digit[0] = ui_digit(d/100);
			d %= 100;
			output_state.digit[1] = ui_digit(d/10);
			d %= 10;
			output_state.digit[2] = ui_digit(d);
			break;
	}
		
	// LEDS
	switch(ui_mode) {
		case UI_IDLE:
			output_state.chan_leds[0] = ui_chan_leds[0];
			output_state.chan_leds[1] = ui_chan_leds[1];
			break;
		case UI_SEL_LOAD:
			if(ui_blink&0x80) {
				output_state.chan_leds[0] = 0b01010101;
				output_state.chan_leds[1] = 0b01010101;			
			}
			else {
				output_state.chan_leds[0] = 0;
				output_state.chan_leds[1] = 0;						
			}
			break;
		case UI_SEL_SAVE:
			if(ui_blink&0x80) {
				output_state.chan_leds[0] = 0b01010101;
				output_state.chan_leds[1] = 0b01010101;			
			}
			else {
				output_state.chan_leds[0] = 0b10101010;
				output_state.chan_leds[1] = 0b10101010;			
			}
			break;
		case UI_SEL_PCNO:
			output_state.chan_leds[0] = 0;
			output_state.chan_leds[1] = 0;						
			break;
	}
	output_state.pending = 1;
	
	
	
}

