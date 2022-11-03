/****************************************************************************************************************************
* Quantizer
* Designed for arduino nano
*
* -> 10 Scales
* -> 0-9 Volt output
* -> 3 Transpose modes
* -> Remote trigger
*
* By Thomas Knak
****************************************************************************************************************************/

#define  ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>

#define ANALOG_PITCH_CV_PIN 0
#define ANALOG_TRANSPOSE_CV_PIN 1				// transpose CV input

#define ANALOG_TRANSPOSE_POT_PIN 6			// transpose potentiometer
#define ANALOG_SWITCH_IN_PIN 7			// R2R switch input
#define JITTER 5
#define TO_VOLT 293

#define ENCODER_A_PIN 1
#define ENCODER_B_PIN 2 // interrupt pin
#define ENCODER_SWITCH_PIN 0
Encoder encoder1(ENCODER_B_PIN, ENCODER_A_PIN);

#define DAC_SCK_PIN	11
#define DAC_SDI_PIN	12
#define DAC_LDAC_PIN 18
#define DAC_CS_PIN 19

#define REMOTE_TRIGGER_PIN 16			// Trigger input
#define GATE_PIN 13						// gate output
#define TRANSPOSE_OFFSET_PIN 17			// + 2 octave transpose output

// R2R analog switch bit position
#define SWITCH_TRANSPOSE_BIT 4
#define SWITCH_REMOTE_TRIGGER_BIT 2
#define SWITCH_SPARE_BIT 1

// Used with "old" valuse to force new analog reading
#define INVALID_VALUE 10000

// Number of loops gate is high. loop time ~500µS
#define GATE_TIME 80	// 40mS

// 7-segment output pins
#define SEG_A_PIN 3
#define SEG_B_PIN 4
#define SEG_C_PIN 5
#define SEG_D_PIN 6
#define SEG_E_PIN 7
#define SEG_F_PIN 8
#define SEG_G_PIN 9
#define SEG_DP_PIN 10

// 7-segment chars. Segment order as bite DP g f e d c b a, 0b00000110 turns on segment b and c thus showing nr 1
#define _0  0b00111111
#define _1  0b00000110
#define _2  0b01011011
#define _3  0b01001111
#define _4  0b01100110
#define _5  0b01101101
#define _6  0b01111101
#define _7  0b00000111
#define _8  0b01111111
#define _9  0b01101111
#define _DP 0b10000000

#define _C	0b00111001
#define _CS	0b10111001
#define _D	0b01011110
#define _DS 0b11011110
#define _E	0b01111001
#define _F	0b01110001
#define _FS	0b11110001
#define _G	0b00111101
#define _GS	0b10111101
#define _A	0b01110111
#define _AS	0b11110111
#define _B	0b01111100

// mode selector display
//#define _MODE_COUNT_WITH_OFFSET 10
#define _MODE_COUNT 3
#define _MODE_1 0b00001000
#define _MODE_2 0b01000000
#define _MODE_3 0b00000001

// 7-segment display presets
const static byte mode[] = { _MODE_1, _MODE_2, _MODE_3 };
const static byte nr[] = { _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _DP };
const static byte key[] = { _C, _CS, _D, _DS, _E, _F, _FS, _G, _GS, _A, _AS, _B };
const static int segment_array[] = { SEG_A_PIN, SEG_B_PIN, SEG_C_PIN, SEG_D_PIN, SEG_E_PIN, SEG_F_PIN, SEG_G_PIN, SEG_DP_PIN };

// MCP4921...
// (pin allocations for convenience in hardware hook-up)
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

byte upper_byte = 0x10;
byte lower_byte = 0;

// Chromatic (Analog in) 5V in - 7V out
//const static int ain_chromatic_scale[] = {
//	0, 6, 18, 30, 43, 55, 67, 79, 91, 104, 116, 128,
//	140, 152, 164, 177, 189, 201, 213, 225, 237, 250, 262, 274,
//	286, 298, 311, 323, 335, 347, 359, 371, 384, 396, 408, 420,
//	432, 445, 457, 469, 481, 493, 505, 518, 530, 542, 554, 566,
//	578, 591, 603, 615, 627, 639, 652, 664, 676, 688, 700, 712,
//	725, 737, 749, 761, 773, 786, 798, 810, 822, 834, 846, 859,
//	871, 883, 895, 907, 919, 932, 944, 956, 968, 980, 993, 1005,
//	1017
//};

