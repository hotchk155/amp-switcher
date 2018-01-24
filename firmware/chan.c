/////////////////////////////////////////////////////////////////////
// CHANNEL STATE HANDLING

/*
Cannot select an amp without a cab being selected

*/

#include <system.h>
#include "amp-switcher.h"
#include "chars.h"



// Flag to say if a channel is connected
byte is_connected[NUM_CHANNELS];

DEVICE_STATUS g_patch[NUM_PATCHES];
DEVICE_STATUS g_status;

/////////////////////////////////////////////////////////////////////
void chan_init() {
	for(int i=0; i<NUM_CHANNELS; ++i) {
		is_connected[i] = 0;
	}
}

////////////////////////////////////////////////////////////
void chan_init_patch(int index) {
	g_patch[index].pc_no = index + 1;
	g_patch[index].amp_sel = NO_SELECTION;
	g_patch[index].cab_sel = NO_SELECTION;
	for(int i=0; i<NUM_FX_CHANNELS; ++i) {
		g_patch[index].fx_sel[i] = 0;
	}
}


/////////////////////////////////////////////////////////////////////
void chan_init_connected(byte which, byte status) {
	is_connected[which] = status;
}

/////////////////////////////////////////////////////////////////////
void chan_consolidate()
{		
	int i;
	
	byte error = 0;
	
	// is the selected cab now disconnected?
	if(g_status.cab_sel != NO_SELECTION && !is_connected[CAB_BASE + g_status.cab_sel]) {
		g_status.cab_sel = NO_SELECTION;
		ui_display_msg(CHAR_C, CHAR_A, CHAR_B, CHAR_O, CHAR_U, CHAR_T);
		error = 1;
	}
	
	// if there is no cab selection then select the first
	// connected cab if there is one 
	if(g_status.cab_sel == NO_SELECTION) {
		for(i=0; i<NUM_CAB_CHANNELS; ++i) {
			if(is_connected[CAB_BASE + i]) {
				g_status.cab_sel = i;
				break;
			}
		}
	}
	
	// is there still no cab selection (meaning that there 
	// are no cabs connected to the system
	if(g_status.cab_sel == NO_SELECTION) {	
		// is an amp selected
		if(g_status.amp_sel != NO_SELECTION) {
			g_status.amp_sel = NO_SELECTION;
			// amp deselected as there are no cabs
			ui_display_msg(CHAR_N, CHAR_O, 0, CHAR_C, CHAR_A, CHAR_B);
			error = 1;
		}
	}
	else {
	
		// if there an amp selected?
		if(g_status.amp_sel != NO_SELECTION) {
			// is the selected amp disconnected?
			if(!is_connected[AMP_BASE + g_status.amp_sel]) {
				g_status.amp_sel = NO_SELECTION;
				if(!error) {
					// amp deselected as it is no longer connected
					ui_display_msg(CHAR_A, CHAR_M, CHAR_P, CHAR_O, CHAR_U, CHAR_T);
					error = 1;
				}
			}
		}

		// no amps selected? 
		// select the first that is connected
		if(g_status.amp_sel == NO_SELECTION) {
			for(i=0; i<NUM_AMP_CHANNELS; ++i) {
				if(is_connected[AMP_BASE + i]) {
					g_status.amp_sel = i;
					break;
				}
			}
		}
		// still none connected 
		if(g_status.amp_sel == NO_SELECTION) {
			if(!error) {
				ui_display_msg(CHAR_N, CHAR_O, 0, CHAR_A, CHAR_M, CHAR_P);
				error = 1;
			}
		}
	}
		
	output_state.relays = 0;
	
	int chan=0;
	for(i=0; i<NUM_AMP_CHANNELS; ++i, ++chan) {
		if(is_connected[chan]) {
			if(i==g_status.amp_sel) {			
				ui_chan_led(chan, LED_RED);
				output_state.relays |= ((unsigned long)1)<<chan;
			}
			else {
				ui_chan_led(chan, LED_GREEN);
			}
		}
		else {
			ui_chan_led(chan, LED_OFF);
		}
	}
	for(i=0; i<NUM_CAB_CHANNELS; ++i, ++chan) {
		if(is_connected[chan]) {
			if(i==g_status.cab_sel) {			
				ui_chan_led(chan, LED_RED);
				output_state.relays |= ((unsigned long)1)<<chan;
			}
			else {
				ui_chan_led(chan, LED_GREEN);
			}
		}
		else {
			ui_chan_led(chan, LED_OFF);
		}
	}
	for(i=0; i<NUM_FX_CHANNELS; ++i, ++chan) {
		if(is_connected[chan]) {
			if(g_status.fx_sel[i]) {			
				ui_chan_led(chan, LED_RED);
				output_state.relays |= ((unsigned long)1)<<chan;
			}
			else {
				ui_chan_led(chan, LED_GREEN);
			}
		}
		else {
			ui_chan_led(chan, LED_OFF);
		}
	}
	output_state.pending = 1;
	if(error) {
		blink_yellow(BLINK_MS_ERROR);
	}	
}

