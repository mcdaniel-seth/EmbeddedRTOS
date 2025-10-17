// ----- PIN SETUP ------------
const uint8_t LED1 = PA0;
const uint8_t LED2 = PA1;
const uint8_t LED3 = PA2;
// ----- TIMER ISR SETUP -----------
HardwareTimer *tickTim = new HardwareTimer(TIM2);
volatile bool tick = false;
volatile uint32_t period_ms = 1000;

// ------- ENUMS ----------
enum SM_states{SM_led1, SM_led2, SM_led3}state;

void onTick() { // ISR
  tick = true;  // everytime ISR hits, just rest flag
}

void setup() {
  //state = SM_led1;
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  tickTim->setOverflow(period_ms * 1000UL, MICROSEC_FORMAT);  // set period for timer
  tickTim->attachInterrupt(onTick);   // link ISR to func
  tickTim->resume(); 
}

void loop() {
  while (!tick);
  noInterrupts();
  tick = false;
  interrupts();
  SM();
}


void SM(){
  static uint8_t state = SM_led1; // initial state

  switch (state) {
    case SM_led1:
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      break;
    case SM_led2:
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, LOW);
      break;
    case SM_led3: // 2
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, HIGH);
      break;
    default:
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      break;

  }

  switch (state) {
    case SM_led1:
      state = SM_led2;
      break;
    case SM_led2:
      state = SM_led3;
      break;
    case SM_led3: // 2
      state = SM_led1;
      break;
    default:
      state = SM_led1;
      break;

  }

}