// Chromatic scale (DAC) 7V
const static int dac_chromatic_scale[] = {
	0,49,98,146,195,244,293,341,390,439,488,536,
	585,634,683,731,780,829,878,926,975,1024,1073,1121,
	1170,1219,1268,1316,1365,1414,1463,1511,1560,1609,1658,1706,
	1755,1804,1853,1901,1950,1999,2048,2096,2145,2194,2243,2291,
	2340,2389,2438,2486,2535,2584,2633,2681,2730,2779,2828,2876,
	2925,2974,3023,3071,3120,3169,3218,3266,3315,3364,3413,3461,
	3510,3559,3608,3656,3705,3754,3803,3851,3900,3949,3998,4046,
	4095
};

// chromatic (DAC) 5V
//const static int dac_chromatic_scale[] = {
//	 0,  68, 137,  205,  273,  341,  410,  478,  546,  614,  683,  751,  
//	 819,  887,  956,  1024, 1092, 1161, 1229, 1297, 1365, 1434, 1502, 1570, 
//	 1638, 1707, 1775, 1843, 1911, 1980, 2048, 2116, 2185, 2253, 2321, 2389, 
//	 2458, 2526, 2594, 2662, 2731, 2799, 2867, 2935, 3004, 3072, 3140, 3209, 
//	 3277, 3345, 3413, 3482, 3550, 3618, 3686, 3755, 3823, 3891, 3959, 4028, 4095
//};

// Scales from chromatic
#define NR_OF_SCALES 10
#define NR_OF_KEYS 12
const static bool scale_array[10][12] = {
	{ true, false, true, false, true, true, false, true, false, true, false, true },        // Major
	{ true, false, true, false, true, false, false, true, false, true, false, false },      // Major pentatonic
	{ true, false, true, false, true, false, false, true, false, true, false, true },       // Major pentatonic + 7
	{ true, false, true, true, false, true, false, true, true, false, true, false },        // Natural Minor
	{ true, false, false, true, false, true, false, true, false, false, true, false },      // Minor pentatonic
	{ true, false, true, true, false, true, false, true, true, false, false, true },        // Harmonic minor
	{ true, false, true, true, false, true, false, true, false, true, false, true },        // Melodic minor
	{ true, false, false, true, false, true, true, true, false, false, true, false },       // Blues
	{ true, true, true, true, true, true, true, true, true, true, true, true},              // Chromatic
	{ true, false, false, false, false, false, false, false, false, false, false, false },  // Octave
};

static int DOUBLE_CLICK_TIME = 500;  //Max time beetween double click detection in ms

// 0 indexed (length -1)
int chromatic_scale_length = 0;
static int encoder1_counter = 0;
static int encoder1_move = 0;
static int encoder1_new_position = 0;

void setup() {
	//Serial.begin(9600);
	pinMode(GATE_PIN, OUTPUT);

	// 7-segment setup
	pinMode(SEG_A_PIN, OUTPUT);
	pinMode(SEG_B_PIN, OUTPUT);
	pinMode(SEG_C_PIN, OUTPUT);
	pinMode(SEG_D_PIN, OUTPUT);
	pinMode(SEG_E_PIN, OUTPUT);
	pinMode(SEG_F_PIN, OUTPUT);
	pinMode(SEG_G_PIN, OUTPUT);
	pinMode(SEG_DP_PIN, OUTPUT);

	// encoder setup
	pinMode(ENCODER_A_PIN, INPUT_PULLUP);
	pinMode(ENCODER_B_PIN, INPUT_PULLUP);
	pinMode(ENCODER_SWITCH_PIN, INPUT_PULLUP);

	pinMode(TRANSPOSE_OFFSET_PIN, OUTPUT); // 2V offset
	pinMode(REMOTE_TRIGGER_PIN, INPUT_PULLUP);

	DACsetup();
	chromatic_scale_length = (sizeof(dac_chromatic_scale) / sizeof(dac_chromatic_scale[0]) - 1);
}

