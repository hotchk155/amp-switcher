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

#define FX_OFF		0
#define FX_ON		1
#define FX_TOGGLE	2


typedef unsigned char byte;

#define DEFAULT_MIDI_CHAN	0
#define DEFAULT_AMP_CC		70
#define DEFAULT_CAB_CC		71
#define DEFAULT_FX0_CC		72
#define DEFAULT_FX1_CC		73

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

enum {
	CHAN_INIT,
	CHAN_CABLEIN,
	CHAN_CABLEOUT,
	CHAN_SELECT,
	CHAN_CLICK,
	CHAN_DESELECT
};

enum {
	ERR_NONE,
	ERR_CAB_DISCONNECTED,
	ERR_AMP_DISCONNECTED,
	ERR_NO_CABS,
	ERR_NO_AMPS
};

enum {
	UI_DIGIT1,
	UI_DIGIT2,
	UI_DIGIT3
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

typedef struct {
	byte midi_chan;
	byte amp_cc;
	byte cab_cc;
	byte fx_cc[NUM_FX_CHANNELS];
} DEVICE_CONFIG;


#define NO_SELECTION (0xFF)
typedef struct {
	byte pc_no;
	byte amp_sel;
	byte cab_sel;
	byte fx_sel[NUM_FX_CHANNELS];
} DEVICE_STATUS;

extern DEVICE_STATUS g_status;
extern DEVICE_CONFIG g_config;

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

#define K_SEL		K_BUTTON1
#define K_MINUS		K_BUTTON2
#define K_PLUS		K_BUTTON3


#define K_MAX_BITS	19 // the key status registers are 32-bit but how many in use?


#define LED_OFF		0x00
#define LED_GREEN	0x01
#define LED_RED		0x02
#define LED_MASK	0x03


extern volatile INPUT_STATE panel_input = {0};
extern volatile OUTPUT_STATE output_state = {0};



void ui_chan_led(byte which, byte state);
void ui_error(byte which);
void ui_key_press(byte i, byte press);
void ui_run();

/////////////////////////////////////////////////////////////////////
void chan_init(byte which, byte status);
void chan_init_connected(byte which, byte status);
void chan_event(byte which, byte action);


void store_load_patch(byte which);
void store_save_patch(byte which);

void blink_blue(byte ms);
void blink_yellow(byte ms);