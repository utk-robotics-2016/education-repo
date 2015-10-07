#include <PID_v1.h>

#include <Encoder.h>

#include <I2CEncoder.h>
#include <Wire.h>

// Globals
int ledState = HIGH;
double newpos, oldpos, newtime, oldtime, enc0pos = 0;
float velocity;
// Command parsing
const int MAX_ARGS = 6;
String args[MAX_ARGS];
int numArgs = 0;

// Pin definitions
const char LED = 13; 

// For motor driver:
#define BRAKEVCC 0
#define FW   1
#define BW  2
#define BRAKEGND 3
#define CS_THRESHOLD 100

/*  VNH2SP30 pin definitions
 xxx[0] controls '1' outputs
 xxx[1] controls '2' outputs */
int inApin[2] = {7, 4};  // INA: Clockwise input
int inBpin[2] = {8, 9}; // INB: Counter-clockwise input
int pwmpin[2] = {5, 6}; // PWM input
int cspin[2] = {2, 3}; // CS: Current sense ANALOG input
int enpin[2] = {0, 1}; // EN: Status of switches output (Analog pin)

// For encoders:
I2CEncoder encoders[2];

void setup() {
    // Init LED pin
    pinMode(LED, OUTPUT);

    // Init serial
    Serial.begin(115200);

    // Display ready LED
    digitalWrite(LED,HIGH);

    // Initialize digital pins as outputs
    for (int i=0; i<2; i++)
    {
        pinMode(inApin[i], OUTPUT);
        pinMode(inBpin[i], OUTPUT);
        pinMode(pwmpin[i], OUTPUT);
    }
    // Initialize braked
    for (int i=0; i<2; i++)
    {
        digitalWrite(inApin[i], LOW);
        digitalWrite(inBpin[i], LOW);
    }

    Wire.begin();
    // From the docs: you must call the init() of each encoder method in the
    // order that they are chained together. The one plugged into the Arduino
    // first, then the one plugged into that and so on until the last encoder.
    encoders[0].init(MOTOR_393_TORQUE_ROTATIONS, MOTOR_393_TIME_DELTA);
    //encoders[1].init(MOTOR_393_TORQUE_ROTATIONS, MOTOR_393_TIME_DELTA);
    // Ideally, moving forward should count as positive rotation.
    // Make this happen:
    encoders[0].setReversed(true);
}

/* listPos() will display the position via number of half rotation
 * Note that the encoder returns a new position every half rotation
 * so we divide by two to get the number of full rotation
 */
double listPos() {
  //double pos = 0;
  return (double)encoders[0].getPosition()/2; //returns every half rotation
  
  /*while (abs(pos) < 20.0) {
    pos = encoders[0].getPosition() / 2;
    Serial.println(pos);
  }*/
}

void calcVel() {
  encoders[0].zero();
  long beginT, endT;
  double beginPos, endPos;
  double velocity;
  beginT = millis();
  beginPos = listPos();
  delay(10);
  endPos = listPos();
  endT = millis();

   for(int i = 0; i < 25; i++) {
    beginT = millis();
    //Serial.println(beginT);
    beginPos = listPos();
    delay(10);
    endPos = listPos();
    endT = millis();
    /*Serial.println(endT);    
    Serial.println(endT-beginT);
    Serial.println(); */

    //Serial.println(endPos - beginPos, 5);
    Serial.println((endPos - beginPos)/(endT - beginT)*1000, 5);
    //velocity = (endPos - beginPos)/(endT - beginT);
    //Serial.println(velocity, 10);
  }
}

void motorOff(int motor)
{
    digitalWrite(inApin[motor], LOW);
    digitalWrite(inBpin[motor], LOW);
    analogWrite(pwmpin[motor], 0);
}

/* motorGo() will set a motor going in a specific direction
 the motor will continue going in that direction, at that speed
 until told to do otherwise.
 
 motor: this should be either 0 or 1, will selet which of the two
 motors to be controlled
 
 direct: Should be between 0 and 3, with the following result
 0: Brake to VCC
 1: Clockwise
 2: CounterClockwise
 3: Brake to GND
 
 pwm: should be a value between ? and 1023, higher the number, the faster
 it'll go
 */
void motorGo(uint8_t motor, uint8_t direct, uint8_t pwm)
{
  if (motor <= 1)
  {
    if (direct <=4)
    {
      // Set inA[motor]
      if (direct <=1)
        digitalWrite(inApin[motor], HIGH);
      else
        digitalWrite(inApin[motor], LOW);

      // Set inB[motor]
      if ((direct==0)||(direct==2))
        digitalWrite(inBpin[motor], HIGH);
      else
        digitalWrite(inBpin[motor], LOW);

      analogWrite(pwmpin[motor], pwm);
    }
  }
}

