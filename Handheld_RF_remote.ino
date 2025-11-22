#include <RCSwitch.h>

// https://github.com/rocketscream/Low-Power
#include <LowPower.h>

#include <Bounce2.h>

// Pin definitions 
#define PIN_BTN_FAN_OFF     3
#define PIN_BTN_SPEED_INC   2
#define PIN_BTN_SPEED_DEC   4
#define PIN_BTN_FAN_DIR     5
#define PIN_BTN_LIGHT       6


#define MAX_FANS 5
#define MAX_CODE_LENGTH 25


enum FanLocation 
{
  DiningArea = 0,
  Study,
  TVArea,
  MasterBedroom,
  MusicRoom
};

typedef struct 
{
    char Fan_Id[20];
    int Pulse_Length;
    int Repeat_Tx;
    char Fan_OFF[MAX_CODE_LENGTH];
    char Fan_Speed_1[MAX_CODE_LENGTH];
    char Fan_Speed_2[MAX_CODE_LENGTH];
    char Fan_Speed_3[MAX_CODE_LENGTH];
    char Fan_Speed_4[MAX_CODE_LENGTH];
    char Fan_Speed_5[MAX_CODE_LENGTH];
    char Light_ON[MAX_CODE_LENGTH];
    char Light_OFF[MAX_CODE_LENGTH];
    char Timer_1h[MAX_CODE_LENGTH];
    char Timer_2h[MAX_CODE_LENGTH];
    char Timer_4h[MAX_CODE_LENGTH];
    char Timer_8h[MAX_CODE_LENGTH];
    char Dir_Fwd_Rev[MAX_CODE_LENGTH];
} FanCode;

const FanCode fanCodes[] PROGMEM = 
{
    {"Dining Area", 290, 10, "111100000000011100000001", "111100000000011100000000", "111100000000011100010101", "111100000000011100001101", "111100000000011100001001", "111100000000011100001010", "111100000000011110111100", "111100000000011110111100", "111100000000011100010110", "111100000000011100010100", "111100000000011100000110", "111100000000011100001000", "111100000000011100001110"},
    {"Study", 290, 10, "111100110110110100000001", "111100110110110100000000", "111100110110110100010101", "111100110110110100001101", "111100110110110100001001", "111100110110110100001010", "111100110110110110111100", "111100110110110110111100", "111100110110110100010110", "111100110110110100010100", "111100110110110100000110", "111100110110110100001000", "111100110110110100001110"},
    {"TV Area", 290, 10, "101110010001010100000001", "101110010001010100000000", "101110010001010100010101", "101110010001010100001101", "101110010001010100001001", "101110010001010100001010", "101110010001010110111100", "101110010001010110111100", "101110010001010100010110", "101110010001010100010100", "101110010001010100000110", "101110010001010100001000", "101110010001010100001110"},
    {"Master Bedroom", 290, 10, "100101111000101000000001", "100101111000101000000000", "100101111000101000010101", "100101111000101000001101", "100101111000101000001001", "100101111000101000001010", "100101111000101010111100", "100101111000101010111100", "100101111000101000010110", "100101111000101000010100", "100101111000101000000110", "100101111000101000001000", "100101111000101000001110"},
    {"Music Room", 260, 10, "000001111100110000110000", "000001111100110011110011", "", "000001111100110000110011", "", "000001111100110011001100", "000001111100110000001100", "000001111100110000000011", "000001111100110011110000", "000001111100110000001111", "000001111100110011000011", "000001111100110011000000", ""}

};

RCSwitch mySwitch = RCSwitch();

Bounce debBtnOff = Bounce();
Bounce debBtnSpdInc = Bounce();
Bounce debBtnSpdDec = Bounce();
Bounce debBtnFanDir = Bounce();
Bounce debBtnLight = Bounce();

char buffer[MAX_CODE_LENGTH];

// Pin range for wakeup
const byte wakePins[] = {2};

int fan_id = DiningArea;

int fan_speed = 0;

