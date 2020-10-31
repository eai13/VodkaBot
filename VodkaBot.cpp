// Sets the frequency of the oscillator
#define F_CPU 8000000L

// PWM impulse gaps for Servo
// The full gap is 20 ms, high-level part is listed below
// Position 1 is 0 degrees
// Position 2 is 60 degrees
// Position 3 is 120 degrees
// Position 4 is 180 degrees
// Default Position is 90 degreees
#define SERVO_POSITION1 10 //81 //2.6
#define SERVO_POSITION2 8 //66 //2.1
#define SERVO_POSITION3 5 //38 //1.2
#define SERVO_POSITION4 3 //22 //0.7
#define SERVO_POSITION_DEFAULT 6 //52 //1.65
// PWM iterations amount
// Sets the periods amount of PWM impulses
#define ITERATIONS_AMOUNT 78 //627 //156 //50
// Test delay length
#define TEST_DELAY_ms _delay_ms(100)
// Motor-Pump speed	ml/sec
#define MOTORPUMPSPEED 33

// *** PORT A ***
// LEDs indicating when vodka is ready (green)
#define LEDREADY1_Out 0 // PORT A
#define LEDREADY2_Out 2 // PORT A
#define LEDREADY3_Out 4	// PORT A
#define LEDREADY4_Out 6	// PORT A
// LEDs indicating when positions for cups are empty (blue)
#define LEDPUT1_Out 1 // PORT A
#define LEDPUT2_Out 3 // PORT A
#define LEDPUT3_Out 5 // PORT A
#define LEDPUT4_Out 7 // PORT A
// *** PORT A ***

// *** PORT B ***
// Motor pump
#define MOTOR_Out 1 // PORT B
// Servo
#define SERVO_Out 3	// PORT B
// Start/Stop button (located on the encoder)
#define SSBUT_In 2 // PORT B
// *** PORT B ***

// *** PORT C ***
// 7-segment indicator segments
#define SEGA_Out 0 // PORT C
#define SEGB_Out 1 // PORT C
#define SEGC_Out 2 // PORT C
#define SEGD_Out 3 // PORT C
#define SEGE_Out 4 // PORT C
#define SEGF_Out 5 // PORT C
#define SEGG_Out 6 // PORT C
// *** PORT C ***

// *** PORT D ***
// Encoder's A and B
#define ENCA_In 0 // PORT D
#define ENCB_In 1 // PORT D
// Indicators for cups put in positions
#define CUP1_In 2 // PORT D
#define CUP2_In 3 // PORT D
#define CUP3_In 4 // PORT D
#define CUP4_In 5 // PORT D
// Dynamic indication of 7-segment indicator (transistors)
#define UNITS_Out 6	// PORT D
#define TENS_Out  7	// PORT D
// *** PORT D ***

#include <util/delay.h>
#include <avr/interrupt.h>

// Variables for encoder incrementing/decrementing
volatile uint8_t incrementCount = 0;
volatile uint8_t decrementCount = 0;

// Timer 0 variables
volatile uint8_t currentIteration = 0;
volatile uint8_t switchIteration = SERVO_POSITION_DEFAULT;

