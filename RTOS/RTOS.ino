// Rtos code for smart home proj
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>

// Fans
const uint8_t FAN_RELAY_PIN      = D4;  
const uint8_t HOT_FAN_RELAY_PIN  = D3;  

// ----- TIMER ISR SETUP -----------
HardwareTimer *timer1 = new HardwareTimer(TIM2);
volatile uint32_t period_ms = 100;

// ------ Thermistor setup ----------
const uint8_t THERM_PIN = A0;
volatile float tempVolts = 0.0f;
volatile int adc = 0;

// ---- UART on PA10 (RX) / PA9 (TX) ----
HardwareSerial ESPUART(PA10, PA9);

// ---- INA219 Setup ----
Adafruit_INA219 ina219(0x40);  // default I2C address
bool ina_ok = false;

float current_mA = 0;
float shuntvoltage_mV = 0;
float busvoltage_V = 0;
float loadvoltage_V = 0;
float power_mW = 0;

// ------- RTOS TASK TYPE ----------
typedef struct task {
  int state;                      // current state of the task
  unsigned long period;           // rate at which the task should tick
  unsigned long elapsedTime;      // time since task's last tick
  int (*function) (int);          // function to call for task's tick
} task;

const int numTasks = 3;
task tasks[numTasks];

// ----- ENUMS -----------
enum {
  FAN_STATE_BOTH_OFF,
  FAN_STATE_COLD_ON,
  FAN_STATE_HOT_ON
} FAN_state;

// -------- State Machine setup ----------
int TickFct_INA(int state);
int TickFct_Therm(int state);
int TickFct_Fan(int state);
void TimerISR();

//  Relays
void setColdFan(bool on) {
  digitalWrite(FAN_RELAY_PIN, on ? LOW : HIGH);   // relays are active low 
}

void setHotFan(bool on) {
  digitalWrite(HOT_FAN_RELAY_PIN, on ? LOW : HIGH);
}

void setup() {
  // USB Serial for debug
  Serial.begin(9600);
  while (!Serial) { /* wait for USB if needed */ }

  // UART to ESP32
  ESPUART.begin(115200); // UART on PA9/PA10 to ESP32

  // Thermistor pin
  pinMode(THERM_PIN, INPUT_ANALOG);

  // Fan relay pins
  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(HOT_FAN_RELAY_PIN, OUTPUT);

  // Start with both fans OFF
  setColdFan(false);
  setHotFan(false);

  Wire.begin();   

  if (!ina219.begin()) {
    Serial.println("WARNING: INA219 not found, continuing without current readings.");
    ina_ok = false;
  } else {
    ina_ok = true;
    Serial.println("INA219 detected.");
  }

  // Task 0: INA219 current/voltage reading
  tasks[0].state = 0;
  tasks[0].period = 1000;               
  tasks[0].elapsedTime = tasks[0].period;
  tasks[0].function = &TickFct_INA;

  // Task 1: Thermistor reading + send T= to ESP
  tasks[1].state = 0;
  tasks[1].period = 5000;              
  tasks[1].elapsedTime = tasks[1].period;
  tasks[1].function = &TickFct_Therm;

  // Task 2: Fan control state machine
  tasks[2].state = FAN_STATE_BOTH_OFF;
  tasks[2].period = 200;              
  tasks[2].elapsedTime = tasks[2].period;
  tasks[2].function = &TickFct_Fan;

  // Timer interrupt every period_ms
  timer1->setOverflow(period_ms * 1000UL, MICROSEC_FORMAT);
  timer1->attachInterrupt(TimerISR);
  timer1->resume();

  Serial.println("boot");
}

void loop() {
}

//  TIMER ISR 
void TimerISR() {
  for (unsigned char i = 0; i < numTasks; i++) {
    if (tasks[i].elapsedTime >= tasks[i].period) {
      tasks[i].state = tasks[i].function(tasks[i].state);
      tasks[i].elapsedTime = 0;
    }
    tasks[i].elapsedTime += period_ms;
  }
}

//  INA219 TASK 
int TickFct_INA(int state) {
  if (ina_ok) {
    // Sensor Readings
    shuntvoltage_mV = ina219.getShuntVoltage_mV();
    busvoltage_V    = ina219.getBusVoltage_V();
    current_mA      = ina219.getCurrent_mA();
    power_mW        = ina219.getPower_mW();
    loadvoltage_V   = busvoltage_V + (shuntvoltage_mV / 1000.0f);
  } else {
    // if issues with hardware or reading still send 0 
    current_mA = 0.0f;
    Serial.println("INA219 not available, sending I=0.0");
  }

  // Send current to ESP
  ESPUART.print("I=");
  ESPUART.println(current_mA, 1); 

  return state;
}

// Read Temp TASK 
const float V_REF_POINT      = 0.68f;   // volts
const float T_REF_POINT_F    = 71.0f;   // deg F at that voltage
const float V_TO_TEMP_GAIN_F = 150.0f;  // deg F per volt 

int TickFct_Therm(int state) {
  adc = analogRead(THERM_PIN);

  tempVolts = (adc * 3.3f) / 4095.0f;
  float tempF = T_REF_POINT_F + (V_REF_POINT - tempVolts) * V_TO_TEMP_GAIN_F;

  // Send to ESP32 as temp
  ESPUART.print("T=");
  ESPUART.println(tempF, 1);

  Serial.print("ADC=");
  Serial.print(adc);
  Serial.print("  V=");
  Serial.print(tempVolts, 3);
  Serial.print("  T_F=");
  Serial.println(tempF, 1);

  return state;
}

// FAN CONTROL TASK 
const float COLD_FAN_ON_V   = 0.55f;  
const float COLD_FAN_OFF_V  = 0.58f;  

const float HOT_FAN_ON_V    = 0.70f;  
const float HOT_FAN_OFF_V   = 0.67f;  

int TickFct_Fan(int state) {
  float v = tempVolts;

  switch (state) {
    case FAN_STATE_BOTH_OFF:
      setColdFan(false);
      setHotFan(false);

      // Too hot
      if (v <= COLD_FAN_ON_V) {
        setColdFan(true);
        Serial.println("Fan state: COLD ON (too hot)");
        state = FAN_STATE_COLD_ON;
      }
      // Too cold
      else if (v >= HOT_FAN_ON_V) {
        setHotFan(true);
        Serial.println("Fan state: HOT ON (too cold)");
        state = FAN_STATE_HOT_ON;
      }
      break;

    case FAN_STATE_COLD_ON:
      setColdFan(true);
      setHotFan(false);

      if (v >= COLD_FAN_OFF_V) {
        setColdFan(false);
        Serial.println("Fan state: BOTH OFF (cooled into comfy band)");
        state = FAN_STATE_BOTH_OFF;
      }
      break;

    case FAN_STATE_HOT_ON:
      setHotFan(true);
      setColdFan(false);

      if (v <= HOT_FAN_OFF_V) {
        setHotFan(false);
        Serial.println("Fan state: BOTH OFF (warmed into comfy band)");
        state = FAN_STATE_BOTH_OFF;
      }
      break;

    default:
      setColdFan(false);
      setHotFan(false);
      state = FAN_STATE_BOTH_OFF;
      break;
  }

  return state;
}
