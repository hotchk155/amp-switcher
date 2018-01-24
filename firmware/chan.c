/////////////////////////////////////////////////////////////////////
// CHANNEL STATE HANDLING

/*
Cannot select an amp without a cab being selected

*/

#include <system.h>
#include "amp-switcher.h"


// Flag to say if a channel is connected
byte is_connected[NUM_CHANNELS];

DEVICE_STATUS g_status;

/////////////////////////////////////////////////////////////////////



void chan_init() {
	int i;
	for(i=0; i<NUM_CHANNELS; ++i) {
		is_connected[i] = 0;
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
	
	int error = 0;
	
	// is the selected cab now disconnected?
	if(g_status.cab_sel != NO_SELECTION && !is_connected[CAB_BASE + g_status.cab_sel]) {
		error = ERR_CAB_DISCONNECTED;
		g_status.cab_sel = NO_SELECTION;
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
			if(!error) {
				// amp deselected as there are no cabs
				error = ERR_NO_CABS;
			}
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
					error = ERR_AMP_DISCONNECTED;
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
				error = ERR_NO_AMPS;
			}
		}
	}

	if(error) {
		ui_error(error);
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
			break;
		case CHAN_CLICK:
			if(g_status.fx_sel[fx_chan]) {
				g_status.fx_sel[fx_chan] = 0;
			}
			else if(is_connected[which]) {
				g_status.fx_sel[fx_chan] = 1;
			}			
			break;
		case CHAN_DESELECT:
			g_status.fx_sel[fx_chan] = 0;
			break;
		}
	}
	chan_consolidate();
}







