// ----- PIN SETUP ------------
const uint8_t LED1 = D7;
const uint8_t LED2 = D6;
const uint8_t LED3 = D5;

// ----- TIMER ISR SETUP -----------
HardwareTimer *timer1 = new HardwareTimer(TIM2);
volatile bool TimerFlag = false;
volatile uint32_t period_ms = 100;

// ------ Thermistor setup ----------
const uint8_t THERM_PIN = A0;        
volatile float tempVolts = 0.0f;
volatile int adc = 0;                

// ---- Explicit UART on PA10 (RX) / PA9 (TX) ----
#include <HardwareSerial.h>
// Order is (RX pin, TX pin):
HardwareSerial ESPUART(PA10, PA9);

// ------- RTOS TASK TYPE ----------
typedef struct task
{
    int state; // currecnt state of the task
    unsigned long period; // rate at which the task should tick
    unsigned long elapsedTime; // time since task's last tick
    int (*function) (int); // function to call for task's tick
}task;

task TL_task;
const int numTasks = 2;
task tasks[numTasks];  //set # of tasks

// ----- ENUMS -----------
enum { TL1, TL2 } TL_state;

// -------- State Machine Declarations ----------
int Test_TaskLEDs(int state);
int TickFct_Therm(int state);

// ----- ISR -----------
void TimerISR() {
  for (unsigned char i = 0; i < numTasks; i++) {
    if (tasks[i].elapsedTime >= tasks[i].period) 
    {
      tasks[i].state = tasks[i].function(tasks[i].state);
      tasks[i].elapsedTime = 0;
    }
    tasks[i].elapsedTime += period_ms;
  }
}

void setup() {
  Serial.begin(9600);                 
  ESPUART.begin(115200);              // UART on PA9/PA10 to ESP32

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(THERM_PIN, INPUT_ANALOG);

  tasks[0].state = TL1;
  tasks[0].period = 500;              // LED task period (ms)
  tasks[0].elapsedTime = tasks[0].period;
  tasks[0].function = &Test_TaskLEDs;

  tasks[1].state = 0;
  tasks[1].period = 5000;             // thermistor task period (ms)
  tasks[1].elapsedTime = tasks[1].period;
  tasks[1].function = &TickFct_Therm;

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

  // Send to ESP32 via PA9/PA10 UART
  ESPUART.print("V=");
  ESPUART.println(tempVolts, 2);

  return state;
}
