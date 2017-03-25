/**
 * @brief Single-string stepper demo.
 */

#include <stdint.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

#include "Servo.h"


// - Defines -------------------------------------------------------------------

#define SERVO 1
//#define STEPPER 1

#define IIR_DIV 8
#define TRUNC 3
#define MOTION SINGLE
#define MAX_STEPS 100
#define POT_INPUT A0

// TODO these don't give full swing
// 0 = 1.5 ms
// +90 = 2.0 ms
// -90 = 1.0 ms
#define SERVO_P90_MS   2.0
#define SERVO_M90_MS   1.0
#define SERVO_0_MS     1.5
#define SERVO_PWM_FREQ 50

#define NUM_STRINGS 10
#define NUM_INPUTS (3 + 4)

// Pedals/levers
#define PEDAL_A 0
#define PEDAL_B 1
#define PEDAL_C 2
#define LKL     3
#define LKR     4
#define RKL     5
#define RKL     6

#define PEDAL_A_MIN 25
#define PEDAL_A_MAX 325

// - Types ---------------------------------------------------------------------

#if 0
typedef struct {
    int string_response[NUM_STRINGS];
} input_response_t;


typedef struct {
    input_response_t inputs[NUM_INPUTS];
} copedent_t;
#endif

// - Local Data ----------------------------------------------------------------

int pos_deg = 0;
static Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

static int32_t stepper_pos = 0;
static bool first = true;
static bool pause = true;
static int32_t tarseget_filtered = 0;

//static copedent_t copedent;

// - Local Functions -----------------------------------------------------------
#if 0
static void init_e9_copedent(copedent_t* cpd) {
    // Zero it out.
    memset(cpd, 0, sizeof(*cpd));

    // B -> C#
    cpd->inputs[PEDAL_A][4] = +2;
    cpd->inputs[PEDAL_A][9] = +2;

    // G# -> A
    cpd->inputs[PEDAL_B][2] = +1;
    cpd->inputs[PEDAL_B][5] = +1;

    // E -> F#, B -> C#
    cpd->inputs[PEDAL_C][3] = +2;
    cpd->inputs[PEDAL_C][4] = +2;

    // E -> D#
    cpd->inputs[LKL][3] = -1;
    cpd->inputs[LKL][7] = -1;

    // E -> D#
    cpd->inputs[LKL][3] = -1;
    cpd->inputs[LKL][7] = -1;

    // E -> F
    cpd->inputs[LKR][3] = +1;
    cpd->inputs[LKR][7] = +1;

    // D# -> C#, D -> D#
    cpd->inputs[RKL][1] = -2;
    cpd->inputs[RKL][8] = -1;

    // F# - G
    cpd->inputs[LKR][0] = +1;
    cpd->inputs[LKR][6] = +1;
}
#endif

static void set_servo_pos(int servo, float deg) {
    // Don't get crazy.
    if(deg > 90 || deg < -90) {
        return;
    }
#if 1    
    Serial.print("\ndeg = ");
    Serial.print(deg);
#endif
    // TODO make this all fixed point later
    float pulse_ms = (float) deg/180 + SERVO_0_MS;
    float period_ms = 1000*(1.0/SERVO_PWM_FREQ);
    float duty_cycle = pulse_ms/period_ms;
    int pulse_bits = (int) ((2<<11) * (duty_cycle));
#if 0
    Serial.print("\nduty cycle = ");
    Serial.print(duty_cycle);

    Serial.print("\npulse ms = ");
    Serial.print(pulse_ms);
    Serial.print("\nperiod ms = ");
    Serial.print(period_ms);


    Serial.print("\nbits = ");
    Serial.print(pulse_bits);
    Serial.print("\n");
#endif

#if 1
    Serial.print("servo ");
    Serial.print(servo);
    Serial.print(" pulse width ");
    Serial.print(pulse_bits);
    Serial.print("\n");
#endif
    pwm.setPWM(servo, 0, pulse_bits);
}


static void control_debug(int pot, int current, int target, int filtered) {
    Serial.print("pot = " );
    Serial.print(pot);
    Serial.print(" current = " );
    Serial.print(current);
    Serial.print(" target = " );
    Serial.print(target);
    Serial.print(" filtered = " );
    Serial.println(filtered);
}


