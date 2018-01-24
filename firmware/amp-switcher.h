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




#define NUM_AMP_CHANNELS	4
#define NUM_CAB_CHANNELS	2
#define NUM_FX_CHANNELS		2

#define BLINK_MS_MIDI 2
#define BLINK_MS_ERROR 1000
#define BLINK_MS_ACTION 100

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

#define NUM_PATCHES 8

#define FX_OFF		0
#define FX_ON		1
#define FX_TOGGLE	2


typedef unsigned char byte;

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
extern DEVICE_STATUS g_patch[NUM_PATCHES];


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


void init_config();




void ui_chan_led(byte which, byte state);
void ui_key_press(byte i, byte press);
void ui_run();
void ui_display_msg(byte count, byte c1, byte c2, byte c3, byte c4, byte c5, byte c6);
byte ui_digit(byte which);

/////////////////////////////////////////////////////////////////////
void chan_init(byte which, byte status);
void chan_init_connected(byte which, byte status);
void chan_event(byte which, byte action);
void chan_init_patch(int index);


void blink_blue(int ms);
void blink_yellow(int ms);


void storage_load_patch(byte which);
void storage_save_patch(byte which);
void storage_load_config();
void storage_save_config();
void storage_init(byte reset);
