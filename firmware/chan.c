/////////////////////////////////////////////////////////////////////
// CHANNEL STATE HANDLING

/*
Cannot select an amp without a cab being selected

*/

#include <system.h>
#include "amp-switcher.h"

typedef struct {
	byte status;
} CHANNEL_INFO;

// the store for channel info
CHANNEL_INFO chan[NUM_CHANNELS];


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