static void process_console(void) {
    // Debug console stuff.
    if (Serial.available() > 0) {
        int in = Serial.read();

        switch(in) {
#ifdef STEPPER
            case 'b':
                Serial.println("<-");
                myMotor->step(1, BACKWARD, SINGLE);
                break;
            case 'f':
                Serial.println("->");
                myMotor->step(1, FORWARD, SINGLE);
                break;
            case 'B':
                Serial.println("<-");
                myMotor->step(16, BACKWARD, SINGLE);
                break;
            case 'F':
                Serial.println("->");
                myMotor->step(16, FORWARD, SINGLE);
                break;
            case 'L':
                Serial.println("<-");
                myMotor->step(200, BACKWARD, SINGLE);
                break;
            case 'P':
                Serial.println("->");
                myMotor->step(200, FORWARD, SINGLE);
                break;
            case 'm':
                Serial.println("<");
                myMotor->step(1, BACKWARD, MICROSTEP);
                break;
            case 'n':
                Serial.println(">");
                myMotor->step(1, FORWARD, MICROSTEP);
                break;
            case 'M':
                Serial.println("<");
                myMotor->step(16, BACKWARD, MICROSTEP);
                break;
            case 'N':
                Serial.println(">");
                myMotor->step(16, FORWARD, MICROSTEP);
                break;
            case 'u':
                pause = false;
                break;
            case 'p':
                pause = true;
                break;            
            case 'r':
                myMotor->release();
                break;
#else
            case 'b':
                Serial.println("<-");
                if(pos_deg > -90) {
                    pos_deg--;
                    set_servo_pos(0, pos_deg);
                }
                Serial.println(pos_deg);
                break;
            case 'f':
                Serial.println("->");
                if(pos_deg < 90) {
                    pos_deg++;
                    set_servo_pos(0, pos_deg);
                }
                Serial.println(pos_deg);
                break;
            case 'B':
                Serial.println("<-");
                if(pos_deg > -90) {
                    pos_deg -= 15;
                    set_servo_pos(0, pos_deg);
                }
                Serial.println(pos_deg);
                break;
            case 'F':
                Serial.println("->");
                if(pos_deg < 90) {
                    pos_deg += 15;
                    set_servo_pos(0, pos_deg);
                }
                Serial.println(pos_deg);
                break;
#endif

            default:
                Serial.println("Usage:  b/f:  step (1.8 deg)");
                Serial.println("        m/n:  microstep (1.8 deg)");
                Serial.println("        B/F:  big step (30 deg)");
                Serial.println("        M/N:  microstep (30 deg)");
                Serial.println("        L/P:    step (360 deg)");
        }
    }
}


#ifdef STEPPER
static void process_control(void) {
    // Read pot.
    uint32_t pot = analogRead(POT_INPUT) >> TRUNC;
    int32_t target = pot - (512 >> TRUNC);

    // Process new reading.
    if(first) {
        stepper_pos = target;
        target_filtered = pot - (512 >> TRUNC);
        first = false;
    } else {
        target_filtered = target/IIR_DIV + (IIR_DIV-1)*target_filtered/IIR_DIV; // IIR filtering
    }

    if(stepper_pos != target && !pause) {
        control_debug(pot, stepper_pos, target, target_filtered);
    }

    // Update motor position.
    if(!pause) {
        unsigned int delta;
        bool fwd;
        if(stepper_pos < target_filtered) {
            //Serial.println(target_filtered);
            //Serial.println(stepper_pos);
            delta = (unsigned int) (target_filtered - stepper_pos);
            //Serial.println(delta);
            fwd = false;
            //Serial.println("<<");
        } else {
            delta = (unsigned int) (stepper_pos - target_filtered);
            fwd = true;
            //Serial.println(delta);
            //Serial.println(">>");
        }

        if(delta) {
            if(fwd) {
                Serial.print("+");
            } else {
                Serial.print("-");
            }
            Serial.println(delta);
        }

        // Limit the step count per loop.
        if(delta > MAX_STEPS) {
            delta = MAX_STEPS;
        }
        myMotor->step(delta, fwd ? FORWARD : BACKWARD, MOTION);
        
        if(fwd) {
            stepper_pos -= delta;
        } else {
            stepper_pos += delta;
        }
    }
}
#endif


static void process_inputs(int inputs[NUM_INPUTS]) {
    uint32_t value = analogRead(POT_INPUT);
    //Serial.println("INPUT_0 = ");
    //Serial.println(value);
    //Serial.println("\n");

    inputs[0] = value;
    inputs[1] = 0;
    inputs[2] = 0;
    inputs[3] = 0;
    inputs[4] = 0;
    inputs[5] = 0;
    inputs[6] = 0;
}


// TODO this should take input structures, resolve conflicts, etc
static void output_positions(int inputs[NUM_INPUTS]) {
    float pct = (float) (inputs[0] - PEDAL_A_MIN)/(PEDAL_A_MAX - PEDAL_A_MIN);
    if(pct > 1.0) {
      pct = 1.0;
    } else if(pct < 0) {
      pct = 0.0;
    }
    
    //Serial.println("pct ");
    //Serial.println(pct);
    float max_deg = +30;
    float min_deg = -30;

    // Convert percentage of input to a position.
    float deg = pct*(max_deg - min_deg) + min_deg;

    // Output to servo.
    set_servo_pos(0, deg);
}


// - Public Functions ----------------------------------------------------------

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("EPSG single string demo");

  pwm.begin();
  pwm.setPWMFreq(SERVO_PWM_FREQ);
  set_servo_pos(0, 0);

  //init_e9_copedent(&copedent);

  Serial.println("Setup finished.");
  yield();
}


void loop() {
    // Sample inputs
    int inputs[NUM_INPUTS];
    process_inputs(inputs);

    // Filter inputs

    // Output new servo positions
    output_positions(inputs);

    //delay(500);
#ifdef STEPPER
    process_control();
#endif
    process_console();
}
