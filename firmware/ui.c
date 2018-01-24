/////////////////////////////////////////////////////////////////////
// USER INTERFACE
#include <system.h>
#include <memory.h>
#include "amp-switcher.h"
#include "chars.h"

#define CHANNEL_LEDS_FLASH_CYCLE 400
#define CHANNEL_LEDS_FLASH_OFF 100

#define KB_REPEAT_DELAY_MS 	1000
#define KB_REPEAT_INTERVAL_MS 	30
#define KB_CHAN_ERROR_MS 	1000

#define ALL_RED_LEDS   (unsigned long)0b1010101010101010
#define ALL_GREEN_LEDS (unsigned long)0b0101010101010101
#define UI_LONG_PRESS 2000
//#define UI_MESSAGE_DELAY	1000
#define UI_MESSAGE_CYCLES	3

enum {
	UI_NONE = 0,
	UI_IDLE,
	UI_SEL_LOAD,
	UI_SEL_SAVE,
	UI_SEL_CHAN,
	UI_SEL_PCNO,
};

enum {
	EVENT_NONE = 0,
	EVENT_INIT,
	EVENT_TICK,
	EVENT_PRESS,
	EVENT_RELEASE,
	EVENT_AUTOREPEAT
};	

byte ui_digit[3] = {0};


byte ui_chan_leds[2];
byte ui_cur_patch;

byte ui_numeric;

int ui_press_counter = 0;

byte ui_mode = UI_IDLE;

int ui_error_timeout = 0;
int ui_error_count = 0;

int ui_kb_repeat = 0;
byte ui_last_key = 0;


byte ui_msg1[3];
byte ui_msg2[3];

int ui_message_cycles = 0;
int ui_message_timer = 0;

void ui_display_msg(byte count, byte c1, byte c2, byte c3, byte c4, byte c5, byte c6) {
	ui_msg1[0] = c1;
	ui_msg1[1] = c2;
	ui_msg1[2] = c3;
	ui_msg2[0] = c4;
	ui_msg2[1] = c5;
	ui_msg2[2] = c6;
	ui_message_timer = 0;
	ui_message_cycles = count;
}

/////////////////////////////////////////////////////////////
// translate digit 0-9 to 7 segment character
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

/////////////////////////////////////////////////////////////
// Translate channel button ID to channel number
int ui_key_order(byte which)
{
	switch(which) {
		case K_CHAN1: return 0;
		case K_CHAN2: return 1;
		case K_CHAN3: return 2;
		case K_CHAN4: return 3;
		case K_CHAN5: return 4;
		case K_CHAN6: return 5;
		case K_CHAN7: return 6;
		case K_CHAN8: return 7;
		default: return -1;
	}
}

/////////////////////////////////////////////////////////////
// Run IDLE mode
int ui_run_IDLE(byte event, byte param) {
	int chan;
	switch(event) {
		case EVENT_TICK:
			++ui_message_timer;
			break;
		case EVENT_PRESS:
			switch(param) {
				case K_SEL:
					// press to the SEL button changes immediately
					// to LOAD mode
					ui_message_cycles = 0;
					return UI_SEL_LOAD;
				default:
					chan = ui_key_order(param);
					if(chan>=0) {
						chan_event(chan, CHAN_CLICK);
					}
					break;
			}
			break;
	}
	// no override of the default channel LED status	
	output_state.chan_leds[0] = ui_chan_leds[0];
	output_state.chan_leds[1] = ui_chan_leds[1];
	
	if(ui_message_cycles) {
		if(ui_message_timer > 800) {
			ui_message_timer = 0;
			--ui_message_cycles;
		}
		else if(ui_message_timer > 700) {
			output_state.digit[0] = 0;
			output_state.digit[1] = 0;		
			output_state.digit[2] = 0;
		}
		else if(ui_message_timer > 400) {
			output_state.digit[0] = ui_msg2[0];
			output_state.digit[1] = ui_msg2[1];		
			output_state.digit[2] = ui_msg2[2];
		}
		else if(ui_message_timer > 300) {
			output_state.digit[0] = 0;
			output_state.digit[1] = 0;		
			output_state.digit[2] = 0;
		}
		else {
			output_state.digit[0] = ui_msg1[0];
			output_state.digit[1] = ui_msg1[1];		
			output_state.digit[2] = ui_msg1[2];
		}		
	}
	else {
		output_state.digit[0] = SEG_DP;
		output_state.digit[1] = SEG_DP;		
		output_state.digit[2] = SEG_DP;
	}
	
	
	output_state.pending = 1;
	return UI_NONE;
}