// Array of numbers to show on the 7-segment indicator
volatile uint8_t numbersToShow[10] = {
	/*0*/	((1 << SEGA_Out) | (1 << SEGB_Out) | (1 << SEGC_Out) | (1 << SEGD_Out) | (1 << SEGE_Out) | (1 << SEGF_Out)),
	/*1*/	((1 << SEGC_Out) | (1 << SEGD_Out)),
	/*2*/	((1 << SEGA_Out) | (1 << SEGB_Out) | (1 << SEGD_Out) | (1 << SEGE_Out) | (1 << SEGG_Out)),
	/*3*/	((1 << SEGB_Out) | (1 << SEGC_Out) | (1 << SEGD_Out) | (1 << SEGE_Out) | (1 << SEGG_Out)),
	/*4*/	((1 << SEGC_Out) | (1 << SEGD_Out) | (1 << SEGF_Out) | (1 << SEGG_Out)),
	/*5*/	((1 << SEGB_Out) | (1 << SEGC_Out) | (1 << SEGE_Out) | (1 << SEGF_Out) | (1 << SEGG_Out)),
	/*6*/	((1 << SEGA_Out) | (1 << SEGB_Out) | (1 << SEGC_Out) | (1 << SEGE_Out) | (1 << SEGF_Out) | (1 << SEGG_Out)),
	/*7*/	((1 << SEGC_Out) | (1 << SEGD_Out) | (1 << SEGE_Out)),
	/*8*/	((1 << SEGA_Out) | (1 << SEGB_Out) | (1 << SEGC_Out) | (1 << SEGD_Out) | (1 << SEGE_Out) | (1 << SEGF_Out) | (1 << SEGG_Out)),
	/*9*/	((1 << SEGB_Out) | (1 << SEGC_Out) | (1 << SEGD_Out) | (1 << SEGE_Out) | (1 << SEGF_Out) | (1 << SEGG_Out))};

// 7-segment indicators struct
struct MyIndicators{
	// Number of tens in the indicator
	volatile uint8_t tens;
	// Number of units in the indicator
	volatile uint8_t units;
	// flag dor dynamic indication with transistors
	volatile uint8_t dynamicIndicationFlag;
	// Function for getting the current volume
	volatile uint8_t Volume(void){
		return (tens * 10 + units);
	}
	// Function for representing the current volume in the indicators
	volatile void ShowNumber(uint8_t numberPosition){
		// If the number to indicate is tens number
		if (numberPosition == 0){
			PORTD |= (1 << TENS_Out);
			PORTD &= ~(1 << UNITS_Out);
			PORTC =	numbersToShow[tens];
			dynamicIndicationFlag = 1;
		}
		else if (numberPosition == 1){
			PORTD &= ~(1 << TENS_Out);
			PORTD |= (1 << UNITS_Out);
			PORTC =	numbersToShow[units];
			dynamicIndicationFlag = 0;
		}
	}
};

// Indication LEDs struct
struct MyIndicationLEDs{
	// LEDPUT states, 1 is for on, 0 is for off
	uint8_t putLED[4];
	// LEDREADY	states, 1 is for on, 0 is for off
	uint8_t readyLED[4];
	// Function for setting the PUTLED, number counts from 0 to 3
	void setPutLED(uint8_t putLEDnumber){
		// Setting the putLED to 1
		putLED[putLEDnumber] = 1;
		PORTA |= (1 << (putLEDnumber * 2 + 1));
	}
	// Funciton for resetting the PUTLED, number counts from 0 to 3
	void resetPutLED(uint8_t putLEDnumber){
		// Setting the putLED to 0
		putLED[putLEDnumber] = 0;
		PORTA &= ~(1 << (putLEDnumber * 2 + 1));
	}
	// Function for setting the READYLED, number counts from 0 to 3
	void setReadyLED(uint8_t readyLEDnumber){
		// Setting the readyLED to 1
		readyLED[readyLEDnumber] = 1;
		PORTA |= (1 << (readyLEDnumber) * 2);
	}
	// Function for resetting the READYLED, number counts from 0 to 3
	void resetReadyLED(uint8_t readyLEDnumber){
		// Setting the readyLED to 0
		readyLED[readyLEDnumber] = 0;
		PORTA &= ~(1 << (readyLEDnumber * 2));
	}
};

// Cups terminators struct
struct MyCups{
	// 1 is for empty, 0 is for not empty
	uint8_t positionEmpty[4];
	// 1 is for filled, 0 is for not filled
	uint8_t cupFilled[4];
	// Sets the empty and filled arrays
	void CheckPresenceAndFillness(void){
		for (uint8_t i = CUP1_In; i < (CUP4_In + 1); i++){
			// If there is cup in position
			if (!(PIND & (1 << i))){
				// Setting the positionEmpty flag to 0
				positionEmpty[i - 2] = 0;
			}
			// If there is no cup in position
			else if (PIND & (1 << i)){
				// Setting the positionEmpty flag to 1
				positionEmpty[i - 2] = 1;
				cupFilled[i - 2] = 0;
			}
		}
	}
};