/*
/////////////////////////////////////////////////////////////////////
static int chan_first_connected(int from, int to) {
	for(int i=from; i<to; ++i) {
		if(chan[i].status > IS_DISCONNECTED) {
			return i;
		}			
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////
static int chan_first_selected(int from, int to) {
	for(int i=from; i<to; ++i) {
		if(chan[i].status == IS_SELECTED) {
			return i;
		}			
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////
// Push the channel state data to the hardware
static void chan_update() {
	output_state.relays = 0;
	for(int i=0; i<NUM_CHANNELS; ++i) {
		switch(chan[i].status) {
			case IS_SELECTED:
				ui_chan_led(i, LED_RED);
				output_state.relays |= ((unsigned long)1)<<i;
				break;
			case IS_CONNECTED:
				ui_chan_led(i, LED_GREEN);
				break;
			default:
	 			ui_chan_led(i, LED_OFF);
				break;
		}
	}
	output_state.pending = 1;
}

void chan_amp_consolidate() {

}


/////////////////////////////////////////////////////////////////////
static void chan_amp_select(byte which) 
{
	int i;
	
	// ensure that the requested amp is connected 
	if(chan[which].status < IS_CONNECTED) {
		ui_chan_error(which);
		return;
	}

	// deselect old channel if any
	int amp = chan_first_selected(AMP_BASE, AMP_MAX);
	if(amp >= 0) {
		chan[amp].status = IS_CONNECTED;
	}	
	
	// check a cab is selected
	int cab = chan_first_selected(CAB_BASE, CAB_MAX);
	if(cab < 0) {
		// no - any cab connected?
		cab = chan_first_connected(CAB_BASE, CAB_MAX);
		if(cab < 0) {
			// none connected!
			for(i=CAB_BASE;i<CAB_MAX;++i) {
				ui_chan_error(i);
			}			
			return;
		}		
		// select the cab
		chan[cab].status = IS_SELECTED;
	}

	
	// select new channel
	chan[which].status = IS_SELECTED;
}

/////////////////////////////////////////////////////////////////////
static void chan_amp_cable_in(byte which) {
	chan[which].status = IS_CONNECTED;
	// if this is the only connected amp then try to select it
	if(chan_first_selected(AMP_BASE, AMP_MAX) < 0) {
		chan_amp_select(which);
	}
}

/////////////////////////////////////////////////////////////////////
static void chan_amp_cable_out(byte which) {	
	if(chan[which].status == IS_SELECTED) {
		chan[which].status = IS_DISCONNECTED;
		int amp = chan_first_connected(AMP_BASE, AMP_MAX);
		if(amp >= 0) {
			chan_amp_select(amp);
		}	
	}
	else {
		chan[which].status = IS_DISCONNECTED;
	}	
}




/////////////////////////////////////////////////////////////////////
static void chan_cab_select(byte which) 
{
	// ensure that the requested cab is connected 
	if(chan[which].status < IS_CONNECTED) {
		ui_chan_error(which);
		return;
	}	
	// deselect any selected channel
	int cab = chan_first_selected(CAB_BASE, CAB_MAX);
	if(cab >= 0) {
		chan[cab].status = IS_CONNECTED;
	}		
	// select the new channel
	chan[which].status = IS_SELECTED;

	// select an amp channel if none already selected
	if(chan_first_selected(AMP_BASE, AMP_MAX) < 0) {
		int amp = chan_first_connected(AMP_BASE, AMP_MAX);
		if(amp >= 0) {
			chan_amp_select(amp);
		}
	}

}

/////////////////////////////////////////////////////////////////////
static void chan_cab_cable_in(byte which) {
	chan[which].status = IS_CONNECTED;		
	// is there a selected cab?
	int cab = chan_first_selected(CAB_BASE, CAB_MAX);
	if(cab < 0) {
		// no, select the newly connected one
		chan_cab_select(which);
	}		
}

/////////////////////////////////////////////////////////////////////
static void chan_cab_cable_out(byte which) {

	chan[which].status = IS_DISCONNECTED;		
	
	// are any other cabs selected
	int cab = chan_first_connected(CAB_BASE, CAB_MAX);
	if(cab >= 0) {
		// yes, select the other cab
		chan_cab_select(cab);
	}
	// reselect the amp, validating that cab is selected
	int amp = chan_first_selected(AMP_BASE, AMP_MAX);
	if(amp >= 0) {
		// try to reselect
		chan_amp_select(amp);
	}		
}


/////////////////////////////////////////////////////////////////////
static void chan_fx_select(byte which, byte action) 
{
	// ensure that the requested fxis connected 
	if(chan[which].status < IS_CONNECTED) {
		ui_chan_error(which);
		return;
	}
	if(chan[which].status == IS_SELECTED) {
		if(action == CHAN_DESELECT || action == CHAN_CLICK) {
			chan[which].status = IS_CONNECTED;
		}
	}
	else if(chan[which].status == IS_CONNECTED) {
		if(action == CHAN_SELECT || action == CHAN_CLICK) {
			chan[which].status = IS_SELECTED;
		}
	}	
}

/////////////////////////////////////////////////////////////////////
static void chan_fx_cable_in(byte which) {

	chan[which].status = IS_CONNECTED;	
}

/////////////////////////////////////////////////////////////////////
static void chan_fx_cable_out(byte which) {
	chan[which].status = IS_DISCONNECTED;			
}

/////////////////////////////////////////////////////////////////////
// SET INITIAL CABLE CONNECT STATUS FOR A CHANNEL
void chan_init(byte which, byte status) {
	if(status) {
		chan[which].status = IS_CONNECTED;
	}
	else {
		chan[which].status = IS_DISCONNECTED;
	}
}
	
/////////////////////////////////////////////////////////////////////
// SELECT DEFAULT CHANNELS AT POWER UP
void chan_select_default() {
	// get the first connected amp
	int amp = chan_first_connected(AMP_BASE, AMP_MAX);
	if(amp >= 0) {
		// try to select it
		chan_amp_select(amp);
	}
	chan_update();
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
	if(which >= AMP_BASE && which < AMP_MAX) {
		switch(action) 	{
		case CHAN_SELECT:
		case CHAN_CLICK:
			chan_amp_select(which);
			break;
		case CHAN_CABLEIN:
			chan_amp_cable_in(which);
			break;
		case CHAN_CABLEOUT:
			chan_amp_cable_out(which);
			break;
		}
	}
	else if(which >= CAB_BASE && which < CAB_MAX) {
		switch(action) 	{
		case CHAN_SELECT:
		case CHAN_CLICK:
			chan_cab_select(which);
			break;
		case CHAN_CABLEIN:
			chan_cab_cable_in(which);
			break;		
		case CHAN_CABLEOUT:
			chan_cab_cable_out(which);
			break;		
		}
	}
	else if(which >= FX_BASE && which < FX_MAX) {
		switch(action) 	{
		case CHAN_SELECT:
		case CHAN_CLICK:
		case CHAN_DESELECT:
			chan_fx_select(which, action);			
			break;
		case CHAN_CABLEIN:
			chan[which].status = IS_CONNECTED;
			break;		
		case CHAN_CABLEOUT:
			chan[which].status = IS_DISCONNECTED;
			break;		
		}
	}
	chan_update();
}

*/