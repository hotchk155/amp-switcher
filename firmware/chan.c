/////////////////////////////////////////////////////////////////////
// CHANNEL STATE HANDLING
#include "amp-switcher.h"

typedef struct {
	byte type;
	byte status;
} CHANNEL_INFO;

// the store for channel info
CHANNEL_INFO chan[NUM_CHANNELS];


/////////////////////////////////////////////////////////////////////
// Initialise the roles of each channel
void chan_init() {	
	int i;
	int c = 0;	
	for(i=0; i<NUM_AMP_CHANNELS; ++i, ++c) {
		chan[c].type = CH_AMP;
		chan[c].status = IS_DISCONNECTED;
	}
	for(i=0; i<NUM_CAB_CHANNELS; ++i, ++c) {
		chan[c].type = CH_CAB;
		chan[c].status = IS_DISCONNECTED;
	}
	for(i=0; i<NUM_FX_CHANNELS; ++i, ++c) {
		chan[c].type = CH_FX;
		chan[c].status = IS_DISCONNECTED;
	}	
}

/////////////////////////////////////////////////////////////////////
// Push the channel state data to the hardware
void chan_update() {
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
// Make a channel selected (if possible)
void chan_select(byte which) {
	int i;
	if(which < NUM_CHANNELS && chan[which].status >= IS_CONNECTED) {
		if(chan[which].status < IS_SELECTED) {
			if(which < AMP_MAX) {
				for(i=AMP_BASE; i<AMP_MAX; ++i) {
					if(chan[i].status == IS_SELECTED) {
						chan[i].status = IS_CONNECTED;
					}
				}
			}
			else if(which < CAB_MAX) {
				for(i=CAB_BASE; i<CAB_MAX; ++i) {
					if(chan[i].status == IS_SELECTED) {
						chan[i].status = IS_CONNECTED;
					}
				}
			}
			chan[which].status = IS_SELECTED;
			chan_update();
		}		
	}
}

/////////////////////////////////////////////////////////////////////
// Make a channel deselected 
void chan_deselect(byte which) {
	if(which < NUM_CHANNELS && chan[which].status == IS_SELECTED) {
		chan[which].status = IS_CONNECTED;
		chan_update();
	}
}

/////////////////////////////////////////////////////////////////////
// Toggle the state of a channel when button is pressed
void chan_click(byte which) {
	if(which < NUM_CHANNELS) {
		switch(chan[which].status) {
			case IS_SELECTED:
				chan_deselect(which);
				break;
			case IS_CONNECTED:
				chan_select(which);
				break;
		}
	}	
}

/////////////////////////////////////////////////////////////////////
// Inform a channel that cable has been connected
void chan_connect(byte which) {
	if(which < NUM_CHANNELS && chan[which].status == IS_DISCONNECTED) {
		chan[which].status = IS_CONNECTED;
		chan_update();
	}
}

/////////////////////////////////////////////////////////////////////
// Inform a channel that cable has been disconnected
void chan_disconnect(byte which) {
	if(which < NUM_CHANNELS && chan[which].status > IS_DISCONNECTED) {
		chan[which].status = IS_DISCONNECTED;
		chan_update();
	}
}