void loop() {
	//***************************************************************************************************************************
	//:::::::::::::::::::::::::::::::::::::::::::::::::::::: Read encoder :::::::::::::::::::::::::::::::::::::::::::::::::::::::
	//***************************************************************************************************************************
	static int scale = 0;
	static int root_key = 0;
	static int transpose_mode = 0;
	static bool encoder1_double_click = false;
	static bool encoder1_first_click = false;
	static unsigned long encoder1_click_timer = 0;

	if (!digitalRead(ENCODER_SWITCH_PIN)) // switch on
	{
		// check if double click or not
		if (encoder1_first_click)
		{
			encoder1_first_click = false;
			if ((encoder1_click_timer + DOUBLE_CLICK_TIME) > millis())
			{
				encoder1_double_click = true;
				Segment_Display(mode[transpose_mode]);
			}
			else
			{
				Segment_Display(key[root_key]);
			}
			encoder1_click_timer = millis();
		}
		if (Encoder1_Move())
		{
			// double click mode selector 
			if (encoder1_double_click)
			{
				transpose_mode = (transpose_mode + encoder1_move + _MODE_COUNT) % _MODE_COUNT; //_MODE_COUNT_WITH_OFFSET) % _MODE_COUNT_WITH_OFFSET;
				Segment_Display(mode[transpose_mode]);
			}
			else // click key selector
			{
				root_key = (root_key + encoder1_move + NR_OF_KEYS) % NR_OF_KEYS; // count from 0 -> 11
				Segment_Display(key[root_key]);
			}
		}
	}
	else // switch off, scale selector
	{
		if (Encoder1_Move() || encoder1_first_click == false)
		{
			scale = (scale + encoder1_move + NR_OF_SCALES) % NR_OF_SCALES; // count from 0 -> 9
			Segment_Display(nr[scale]);
		}
		encoder1_first_click = true;
		encoder1_double_click = false;
	}

	//***************************************************************************************************************************
	//::::::::::::::::::::::::::::::::::::::::::::::::::::::: R2R switch ::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	//***************************************************************************************************************************
	static int transpose_pot_old = INVALID_VALUE;
	static int switch_read_old = INVALID_VALUE;
	static int debounceR2R = 0;
	static int previous_note = INVALID_VALUE;
	static bool switch_remote_trigger = false;
	// 0 = transpose offset off, 1 = transpose offset on
	static bool switch_transpose_offset = false;
	static bool switch_transpose_offset_old = false;
	// enable transpose offset
	static bool transpose_offset = false;

	int switch_read = analogRead(ANALOG_SWITCH_IN_PIN);
	switch_read = map(switch_read, -64, 960, 0, 8);

	if (debounceR2R != 0)
	{
		if (debounceR2R == 1)  // debounce delay end
		{
			switch_read_old = switch_read;
			switch_remote_trigger = !(SWITCH_REMOTE_TRIGGER_BIT & switch_read); // remote trigger active low to avoide hazard state when switching transpose offset
			switch_transpose_offset = SWITCH_TRANSPOSE_BIT & switch_read;
			// if offset switch changed set new state
			if (switch_transpose_offset != switch_transpose_offset_old)
			{
				switch_transpose_offset_old = switch_transpose_offset;
				transpose_pot_old = INVALID_VALUE; // recalculate transpose when transpose offset changes
				previous_note = INVALID_VALUE;
				transpose_offset = switch_transpose_offset;
			}
		}
		debounceR2R--;
	}
	if (switch_read != switch_read_old && debounceR2R == 0)
	{
		debounceR2R = 3;		// start debounce delay
	}

	//****************************************************************************************************************************
	//::::::::::::::::::::::::::::::::::::::::::::::::::::::: Transpose pot ::::::::::::::::::::::::::::::::::::::::::::::::::::::
	//****************************************************************************************************************************
	static int transpose_pot = 0;
	int transpose_read = analogRead(ANALOG_TRANSPOSE_POT_PIN);
	if (abs(transpose_read - transpose_pot_old) > JITTER)
	{
		transpose_pot_old = transpose_read;
		if (!switch_transpose_offset)
		{
			if (transpose_read > TO_VOLT)
			{
				transpose_read = transpose_read - TO_VOLT;
				transpose_offset = true;
			}
			else
			{
				transpose_offset = false;
			}
		}
		transpose_pot = Transpose_Mode(transpose_read, transpose_mode);
	}

	//****************************************************************************************************************************
	// ::::::::::::::::::::::::::::::::::::::::::::::::::: Remote Trigger :::::::::::::::::::::::::::::::::::::::::::::::::::::::
	//***************************************************************************************************************************/
	
	// Remote trigger signal ready for processing
	static bool remote_trigger = false; 
	// Only trigger on high to low
	static bool remote_trigger_reset = true;

	if (digitalRead(REMOTE_TRIGGER_PIN) == LOW && switch_remote_trigger)
	{
		if (remote_trigger_reset)
		{
			remote_trigger = true;
			remote_trigger_reset = false;
		}
	}
	else
	{
		remote_trigger_reset = true;
	}

	//****************************************************************************************************************************
	//::::::::::::::::::::::::::::::::::::::::::::::::::::: Transpose CV ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	//***************************************************************************************************************************/
	static int transpose_cv = 0;
	static int transpose_cv_old = INVALID_VALUE; 
	transpose_read = analogRead(ANALOG_TRANSPOSE_CV_PIN);
	if (abs(transpose_read - transpose_cv_old) > JITTER || encoder1_move != 0)
	{
		transpose_cv_old = transpose_read;
		transpose_cv = Transpose_Mode(transpose_read, transpose_mode);
	}

	//****************************************************************************************************************************
	//::::::::::::::::::::::::::::::::::::::::::::::::::::::: PITCH CV :::::::::::::::::::::::::::::::::::::::::::::::::::::::
	//***************************************************************************************************************************/
	// dac_chromatic_scale array index to set DAC
	static int ain_tone_index = 0;
	static int pitch_cv_old = INVALID_VALUE;
	int pitch_cv = analogRead(ANALOG_PITCH_CV_PIN) + transpose_cv + transpose_pot;
	if (abs(pitch_cv_old - pitch_cv) > JITTER)
	{
		pitch_cv_old = pitch_cv;
		ain_tone_index = map(pitch_cv, -6, 1017, 0, 84);  // 0-1023 (-6) for shift midt tone, maybe ruined by jitter rejection
		ain_tone_index = Next_Note(scale, ain_tone_index);

		// if end of DAC array find first note down and add root_key index
		if (ain_tone_index + root_key > chromatic_scale_length)
		{
			ain_tone_index = Next_note_down(scale, chromatic_scale_length - root_key) + root_key;
		}
		else
		{
			ain_tone_index = ain_tone_index + root_key;
		}

		// stop at end of array
		if (ain_tone_index > chromatic_scale_length)
		{
			ain_tone_index = chromatic_scale_length; // chromatic_scale_length;
		}
	}
	//****************************************************************************************************************************
	//::::::::::: Set DAC and pulse Gate out if new note or Remote tigger signal:::::::::::::::::::::::::::::::::::::::::::::::::::
	//::::::::::: not all input changes lead to new output notes ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	//****************************************************************************************************************************/
	static int gateLoopCounter = 0;
	if (gateLoopCounter != 0)		// count down gate loop counter and end gate pulse at =1
	{
		if (gateLoopCounter == 1)
		{
			digitalWrite(GATE_PIN, LOW);
		}
		gateLoopCounter--;
	}
	if (ain_tone_index != previous_note)
	{
		if (!switch_remote_trigger || remote_trigger)
		{
			digitalWrite(GATE_PIN, HIGH); 
			gateLoopCounter = GATE_TIME;		// start gate pulse

			if (transpose_offset)
			{
				digitalWrite(TRANSPOSE_OFFSET_PIN, HIGH);
			}
			else
			{
				digitalWrite(TRANSPOSE_OFFSET_PIN, LOW);
			}
			Set_DAC_4921(dac_chromatic_scale[ain_tone_index]);
			previous_note = ain_tone_index;
		}
	}
	else
	{
		if (remote_trigger)		// same note, only pulse gate out
		{
			// pulse gate out with pitch change
			digitalWrite(GATE_PIN, HIGH); 
			gateLoopCounter = GATE_TIME;		// start gate pulse
		}
	}
	remote_trigger = false;
}