// Pump Drive struct
struct MyDrive{
	// 1 is for turned on, 0 is for turned off
	uint8_t state;
	// 1 is for error was detected, 0 is for no errors
	uint8_t errorPouring;
	// 1 is for Pouring was ended, 0 is for Pouring was not ended
	uint8_t endPouring;
	// Function turns the motor pump on
	void On(void){
		PORTB |= (1 << MOTOR_Out);
		// Setting state of the motor pump to 1
		state = 1;
	}
	// Function turns the motor pump off
	void Off(void){
		PORTB &= ~(1 << MOTOR_Out);
		// Setting state of the motor pump to 0
		state = 0;
	}
	// Function starts the pouring
	void StartPouring(uint16_t ml, uint8_t cupIndex, MyCups* cup){
		// Setting endPouring to 0, means that the pouring was started
		endPouring = 0;
		// Turning on the motor pump
		On();
		// Waiting till the fluid fills the volume
		for (uint16_t pouringIterator = 0; pouringIterator < (ml * 30); pouringIterator++){
			// If the cup was taken up
			if (cup->positionEmpty[cupIndex] == 1){
				// Turning off the pump
				Off();
				// Setting the error flag to 1
				errorPouring = 1;
				// Setting the fillness flag to 0
				cup->cupFilled[cupIndex] = 0;
				// Getting out of the loop
				break;
			}
			// If the cup is in position
			else{
				// Setting the error flag to 0
				errorPouring = 0;
				_delay_ms(1);
			}
		}
		// Turning off the motor pump
		Off();
		// If the pouring process failed
		if (errorPouring == 1){
			// Setting the fillness flag to 0
			cup->cupFilled[cupIndex] = 0;
		}
		// If the pouring process succeeded
		else{
			// Setting the fillness flag to 1
			cup->cupFilled[cupIndex] = 1;
		}
		// Setting endPouring to 1, means that te pouring was ended
		endPouring = 1;
	}
};

// Servo struct
struct MyServo{
	// Variable, containing the current position value
	uint8_t currentPosition;
	// Servo position setting function
	// positionNumber is integer in range of [0 ; 4]
	// 0 - 90 degrees
	// 1 - 0 degrees
	// 2 - 60 degrees
	// 3 - 120 degrees
	// 4 - 180 degrees
	void SetPosition(uint8_t position){
		currentPosition = position;
		switch(position){
			case 0: // Default servo position (90 degrees)
				switchIteration = SERVO_POSITION_DEFAULT;
			break;
			case 1:	// 1st servo position (0 degrees)
				switchIteration = SERVO_POSITION1;
			break;
			case 2:	// 2nd servo position (60 degrees)
				switchIteration = SERVO_POSITION2;
			break;
			case 3:	// 3rd servo position (120 degrees)
				switchIteration = SERVO_POSITION3;
			break;
			case 4:	// 4th servo position (180 degrees)
				switchIteration = SERVO_POSITION4;
			break;
		}
	}
};

MyServo Servo;
MyDrive Pump;
MyCups Cups;
MyIndicationLEDs LEDs;
MyIndicators Indicators;

