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


// - Local Data ----------------------------------------------------------------

#ifdef STEPPER
// Create the motor shield object with the default I2C address
static Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

static Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 1); // M1 + M2 pins
//static Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2); // M3 + M4 pins
#endif

#ifdef SERVO
Servo servo;
int pos_deg = 0;

#define SERVO_INPUT A1


static Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define PWM_FREQ 50
#endif

static int32_t stepper_pos = 0;
static bool first = true;
static bool pause = true;
static int32_t tarseget_filtered = 0;


// 0 = 1.5 ms
// +90 = 2.0 ms
// -90 = 1.0 ms
#define SERVO_P90_MS 2.0
#define SERVO_M90_MS 1.0
#define SERVO_0_MS 1.5


// - Local Functions -----------------------------------------------------------


static void set_servo_pos(int servo, float deg) {
    // Don't get crazy.
    if(deg > 90 || deg < -90) {
        return;
    }
#if 0    
    Serial.print("\ndeg = ");
    Serial.print(deg);
#endif
    // TODO make this all fixed point later
    float pulse_ms = (float) deg/180 + SERVO_0_MS;
    float period_ms = 1000*(1.0/PWM_FREQ);
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
    Serial.print("servo ");
    Serial.print(servo);
    Serial.print(" pulse width ");
    Serial.print(pulse_bits);
    Serial.print("\n");
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
                    servo.write(pos_deg + 90);
  set_servo_pos(0, pos_deg);
                }
                Serial.println(pos_deg);
                Serial.println(analogRead(SERVO_INPUT));
                break;
            case 'f':
                Serial.println("->");
                if(pos_deg < 90) {
                    pos_deg++;
                    servo.write(pos_deg + 90);
  set_servo_pos(0, pos_deg);
                }
                Serial.println(pos_deg);
                Serial.println(analogRead(SERVO_INPUT));
                break;
            case 'B':
                Serial.println("<-");
                if(pos_deg > -90) {
                    pos_deg -= 15;
                    servo.write(pos_deg + 90);
  set_servo_pos(0, pos_deg);
                }
                Serial.println(pos_deg);
                Serial.println(analogRead(SERVO_INPUT));
                break;
            case 'F':
                Serial.println("->");
                if(pos_deg < 90) {
                    pos_deg += 15;
                    servo.write(pos_deg + 90);
  set_servo_pos(0, pos_deg);
                }
                Serial.println(pos_deg);
                Serial.println(analogRead(SERVO_INPUT));
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


// - Public Functions ----------------------------------------------------------

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("EPSG single string demo");

#ifdef STEPPER
  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz
  
  myMotor->setSpeed(255);  // 10 rpm   
#endif

#ifdef SERVO
  servo.attach(9);

  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);
  set_servo_pos(0, 0);
#endif
  Serial.println("Setup finished.");
  yield();
}


void loop() {
#ifdef STEPPER
    process_control();
#endif
    process_console();
}