// Transpose cv_value according to mode selection
// - returns transposed CV
int Transpose_Mode(int cv_value, int mode) {
	mode = mode % (_MODE_COUNT);
	switch (mode)
	{
	case 0: // //CV transposing in Semitones
	default:
		break;
	case 1: //CV transposing in Fifths (7 Semitones)
		cv_value = map(cv_value, 0, 1023, 0, 12) * 85; // 85 bit = 7 seminotes
		break;
	case 2:	//CV transposing in Octaves (12 Semitones) 
		cv_value = map(cv_value, 0, 1023, 0, 7) * 146; // 146 bit = 12 seminotes
		break;
	}
	return cv_value;
}

bool Encoder1_Move() {
	encoder1_move = 0;
	encoder1_new_position = encoder1.read();
	encoder1_move = (encoder1_new_position - encoder1_counter) / 4;
	if (encoder1_move != 0)
	{
		encoder1_counter = encoder1_new_position;
		return true;
	}
	return false;
}

void Segment_Display(byte value) {
	for (int i = 0; i < 8; i++)
	{
		digitalWrite(segment_array[i], bitRead(value, i));
	}
}

// subroutine to set DAC on MCP4921
void Set_DAC_4921(int DC_Value) {
	lower_byte = DC_Value & 0xff;
	upper_byte = (DC_Value >> 8) & 0x0f;
	bitSet(upper_byte, 4);
	bitSet(upper_byte, 5);
	digitalWrite(DAC_CS_PIN, LOW);
	tfr_byte(upper_byte);
	tfr_byte(lower_byte);
	digitalWrite(DAC_SDI_PIN, LOW);
	digitalWrite(DAC_CS_PIN, HIGH);
	digitalWrite(DAC_LDAC_PIN, LOW);
	digitalWrite(DAC_LDAC_PIN, HIGH);
}