// Timer 2 interrupt function (Encoder + Cups recognizing + Dynamic 7-segment indication)
// Encoder works through the states
// In order to increment the fluid volume value, it should pass through
// State 0 -> State Increment 1 -> State Increment 2 -> State Increment 3 -> State 0
// In order to decrement the fluid volume value, it should pass through
// State 0 -> State Decrement 1 -> State Decrement 2 -> State Decrement 3 -> State 0
ISR(TIMER2_OVF_vect){
	// When the position is empty, the led is blue color
	// and the flag turns to zero, which means, the position is empty
	// If the cup is put in position, the led is shut
	// and the flag turns to unity, which means, the position is filled
	Cups.CheckPresenceAndFillness();
	for (uint8_t i = 0; i < 4; i++){
		if (Cups.positionEmpty[i]){
			LEDs.setPutLED(i);
		}
		else{
			LEDs.resetPutLED(i);
		}
		if (Cups.cupFilled[i]){
			LEDs.setReadyLED(i);
		}
		else{
			LEDs.resetReadyLED(i);
		}
	}
	// If the dynamic indication flag is set, then show tens and reset the flag
	Indicators.ShowNumber(Indicators.dynamicIndicationFlag);
	// Encoder checking
	// State 0
	if ((incrementCount == 0) && (decrementCount == 0)){
		// B = 0  A = 0
		// -> State 0
		if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 0){
			incrementCount = 0;
			incrementCount = 0;
		}
		// B = 0  A = 1
		// -> State Increment 1
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 1){
			incrementCount = 1;
			decrementCount = 0;
		}
		// B = 1  A = 0
		// -> State Decrement 1
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 2){
			incrementCount = 0;
			decrementCount = 1;
		}
	}
	// State Increment 1
	else if ((incrementCount == 1) && (decrementCount == 0)){
		// B = 0  A = 0
		// -> State 0
		if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 0){
			incrementCount = 0;
			decrementCount = 0;
		}
		// B = 0  A = 1
		// -> State Increment 1
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 1){
			incrementCount = 1;
			decrementCount = 0;
		}
		// B = 1  A = 1
		// -> State Increment 2
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 3){
			incrementCount = 2;
			decrementCount = 0;
		}
	}
	// State Increment 2
	else if ((incrementCount == 2) && (decrementCount == 0)){
		// B = 0  A = 0
		// -> State 0
		if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 0){
			incrementCount = 0;
			decrementCount = 0;
		}
		// B = 0  A = 1
		// -> State Increment 1
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 1){
			incrementCount = 1;
			decrementCount = 0;
		}
		// B = 1  A = 1
		// -> State Increment 2
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 3){
			incrementCount = 2;
			decrementCount = 0;
		}
		// B = 1  A = 0
		// -> State Increment 3
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 2){
			incrementCount = 3;
			decrementCount = 0;
		}
	}
	// State Increment 3
	else if ((incrementCount == 3) && (decrementCount == 0)){
		// B = 1  A = 1
		// -> State Increment 2
		if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 3){
			incrementCount = 2;
			decrementCount = 0;
		}
		// B = 1  A = 0
		// -> State Increment 3
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 2){
			incrementCount = 3;
			decrementCount = 0;
		}
		// B = 0  A = 0
		// -> State 0
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 0){
			incrementCount = 0;
			decrementCount = 0;
			if (Indicators.units == 9){
				if (Indicators.tens == 9){
					Indicators.tens = 9;
					Indicators.units = 9;
				}
				else{
					Indicators.tens = Indicators.tens + 1;
					Indicators.units = 0;
				}
			}
			else{
				Indicators.units = Indicators.units + 1;
			}
		}
	}
	// State Decrement 1
	else if ((incrementCount == 0) && (decrementCount == 1)){
		// B = 1  A = 0
		// -> State Decrement 1
		if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 2){
			incrementCount = 0;
			decrementCount = 1;
		}
		// B = 0  A = 0
		// -> State 0
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 0){
			incrementCount = 0;
			decrementCount = 0;
		}
		// B = 1  A = 1
		// -> State Decrement 2
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 3){
			incrementCount = 0;
			decrementCount = 2;
		}
	}
	// State Decrement 2
	else if ((incrementCount == 0) && (decrementCount == 2)){
		// B = 1  A = 1
		// -> State Decrement 2
		if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 3){
			incrementCount = 0;
			decrementCount = 2;
		}
		// B = 1  A = 0
		// -> State Decrement 1
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 2){
			incrementCount = 0;
			decrementCount = 1;
		}
		// B = 0  A = 0
		// -> State 0
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 0){
			incrementCount = 0;
			decrementCount = 0;
		}
		// B = 0  A = 1
		// -> State Decrement 3
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 1){
			incrementCount = 0;
			decrementCount = 3;
		}
	}
	// State Decrement 3
	else if ((incrementCount == 0) && (decrementCount == 3)){
		// B = 0  A = 1
		// -> State Decrement 3
		if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 1){
			incrementCount = 0;
			decrementCount = 3;
		}
		// B = 1  A = 1
		// -> State Decrement 2
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 3){
			incrementCount = 0;
			decrementCount = 2;
		}
		// B = 0  A = 0
		// -> State 0
		else if (((~PIND) & ((1 << ENCA_In) | (1 << ENCB_In))) == 0){
			incrementCount = 0;
			decrementCount = 0;
			if (Indicators.units == 0){
				if (Indicators.tens == 1){
					Indicators.tens = 1;
					Indicators.units = 0;
				}
				else{
					Indicators.tens = Indicators.tens - 1;
					Indicators.units = 9;
				}
			}
			else{
				Indicators.units = Indicators.units - 1;
			}
		}
	}
}