/* The loop is set up in two parts. First the Arduino does the work it needs to
 * do for every loop, next is runs the checkInput() routine to check and act on
 * any input from the serial connection.
 */
void loop() {
    int inbyte;

    // Accept and parse serial input
    checkInput();
}

void parse_args(String command) {
    numArgs = 0;
    int beginIdx = 0;
    int idx = command.indexOf(" ");

    String arg;
    char charBuffer[16];

    while (idx != -1)
    {
        arg = command.substring(beginIdx, idx);

        // add error handling for atoi:
        args[numArgs++] = arg;
        beginIdx = idx + 1;
        idx = command.indexOf(" ", beginIdx);
    }

    arg = command.substring(beginIdx);
    args[numArgs++] = arg;
}

/* This routine checks for any input waiting on the serial line. If any is
 * available it is read in and added to a 128 character buffer. It sends back
 * an error should the buffer overflow, and starts overwriting the buffer
 * at that point. It only reads one character per call. If it receives a
 * newline character is then runs the parseAndExecuteCommand() routine.
 */
void checkInput() {
    int inbyte;
    static char incomingBuffer[128];
    static char bufPosition=0;

    if(Serial.available()>0) {
        // Read only one character per call
        inbyte = Serial.read();
        if(inbyte==10||inbyte==13) {
            // Newline detected
            incomingBuffer[bufPosition]='\0'; // NULL terminate the string
            bufPosition=0; // Prepare for next command

            // Supply a separate routine for parsing the command. This will
            // vary depending on the task.
            parseAndExecuteCommand(String(incomingBuffer));
        }
        else {
            incomingBuffer[bufPosition]=(char)inbyte;
            bufPosition++;
            if(bufPosition==128) {
                Serial.println("error: command overflow");
                bufPosition=0;
            }
        }
    }
}

/* This routine parses and executes any command received. It will have to be
 * rewritten for any sketch to use the appropriate commands and arguments for
 * the program you design. I find it easier to separate the input assembly
 * from parsing so that I only have to modify this function and can keep the
 * checkInput() function the same in each sketch.
 */
void parseAndExecuteCommand(String command) {
    Serial.println("> " + command);
    parse_args(command);
    if(args[0].equals(String("ping"))) {
        if(numArgs == 1) {
            Serial.println("ok");
        } else {
            Serial.println("error: usage - 'ping'");
        }
    }
    else if(args[0].equals(String("le"))) { // led set
        if(numArgs == 2) {
            if(args[1].equals(String("on"))) {
                ledState = HIGH;
                digitalWrite(LED,HIGH);
                Serial.println("ok");
            } else if(args[1].equals(String("off"))) {
                ledState = LOW;
                digitalWrite(LED,LOW);
                Serial.println("ok");
            } else {
                Serial.println("error: usage - 'le [on/off]'");
            }
        } else {
            Serial.println("error: usage - 'le [on/off]'");
        }
    }
    else if(args[0].equals(String("rl"))) { // read led
        if(numArgs == 1) {
            Serial.println(ledState);
        } else {
            Serial.println("error: usage - 'rl'");
        }
    }
    else if(args[0].equals(String("mod"))) { // motor drive
        if(numArgs == 4) {
            int speed = args[2].toInt();
            int mot = args[1].toInt();
            int dir = FW;

            if(args[3].equals(String("bw"))) {
                dir = BW;
            }

            motorGo(mot, dir, speed);
            Serial.println("ok");
            //listPos();
            calcVel();
        } else {
            Serial.println("error: usage - 'mod [0/1] [speed] [fw/bw]'");
        }
    }
    else if(args[0].equals(String("mos"))) { // motor stop
        if(numArgs == 2) {
            int mot = args[1].toInt();
            motorOff(mot);
            Serial.println("ok");
        } else {
            Serial.println("error: usage - 'mos [0/1]'");
        }
    }
    else if(args[0].equals(String("ep"))) { // encoder position (in rotations)
        if(numArgs == 2) {
            int enc = args[1].toInt();
            double pos = encoders[enc].getPosition();
            Serial.println(pos);
        } else {
            Serial.println("error: usage - 'ep [0/1]'");
        }
    }
    else if(args[0].equals(String("erp"))) { // encoder raw position (in ticks)
        if(numArgs == 2) {
            int enc = args[1].toInt();
            long pos = encoders[enc].getRawPosition();
            Serial.println(pos);
        } else {
            Serial.println("error: usage - 'erp [0/1]'");
        }
    }
    else if(args[0].equals(String("ez"))) { // encoder zero
        if(numArgs == 2) {
            int enc = args[1].toInt();
            encoders[enc].zero();
            Serial.println("ok");
        } else {
            Serial.println("error: usage - 'ez [0/1]'");
        }
    }
    else {
        // Unrecognized command
        Serial.println("error: unrecognized command");
    }
}
