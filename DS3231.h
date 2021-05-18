#ifndef DS3231_LIB_HEADER
#define DS3231_LIB_HEADER

#define DS3231_I2C_ADDRESS 0b11010000 // I2C address of the DS3231 (this address is immutable)
#define SYSCLOCKFREQ 8000000 // frequency of the system clock

// defines for setting and reading the day of of the week
#define MONDAY    1
#define TUESDAY   2
#define WEDNESDAY 3
#define THURSDAY  4
#define FRIDAY    5
#define SATURDAY  6
#define SUNDAY    7


// defines for setting the frequency to which the alarm should tigger
#define PER_MINUTE  1
#define PER_HOUR    2
#define PER_DAY     3
#define PER_WEEK    4
#define PER_MONTH   5

// structure for setting the control register
struct DS3231_Init_Struct
{
  uint8_t EnableOscillator; // with this bit the oscillator of the RTC can be enabled or disabled in case the RTC is powered by the backup battery (VBAT). see defines below for possible parameter values
  uint8_t SquareWaveOrInterrupt; // this bit controls whether pin 3 takes on the interrupt functionality or the square wave functionality, see defines below for possible parameter values
  uint8_t BatteryBackedSquareWave; // this bit controls whether the square wave is still outputted/enabled in case the RTC is switched to the backup battery (VBAT), see defines below for possible parameter values
  uint8_t SquareWaveFreq; // this variable controls the frequency of the square wave (that is outputted on pin 3), see defines below for possible parameter values
  uint8_t Alarm1InterruptEnable; // this bit controls whether Alarm 1 will generate an interrupt signal on pin 3 (provided Interrupt is enabled with the SquareWaveOrInterrupt bit), see defines below for possible parameter values
  uint8_t Alarm2InterruptEnable; // this bit controls whether Alarm 2 will generate an interrupt signal on pin 3 (provided Interrupt is enabled with the SquareWaveOrInterrupt bit), see defines below for possible parameter values
};

// possible parameter values for the EnableOscillator field of the DS3231_Init_Struct
#define DISABLE_OSC 0b10000000
#define ENABLE_OSC 0

// possible parameter values for the SquareWaveOrInterrupt field of the DS3231_Init_Struct
#define SQUAREWAVE_FUNC 0
#define INTERRUPT_FUNC 0b00000100

// possible parameter values for the BatteryBackedSquareWave field of the DS3231_Init_Struct
#define BBSW_DISABLE 0
#define BBSW_ENABLE 0b01000000

// possible parameter values for the SquareWaveFreq field of the DS3231_Init_Struct
#define SWFREQ_1HZ    0
#define SWFREQ_1024HZ 0b00001000
#define SWFREQ_4096HZ 0b00010000
#define SWFREQ_8192HZ 0b00011000

// possible parameter values for the Alarm1InterruptEnable field of the DS3231_Init_Struct
#define ALARM1_INT_DISABLE 0
#define ALARM1_INT_ENABLE 0b00000001

// possible parameter values for the Alarm2InterruptEnable field of the DS3231_Init_Struct
#define ALARM2_INT_DISABLE 0
#define ALARM2_INT_ENABLE 0b00000010

// public function prototypes (for using the DS3231 lib)
uint8_t DS3231_PutInKnownI2CState(void);
uint8_t DS3231_Init(struct DS3231_Init_Struct* pStruct);
uint8_t DS3231_SetTime(uint8_t seconds, uint8_t minutes, uint8_t hours);
uint8_t DS3231_ReadTime(uint8_t* seconds, uint8_t* minutes, uint8_t* hours);
uint8_t DS3231_SetDate(uint8_t day_of_month, uint8_t month, uint8_t year);
uint8_t DS3231_ReadDate(uint8_t* day_of_month, uint8_t* month, uint8_t* year);
uint8_t DS3231_SetAlarm1(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t day, uint8_t day_of_month);
uint8_t DS3231_ReadAlarm1Flag(void);
uint8_t DS3231_ClearAlarm1Flag(void);
uint8_t DS3231_SetAlarm2(uint8_t minutes, uint8_t hours, uint8_t day, uint8_t day_of_month);
uint8_t DS3231_ReadAlarm2Flag(void);
uint8_t DS3231_ClearAlarm2Flag(void);

#endif