/////////////////////////////////////////////////////////////
// Run LOAD mode
int ui_run_SEL_LOAD(byte event, byte param) {
	int patch;
	switch(event) {
		case EVENT_INIT:
			output_state.digit[0] = CHAR_L;
			output_state.digit[1] = CHAR_O;
			output_state.digit[2] = CHAR_D;
			ui_press_counter = 0;
			ui_message_timer = 0;
			break;						
		case EVENT_TICK:
			if(++ui_message_timer > CHANNEL_LEDS_FLASH_CYCLE) {
				ui_message_timer = 0;
			}
			if(++ui_press_counter >= UI_LONG_PRESS) {
				// when SEL button has been pressed for long time
				// we change to SAVE mode
				return UI_SEL_SAVE;
			}
			break;
		case EVENT_PRESS:
			if(param == K_MINUS || param == K_PLUS) {
				return UI_SEL_CHAN;
			}
			else {		
				patch = ui_key_order(param);
				if(patch>=0) {
					storage_load_patch(patch);
					ui_display_msg(3,CHAR_P, ui_digit(patch+1), 0, 0, 0, 0);
					chan_event(0,CHAN_INIT);
					return UI_IDLE;
				}
			}
			break;
		case EVENT_RELEASE:
			if(param == K_SEL) {
				// release the SEL button before timeout
				// just returns to IDLE mode
				return UI_IDLE;
			}
			break;
	}
	if(ui_message_timer > CHANNEL_LEDS_FLASH_OFF) {
		output_state.chan_leds[0] = ALL_GREEN_LEDS;
		output_state.chan_leds[1] = ALL_GREEN_LEDS;
	}
	else {
		output_state.chan_leds[0] = 0;
		output_state.chan_leds[1] = 0;
	}
	output_state.pending = 1;
	return UI_NONE;
}

/////////////////////////////////////////////////////////////
// Run SAVE mode
int ui_run_SEL_SAVE(byte event, byte param) {
	int patch;
	switch(event) {
		case EVENT_INIT:
			output_state.digit[0] = CHAR_S;
			output_state.digit[1] = CHAR_A;
			output_state.digit[2] = CHAR_V;
			break;
		case EVENT_TICK:
			if(++ui_message_timer > CHANNEL_LEDS_FLASH_CYCLE) {
				ui_message_timer = 0;
			}
			break;
		case EVENT_PRESS:
			patch = ui_key_order(param);
			if(patch>=0) {
				ui_cur_patch = patch;
				return UI_SEL_PCNO;
			}
			break;
		case EVENT_RELEASE:
			if(param == K_SEL) {
				// release SEL button without saving
				return UI_IDLE;
			}
			break;
	}
	if(ui_message_timer > CHANNEL_LEDS_FLASH_OFF) {
		output_state.chan_leds[0] = ALL_RED_LEDS;
		output_state.chan_leds[1] = ALL_RED_LEDS;
	}
	else {
		output_state.chan_leds[0] = 0;
		output_state.chan_leds[1] = 0;
	}
	output_state.pending = 1;
	return UI_NONE;
}

/////////////////////////////////////////////////////////////
// Run SEL PCNO mode
int ui_run_SEL_PCNO(byte event, byte param) {
	byte n;
	switch(event) {
		case EVENT_INIT:
			ui_numeric = g_patch[ui_cur_patch].pc_no;
			break;
		case EVENT_AUTOREPEAT:
			if(param != K_MINUS && param != K_PLUS) {
				break;
			}
			// fall thru
		case EVENT_PRESS:
			if(param == K_MINUS && ui_numeric > 0) {
				--ui_numeric;
			} 
			else if(param == K_PLUS && ui_numeric < 127) {
				++ui_numeric;
			} 
			break;
		case EVENT_RELEASE:
			if(param == K_SEL) {
				g_status.pc_no = ui_numeric;
				memcpy(&g_patch[ui_cur_patch], &g_status, sizeof(DEVICE_STATUS));
				storage_save_patch(ui_cur_patch);
				ui_display_msg(3,CHAR_P, ui_digit(ui_cur_patch+1), 0, CHAR_S, CHAR_A, CHAR_V);
				return UI_IDLE;
			}
			break;
	}
	n = ui_numeric;
	output_state.digit[0] = ui_digit(n/100);
	n%=100;
	output_state.digit[1] = ui_digit(n/10);
	output_state.digit[2] = ui_digit(n%10);	
	output_state.chan_leds[0] = 0;
	output_state.chan_leds[1] = 0;
	output_state.pending = 1;
	return UI_NONE;
}

