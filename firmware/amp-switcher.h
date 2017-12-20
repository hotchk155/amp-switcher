#define NUM_AMP_CHANNELS	4
#define NUM_CAB_CHANNELS	2
#define NUM_FX_CHANNELS		2

#define AMP_BASE		0
#define AMP_MAX			(AMP_BASE + NUM_AMP_CHANNELS)
#define CAB_BASE		AMP_MAX
#define CAB_MAX			(CAB_BASE + NUM_CAB_CHANNELS)
#define FX_BASE			CAB_MAX
#define FX_MAX			(FX_BASE + NUM_FX_CHANNELS)
#define NUM_CHANNELS	FX_MAX

#define NOTE_AMP_BASE	1
#define NOTE_AMP_MAX	(NOTE_AMP_BASE + NUM_AMP_CHANNELS)
#define NOTE_AMP_MUTE	35

#define NOTE_CAB_BASE	36
#define NOTE_CAB_MAX	(NOTE_CAB_BASE + NUM_CAB_CHANNELS)

#define NOTE_FX_BASE	60
#define NOTE_FX_MAX		(NOTE_FX_BASE + NUM_FX_CHANNELS)

#define NUM_PATCH_BANKS	4


typedef unsigned char byte;

#define DEFAULT_MIDI_CHAN	0
#define DEFAULT_AMP_CC		70
#define DEFAULT_CAB_CC		71
#define DEFAULT_FX0_CC		72
#define DEFAULT_FX1_CC		73


enum {
	CHAN_OFF,			// deselect a channel
	CHAN_ON,			// select a channel, not exclusive
	CHAN_TOGGLE,		// select/deselect a channel, not exclusive
	CHAN_ON_EX,			// select a channel, exclusive
	CHAN_TOGGLE_EX,		// select/deselect a channel, exclusive
	CHAN_OFF_ALL,		// all off
	CHAN_ON_FIRST		// first available, exclusive
};

enum {
	UI_DIGIT1,
	UI_DIGIT2,
	UI_DIGIT3
};

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

// States for sysex loading
enum {
	SYSEX_NONE,		// no sysex
	SYSEX_IGNORE,	// sysex in progress, but not for us
	SYSEX_ID0,		// expect first byte of id
	SYSEX_ID1,		// expect second byte of id
	SYSEX_ID2,		// expect third byte of id
	SYSEX_PARAMH,	// expect high byte of a param number
	SYSEX_PARAML,	// expect low byte of a param number
	SYSEX_VALUEH,	// expect high byte of a param value
	SYSEX_VALUEL	// expect low byte of a param value
};


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


#define LED_OFF		0x00
#define LED_GREEN	0x01
#define LED_RED		0x02
#define LED_MASK	0x03


extern volatile INPUT_STATE panel_input = {0};
extern volatile OUTPUT_STATE output_state = {0};



void ui_chan_led(byte which, byte state);
void ui_key_press(byte i, byte press);
void ui_run();

/////////////////////////////////////////////////////////////////////
// Make a channel selected (if possible)
void chan_init();
void chan_update();
void chan_select(byte which);
void chan_deselect(byte which);
void chan_click(byte which);
void chan_connect(byte which);
void chan_disconnect(byte which);
void chan_deselect_range(byte min, byte max);