void init_inputs(void)
{

  /* Configure pin direction, internal pull-ups, etc*/   
  pinMode(PIN_BTN_FAN_OFF, INPUT_PULLUP);
  pinMode(PIN_BTN_SPEED_INC, INPUT_PULLUP);
  pinMode(PIN_BTN_SPEED_DEC, INPUT_PULLUP);
  pinMode(PIN_BTN_FAN_DIR, INPUT_PULLUP);
  pinMode(PIN_BTN_LIGHT, INPUT_PULLUP);

  debBtnOff.attach(PIN_BTN_FAN_OFF);
  debBtnOff.interval(1);  // debounce time in ms

  debBtnSpdInc.attach(PIN_BTN_SPEED_INC);
  debBtnSpdInc.interval(1);

  debBtnSpdDec.attach(PIN_BTN_SPEED_DEC);
  debBtnSpdDec.interval(1);

  debBtnFanDir.attach(PIN_BTN_FAN_DIR);
  debBtnFanDir.interval(1);

  debBtnLight.attach(PIN_BTN_LIGHT);
  debBtnLight.interval(1);
}

void init_interrupts(void)
{
  // Attach interrupt on INT0 (pin 2) to trigger on LOW level
  //attachInterrupt(digitalPinToInterrupt(2), wakeUpISR, LOW);
}

void setup_RfTx()
{

  //Decimal: 15730433 (24Bit) Binary: 111100000000011100000001 Tri-State: 110000F1000F PulseLength: 297 microseconds Protocol: 1

  // Transmitter is connected to Arduino Pin D7 (pin 13 on AT Mega 328)
  mySwitch.enableTransmit(7);

  // Set Protocol (default is 1, will work for most outlets)
  mySwitch.setProtocol(1); 

  // Set pulse length 
  // NB Pulse length must be set AFTER Protocol, 
  // because setProtocol(1) also sets pulse length = 350
  
  // Pulse Length = 260 [Piano Room]
  //mySwitch.setPulseLength(260);

  // Pulse Length = 290 [Fanco Fans]
  mySwitch.setPulseLength(290);

  // Optional set number of transmission repetitions.
  // Mine seem to work with 2, yours may need more
  mySwitch.setRepeatTransmit(15);  
}

void setup() 
{
  //pinMode(7, OUTPUT); // Set D2 as a digital output

  Serial.begin(9600);  // Start UART at 9600 baud

  init_inputs();

  // Initialize and enable inerrupts
  //init_interrupts();

  // Configure fan_id 
  fan_id = Study;

  // Setup RF communication
  setup_RfTx();
  
  Serial.println("RF remote init !");
}

void rf_tx_fan_speed()
{
  char tx_buffer[MAX_CODE_LENGTH];

  Serial.print("fan_speed: ");
  Serial.println(fan_speed);


  switch(fan_speed)
  {
    case 0:
      strcpy_P(tx_buffer, fanCodes[fan_id].Fan_OFF);  
    break;
    
    case 1:
      strcpy_P(tx_buffer, fanCodes[fan_id].Fan_Speed_1);  
    break;

    case 2:
      strcpy_P(tx_buffer, fanCodes[fan_id].Fan_Speed_2);  
    break;

    case 3:
      strcpy_P(tx_buffer, fanCodes[fan_id].Fan_Speed_3);  
    break;

    case 4:
      strcpy_P(tx_buffer, fanCodes[fan_id].Fan_Speed_4);  
    break;

    case 5:
      strcpy_P(tx_buffer, fanCodes[fan_id].Fan_Speed_5);  
    break;
  }

  // Send RF command to fan
  mySwitch.send(tx_buffer); 
}

void loop() 
{

    /* Debounce button inputs */
    debBtnOff.update();
    debBtnSpdInc.update();
    debBtnSpdDec.update();
    debBtnFanDir.update();
    debBtnLight.update();
    

    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pin is low.
    //LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 

    if(debBtnOff.fell())
    {
      fan_speed = 0;

      strcpy_P(buffer, fanCodes[fan_id].Fan_OFF);
      
      // Send RF command to fan
      mySwitch.send(buffer);    
    }

    
    if(debBtnSpdInc.fell())
    {
      if(fan_speed < 5)
      {
        fan_speed++;

        /* RF TX fan command */
        rf_tx_fan_speed();
      }


    }

    if(debBtnSpdDec.fell())
    {
      if(fan_speed > 0)
      {
        fan_speed--;

        /* RF TX fan command */
        rf_tx_fan_speed();        
      }

    }


#if 0
    delay(1000);

    char tx_buffer[MAX_CODE_LENGTH];

    strcpy_P(tx_buffer, fanCodes[Study].Fan_Speed_1);
    mySwitch.send(tx_buffer); 

    Serial.println("RF TX");
    Serial.print(tx_buffer);
#endif

}

void wakeUpISR() 
{

}

