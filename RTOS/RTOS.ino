// ----- PIN SETUP ------------
const uint8_t LED1 = D7;
const uint8_t LED2 = D6;
const uint8_t LED3 = D5;

// === FAN SETUP===
const uint8_t FAN_RELAY_PIN = D4;  

// ----- TIMER ISR SETUP -----------
HardwareTimer *timer1 = new HardwareTimer(TIM2);
volatile bool TimerFlag = false;
volatile uint32_t period_ms = 100;

// ------ Thermistor setup ----------
const uint8_t THERM_PIN = A0;
volatile float tempVolts = 0.0f;
volatile int adc = 0;

// ---- UART on PA10 (RX) / PA9 (TX) ----
#include <HardwareSerial.h>
HardwareSerial ESPUART(PA10, PA9);

// ------- RTOS TASK TYPE ----------
typedef struct task {
  int state; // current state of the task
  unsigned long period; // rate at which the task should tick
  unsigned long elapsedTime;  // time since task's last tick
  int (*function) (int); // function to call for task's tick
} task;

task TL_task;
const int numTasks = 3;        
task tasks[numTasks];

// ----- ENUMS -----------
enum { TL1, TL2 } TL_state;
enum { FAN_OFF_ST, FAN_ON_ST} FAN_state;

// -------- State Machine Declarations ----------
int Test_TaskLEDs(int state);
int TickFct_Therm(int state);
int TickFct_Fan(int state);

// ----- ISR -----------
void TimerISR() {
  for (unsigned char i = 0; i < numTasks; i++) {
    if (tasks[i].elapsedTime >= tasks[i].period) {
      tasks[i].state = tasks[i].function(tasks[i].state);
      tasks[i].elapsedTime = 0;
    }
    tasks[i].elapsedTime += period_ms;
  }
}

// ----- Relay -----
void setRelay(bool on) {
  digitalWrite(FAN_RELAY_PIN, on ? HIGH : LOW);
}

void setup() {
  Serial.begin(9600);
  ESPUART.begin(115200); // UART on PA9/PA10 to ESP32

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(THERM_PIN, INPUT_ANALOG);

  pinMode(FAN_RELAY_PIN, OUTPUT);
  setRelay(false); // start with fan OFF

  // LED task
  tasks[0].state = TL1;
  tasks[0].period = 500;
  tasks[0].elapsedTime = tasks[0].period;
  tasks[0].function = &Test_TaskLEDs;

  // Thermistor task
  tasks[1].state = 0;
  tasks[1].period = 5000;
  tasks[1].elapsedTime = tasks[1].period;
  tasks[1].function = &TickFct_Therm;


  // Fan control task
  tasks[2].state = FAN_OFF_ST;
  tasks[2].period = 200; 
  tasks[2].elapsedTime = tasks[2].period;
  tasks[2].function = &TickFct_Fan;

  timer1->setOverflow(period_ms * 1000UL, MICROSEC_FORMAT);
  timer1->attachInterrupt(TimerISR);
  timer1->resume();

  Serial.println("boot");
}

void loop() {
  // sample ADC
  adc = analogRead(THERM_PIN);
  delay(1500);
}

int Test_TaskLEDs(int state) {
  switch (state) {
    case TL1: digitalWrite(LED1, HIGH); digitalWrite(LED2, LOW);  break;
    case TL2: digitalWrite(LED1, LOW);  digitalWrite(LED2, HIGH); break;
    default:  digitalWrite(LED1, LOW);  digitalWrite(LED2, LOW);  break;
  }
  switch (state) {
    case TL1: state = TL2; break;
    case TL2: state = TL1; break;
    default:  state = TL1; break;
  }
  return state;
}

int TickFct_Therm(int state) {
  int a = adc;
  tempVolts = (a * 3.3f) / 4095.0f;

  // Send to ESP32 
  ESPUART.print("V=");
  ESPUART.println(tempVolts, 2);

  return state;
}



const float FAN_ON_V  = 0.6f; // go ON when tempVolts >= this
const float FAN_OFF_V = 0.58f; // go OFF when tempVolts <= this

int TickFct_Fan(int state) {
  float v = tempVolts;

  switch (state) {
    case FAN_OFF_ST:
      if (v >= FAN_ON_V) {
        setRelay(true);
        Serial.println("Fan: OFF");
        state = FAN_ON_ST;
      }
      break;

    case FAN_ON_ST:
      if (v <= FAN_OFF_V) {
        setRelay(false);
        Serial.println("Fan: ON");
        state = FAN_OFF_ST;
      }
      break;

    default:
      setRelay(false);
      state = FAN_OFF_ST;
      break;
  }

  return state;
}