// Timer 0 is for the PWM control of the Servo
ISR(TIMER0_OVF_vect){
	// Incrementing the iteration number variable
	currentIteration = currentIteration + 1;
	// If 20 ms since the timer started passed
	// Reset the TCNT0 register
	// Reset the Servo pin
	// Reset the iteration number variable
	if (currentIteration == ITERATIONS_AMOUNT){
		PORTB |= (1 << SERVO_Out);
		currentIteration = 0;
	}
	// If time of high signal passed set the servo pin
	else if (currentIteration == switchIteration){
		PORTB &= ~(1 << SERVO_Out);
	}
}

// Ports initializing function
volatile void PortsInitialization(void);
// Timers initialization function
volatile void TimersInitialization(void);

int main(void){
	TimersInitialization();
	Indicators.dynamicIndicationFlag = 1;
	Indicators.tens = 2;
	Indicators.units = 5;
	PortsInitialization();
	sei();
	Servo.SetPosition(0);
	_delay_ms(5000);
	while(1){
		if (!(PINB & (1 << SSBUT_In))){
			_delay_ms(100);
			if (!(PINB & (1 << SSBUT_In))){
				for (uint8_t i = 1; i < 5; i++){
					if (Cups.positionEmpty[i - 1] == 0){
						Servo.SetPosition(i);
						_delay_ms(1500);
						if (Cups.positionEmpty[i - 1] == 1){
							//nothing
						}
						else{
							Pump.StartPouring(Indicators.Volume(), i - 1, &Cups);
							_delay_ms(2500);
						}
					}
					else{
						//nothing
					}
				}
			}
		}
	}
}

volatile void PortsInitialization(void){
	// Port A is for indicating Ready (green) or Put (blue) LEDS
	// Port B is for Motor-Pump and Servo
	// Port C is for the 7-segment indicators
	// Port D is for the dynamic indication transistors
	DDRA |= (1 << LEDREADY1_Out) | (1 << LEDREADY2_Out) | (1 << LEDREADY3_Out) | (1 << LEDREADY4_Out) | (1 << LEDPUT1_Out) | (1 << LEDPUT2_Out) | (1 << LEDPUT3_Out) | (1 << LEDPUT4_Out);
	DDRB |= (1 << MOTOR_Out) | (1 << SERVO_Out);
	DDRC |= (1 << SEGA_Out) | (1 << SEGB_Out) | (1 << SEGC_Out) | (1 << SEGD_Out) | (1 << SEGE_Out) | (1 << SEGF_Out) | (1 << SEGG_Out);
	DDRD |= (1 << UNITS_Out) | (1 << TENS_Out);
}

volatile void TimersInitialization(void){
	// PWM for Timer 0
	TIMSK |= (1 << TOIE0);
	TCCR0 |= (1 << CS01);
	//Enabling the timer TIMER2 to count normally
	//The TIMER2 scaler is 128
	TIMSK |= (1 << TOIE2);
	TCCR2 |= (1 << CS21) | (1 << CS20);
	TEST_DELAY_ms;
}