// transfers a byte, a bit at a time, LSB first to the DAC
void tfr_byte(byte data) {
	for (int i = 0; i < 8; i++, data <<= 1) {
		digitalWrite(DAC_SDI_PIN, data & 0x80);
		pulseHigh(DAC_SCK_PIN);   //after each bit sent, CLK is pulsed high
	}
}

void DACsetup() {
	pinMode(DAC_CS_PIN, OUTPUT);
	pinMode(DAC_SCK_PIN, OUTPUT);
	pinMode(DAC_SDI_PIN, OUTPUT);
	pinMode(DAC_LDAC_PIN, OUTPUT);

	digitalWrite(DAC_CS_PIN, HIGH);
	digitalWrite(DAC_LDAC_PIN, HIGH);
	digitalWrite(DAC_SCK_PIN, LOW);
	digitalWrite(DAC_SDI_PIN, HIGH);

	Set_DAC_4921(0);
}

int Next_Note(int selected_scale, int index) {
	int up = Next_note_up(selected_scale, index);
	int down = Next_note_down(selected_scale, index);

	if (up - index < index - down)
	{
		down = up;
	}
	return down;
}

int Next_note_up(int selected_scale, int index) {
	while (!scale_array[selected_scale][index % 12])
	{
		index++;
	}
	if (index > chromatic_scale_length)
	{
		index = index + 12; // if out of range, exclude next_note_up from being closest note
	}
	return index;
}

int Next_note_down(int selected_scale, int index) {
	while (!scale_array[selected_scale][index % 12])
	{
		index--;
		if (index < 0)
		{
			index = 0;
			index = Next_note_up(selected_scale, index);
		}
	}
	return index;
}

