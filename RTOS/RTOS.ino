// ----- PIN SETUP ------------
const uint8_t LED1 = D7;
const uint8_t LED2 = D6;
const uint8_t LED3 = D5;

// ----- TIMER ISR SETUP -----------
HardwareTimer *timer1 = new HardwareTimer(TIM2);
volatile bool TimerFlag = false;
volatile uint32_t period_ms = 100;

// ------ Thermisot setuo ----------
const uint8_t THERM_PIN = PA4;        // use any ADC-capable pin
const float VCC = 3.3f;
const int ADC_MAX = 4095;             // 12-bit ADC
const float R_FIXED = 10000.0f;       // 10k resistor
const float BETA = 3950.0f;           // thermistor Beta constant
const float T0K  = 298.15f;           // 25°C in Kelvin
const float R0   = 10000.0f; 

// ------- RTOS TASK TYPE ----------
typedef struct task
{
    int state; // currecnt state of the task
    unsigned long period; // rate at which the task should tick
    unsigned long elapsedTime; // time since task's last tick
    int (*function) (int); // function to call for task's tick
}task;

task TL_task;

task tasks[2];  //set # of tasks

// ----- ENUMS -----------
enum {TL1, TL2}TL_state;

// -------- State Machine Declarartions ----------
int Test_TaskLEDs(int state);


// ----- ISR -----------
void TimerISR() { 
  TimerFlag = true;  // everytime ISR hits, just rest flag
}

void setup() {
  Serial.begin(9600);
  //state = SM_led1;
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  // test serial out thermistoe
  Serial.begin(9600);
  pinMode(THERM_PIN, INPUT_ANALOG);

  tasks[0].state = TL1;
  tasks[0].period = 500;          // this task runs every 1000 ms
  tasks[0].elapsedTime = tasks[0].period;
  tasks[0].function = &Test_TaskLEDs;

  tasks[1].state = 0;
  tasks[1].period = 500;                 // read every 0.5 s
  tasks[1].elapsedTime = tasks[1].period;
  tasks[1].function = &TickFct_Therm;

  timer1->setOverflow(period_ms * 1000UL, MICROSEC_FORMAT);  // period for timer , it wants in microsec and period is in ms so *1000UL is micro
  timer1->attachInterrupt(TimerISR);   // link ISR to func
  timer1->resume(); 
}

void loop() {
  while (!TimerFlag);
  noInterrupts();
  TimerFlag = false;
  interrupts();

  if(tasks[0].elapsedTime >= tasks[0].period)
    {
        tasks[0].state = tasks[0].function(tasks[0].state);
        tasks[0].elapsedTime = 0;
    }
  tasks[0].elapsedTime += period_ms;

  if(tasks[1].elapsedTime >= tasks[1].period)
    {
        tasks[1].state = tasks[1].function(tasks[1].state);
        tasks[1].elapsedTime = 0;
    }
  tasks[1].elapsedTime += period_ms;


}


int Test_TaskLEDs(int state){

  switch (state) {
    case TL1:
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, LOW);
      break;
    case TL2:
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, HIGH);
      break;
    default:
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      break;

  }

  switch (state) {
    case TL1:
      state = TL2;
      break;
    case TL2:
      state = TL1;
      break;
    default:
      state = TL1;
      break;

  }

  return state;

}


int TickFct_Therm(int state) {
  long sum = 0;
  const int N = 8; 
  for (int i = 0; i < N; ++i) {
    sum += analogRead(THERM_PIN);
  }
  int adc = sum / N;


  float v = (adc * VCC) / ADC_MAX;

  float rTherm = R_FIXED * (v / (VCC - v));

  float invT = (1.0f / T0K) + (1.0f / BETA) * log(rTherm / R0);
  float tempK = 1.0f / invT;
  float tempC = tempK - 273.15f;

  Serial.print("Temp: ");
  Serial.print(tempC, 2); 
  Serial.println(" °C");

  return state; 
}