/////////////////////////////////////////////////////////////
// Run SEL CHAN mode
int ui_run_SEL_CHAN(byte event, byte param) {
	switch(event) {
		case EVENT_INIT:
			output_state.digit[0] = CHAR_C;
			output_state.digit[1] = CHAR_H;
			output_state.digit[2] = CHAR_N;
			ui_numeric = 0;
			break;
		case EVENT_AUTOREPEAT:
			if(param != K_MINUS && param != K_PLUS) {
				break;
			}
			// fall thru
		case EVENT_PRESS:
			if(!ui_numeric) {
				ui_numeric = g_config.midi_chan + 1;
			}
			else if(param == K_MINUS && ui_numeric > 1) {
				--ui_numeric;
			} 
			else if(param == K_PLUS && ui_numeric < 16) {
				++ui_numeric;
			} 
			output_state.digit[0] = 0;
			output_state.digit[1] = ui_digit(ui_numeric/10);
			output_state.digit[2] = ui_digit(ui_numeric%10);
			break;
		case EVENT_RELEASE:
			if(param == K_SEL) {
				if(ui_numeric) {
					// save the channel
					g_config.midi_chan = ui_numeric - 1;
					storage_save_config();
					ui_display_msg(3,CHAR_C, CHAR_H, 0, CHAR_S, CHAR_E, CHAR_T);
				}
				// release SEL button without saving
				return UI_IDLE;
			}
			break;
	}
	output_state.chan_leds[0] = 0;
	output_state.chan_leds[1] = 0;
	output_state.pending = 1;
	return UI_NONE;
}


/////////////////////////////////////////////////////////////////////
// helper to invoke the correct event handler depending on the 
// current UI mode
int ui_event_call(byte event, byte param) {	
	switch(ui_mode) {
	case UI_IDLE:
		return ui_run_IDLE(event, param);
	case UI_SEL_LOAD:		
		return ui_run_SEL_LOAD(event, param);
	case UI_SEL_SAVE:
		return ui_run_SEL_SAVE(event, param);
	case UI_SEL_CHAN:
		return ui_run_SEL_CHAN(event, param);
	case UI_SEL_PCNO:
		return ui_run_SEL_PCNO(event, param);
	}
	return UI_NONE;
}

/////////////////////////////////////////////////////////////////////
// invoke event handler and deal with state transitions
void ui_handle_event(byte event, byte param) {	
	int next_state = ui_event_call(event, param);
	while(next_state != UI_NONE) {
		ui_mode = next_state;
		next_state = ui_event_call(EVENT_INIT, 0);
	}
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// PUBLIC INTERFACE
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Called at startup
void ui_unit() {
	ui_mode = UI_IDLE;
	ui_handle_event(EVENT_INIT, 0);
}

/////////////////////////////////////////////////////////////////////
// Called when a button is pressed or released
void ui_key_press(byte key, byte press) {
	if(press) {
		ui_last_key = key;
		ui_kb_repeat = KB_REPEAT_DELAY_MS;
		ui_handle_event(EVENT_PRESS, key);
	}
	else {
		ui_last_key = 0;
		ui_kb_repeat = 0;
		ui_handle_event(EVENT_RELEASE, key);
	}
}

/////////////////////////////////////////////////////////////
// Called to set the "idle" state of a given channel 
// LED indicator
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
}



/////////////////////////////////////////////////////////////////////
// Called once per millisecond
void ui_run() {	
	if(ui_kb_repeat) {
		if(!--ui_kb_repeat) {
			ui_kb_repeat = KB_REPEAT_INTERVAL_MS;
			ui_handle_event(EVENT_AUTOREPEAT, ui_last_key);
		}
	}
	ui_handle_event(EVENT_TICK, 0);
}


