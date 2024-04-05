#include <Servo.h>
#include <Keypad.h>
#include <IRremote.h>
#include <LiquidCrystal.h>


// Define the servo objects
Servo servo1;
Servo servo2;


unsigned long previousMillis = 0;   // will store last time the buzzer was activated
unsigned long beepDuration = 500;     // duration of the current beep
const long continuousBeepDuration = 3000;  // continuous beep duration (milliseconds)
bool continuousBeepActive = false;  // flag for continuous beep
unsigned long lastDistanceMeasurementMillis = 0; // will store last time the distance was updated
const long distanceMeasurementInterval = 200;


// Servo positions for open and close
const int closePos1 = 87;
const int closePos2 = 93;
const int openPos1 = 10;
const int openPos2 = 170;


// Current state of the door
bool doorIsOpen = false;


// Password functionality
String enteredPassword = "";
const String correctPassword = "1234"; // Example password


// Keypad setup
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {36, 34, 32, 30}; // Connect to the row pinouts of the keypad
byte colPins[COLS] = {28, 26, 24, 22}; // Connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


// Button setup
const int buttonPin = 2;
int buttonState = 0;
int lastButtonState = 0;


//IR Reciever setup
int recvPin = 13;
IRrecv irrecv(recvPin);
decode_results results;
unsigned long irOpenCommand = 0xFFFFFFFF; // Replace 0xXXXXXXXX with the actual IR code from your remote


LiquidCrystal lcd(52,53,50,51,48,49);
const int trigPin = 45;
const int echoPin = 47;
long duration;
int distance;


const int buzzerPin = 3;
const int alarmPin = A9;


//LED setup
int redPin = 7;
int greenPin = 10;
int bluePin = 11;
const int ledButtonPin = 8;
bool ledOn = false;


void setup() {
  // Initialize the servos
  servo1.attach(11); // Attach servo1 to pin 11
  servo2.attach(12); // Attach servo2 to pin 12


  pinMode(buttonPin, INPUT); // Initialize the button pin as an input


  irrecv.enableIRIn();


  // Setup for ultrasonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);


  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(ledButtonPin, INPUT);
  analogWrite(redPin, 255);


  // Setup for LCD
  lcd.begin(16, 2);


  //Setup for buzzer
  pinMode(buzzerPin, OUTPUT);
 
  //Setup for alarm
  pinMode(alarmPin, INPUT);


  // Start Serial communication
  Serial.begin(9600);
  Serial.println("Setup complete. Door is initially closed.");


  // Ensure door starts closed
  closeDoor(); // This ensures the door starts in a closed state
}


void loop() {
  char key = keypad.getKey();
 
  // Check keypad input
  if (key) {
    Serial.print("Keypad Key Pressed: ");
    Serial.println(key);


    if (key=='#'){
      if(doorIsOpen == true){
        closeDoor();
        Serial.println("Door Closed via Keypad");
      }
      enteredPassword = "";
    } else if (key == '*'){
      Serial.println("Password Input Reset via Keypad");
      enteredPassword = "";
    } else {
      enteredPassword += key;


      if(enteredPassword.length() == correctPassword.length()){
        if(enteredPassword == correctPassword && doorIsOpen == false){
          openDoor();
          Serial.println("Door Opened via Keypad");
        } else{
          Serial.println("Incorrect Password or Door Already Open via Keypad");
        }
        enteredPassword = "";
      }
    }
  }


  // Check button state
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      // Toggle door state
      toggleDoor();
    }
    // Delay to avoid bouncing
    delay(50);
  }
  lastButtonState = buttonState;


  if (irrecv.decode(&results)) {
    Serial.print("IR Code Received: ");
    Serial.println(results.value, HEX);
    if (results.value == results.value) {
      if (doorIsOpen == false) {
        openDoor();
        Serial.println("Door Opened by IR Remote");
      } else if (doorIsOpen == true) {
        closeDoor();
        Serial.println("Door Closed by IR Remote");
      }
    } else {
      Serial.println("IR Code not recognized or door in target state already");
    }
    irrecv.resume(); // Prepare for the next value
  }


  unsigned long currentMillis = millis();


  // Ultrasonic distance measurement
  if (currentMillis - lastDistanceMeasurementMillis >= distanceMeasurementInterval) {
    lastDistanceMeasurementMillis = currentMillis;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;


    // Adjusted logic for buzzer and LCD based on distance
    if (distance <= 10) {
      lcd.clear();
      lcd.print("Stop!");
      if (!continuousBeepActive) {
        tone(buzzerPin, 1000); // Continuous beep starts
        continuousBeepActive = true;
        previousMillis = currentMillis; // Record the start time of continuous beep
      }
    } else {
      if (continuousBeepActive) {
        noTone(buzzerPin); // Stop the continuous beep
        continuousBeepActive = false;
      }
      // Adjusted else-if condition for distances > 10 cm and <= 20 cm
      if (distance > 10 && distance <= 20) {
        lcd.clear();
        lcd.print("Distance: ");
        lcd.print(distance);
        lcd.print(" cm");
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm ");
        // Single beep logic here if necessary
      } else {
        // Logic for distances > 20 cm
        lcd.clear();
        lcd.print("Distance: ");
        lcd.print(distance);
        lcd.print(" cm");
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm ");
        // Ensure no beep is ongoing
        noTone(buzzerPin);
      }
    }
  }


  int alarmState = digitalRead(alarmPin); // Read the state of the alarm pin
 
  if (alarmState == HIGH) {
    // If the alarm pin is HIGH, sound the buzzer
    tone(buzzerPin, 1000); // Start the buzzer at 1000 Hz
  } else {
    // If the alarm pin is not HIGH, stop the buzzer
    noTone(buzzerPin);
  }


}


void openDoor() {
  if (!doorIsOpen) {
    servo1.write(openPos1);
    servo2.write(openPos2);
    doorIsOpen = true;
  } else {
    Serial.println("Door is already open.");
  }
}


void closeDoor() {
  if (doorIsOpen) {
    servo1.write(closePos1);
    servo2.write(closePos2);
    doorIsOpen = false;
  } else {
    Serial.println("Door is already closed.");
  }
}


void toggleDoor() {
  if (doorIsOpen) {
    closeDoor();
  } else {
    openDoor();
  }
}