/////////////////////////////////////////////////////////////////////
// CHANNEL EVENT NOTIFICATION
// 
// CHAN_CLICK		Panel button click
// CHAN_SELECT		Selection of channel via MIDI etc
// CHAN_DESELECT	Deselection of channel via MIDI etc
// CHAN_CABLEIN		Connection of the cable for a channel
// CHAN_CABLEOUT	Disconnection of the cable for a channel
//
void chan_event(byte which, byte action) {

	if(CHAN_INIT == action) {
		// just drop thru to consolidate call
	}
	else if(CHAN_CABLEIN == action) {
		is_connected[which] = 1;
	}
	else if(CHAN_CABLEOUT == action) {
		is_connected[which] = 0;
	}
	else if(which >= AMP_BASE && which < AMP_MAX) {
		switch(action) 	{
		case CHAN_SELECT:
		case CHAN_CLICK:
			if(is_connected[which]) {
				g_status.amp_sel = which - AMP_BASE;
			}
//			else {
//				ui_display_msg(CHAR_A, CHAR_M, CHAR_P, CHAR_O, CHAR_U, CHAR_T);			
//				blink_yellow(BLINK_MS_ERROR);
//			}
			break;
		}
	}
	else if(which >= CAB_BASE && which < CAB_MAX) {
		switch(action) 	{
		case CHAN_SELECT:
		case CHAN_CLICK:
			if(is_connected[which]) {
				g_status.cab_sel = which - CAB_BASE;
			}			
//			else {
//				ui_display_msg(CHAR_C, CHAR_A, CHAR_B, CHAR_O, CHAR_U, CHAR_T);			
//				blink_yellow(BLINK_MS_ERROR);			
//			}
			break;
		}
	}
	else if(which >= FX_BASE && which < FX_MAX) {
		char fx_chan = which - FX_BASE;
		switch(action) 	{
		case CHAN_SELECT:
			if(is_connected[which]) {
				g_status.fx_sel[fx_chan] = 1;
			}			
//			else {
//				ui_display_msg(CHAR_F, CHAR_X, 0, CHAR_O, CHAR_U, CHAR_T);			
//				blink_yellow(BLINK_MS_ERROR);			
//			}
			break;
		case CHAN_CLICK:
			if(g_status.fx_sel[fx_chan]) {
				g_status.fx_sel[fx_chan] = 0;
			}
			else if(is_connected[which]) {
				g_status.fx_sel[fx_chan] = 1;
			}			
//			else {
//				ui_display_msg(CHAR_F, CHAR_X, 0, CHAR_O, CHAR_U, CHAR_T);			
//				blink_yellow(BLINK_MS_ERROR);						
//			}
			break;
		case CHAN_DESELECT:
			g_status.fx_sel[fx_chan] = 0;
			break;
		}
	}
	chan_consolidate();
}
