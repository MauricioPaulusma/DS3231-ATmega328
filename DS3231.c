#include "Gpio.h"
#include "Timer.h"
#include "WDT.h"
#include "DS3231.h"
#include "TWI.h"
#include "USART.h"
#include <stdio.h>

#define SECONDS_ADDRESS 0x00
#define MINUTES_ADDRESS 0x01
#define HOURS_ADDRESS 0x02
#define DAY_ADDRESS 0x03
#define DATE_ADDRESS 0x04
#define MONTH_ADDRESS 0x05
#define YEAR_ADDRESS 0x06

#define ALARM1_SEC_ADDRESS 0x07
#define ALARM1_MIN_ADDRESS 0x08
#define ALARM1_HOUR_ADDRESS 0x09
#define ALARM1_DYDT_ADDRESS 0x0A

#define ALARM2_MIN_ADDRESS 0x0B
#define ALARM2_HOUR_ADDRESS 0x0C
#define ALARM2_DYDT_ADDRESS 0x0D

#define CTRL_ADDRESS 0x0E
#define STATUS_ADDRESS 0x0F


//#define DEBUG_DS3231 1 // uncomment this define for activating the debug output via the USART0

////////////////////////////////////////////////////////////////////////////////////////
// Name: DS3231_PutInKnownI2CState
// Description: This function puts the I2C driver of the DS3231 in its default state.
//              This may be necessary in case the microcontroller resets during a communication
//              with the DS3231. In that case the DS3231 I2C driver is locked into the state from
//              during the communication and as a result will not respond to a new start condition.
//              The routing of this function resets this lockup.
// Arguments: none
//
// Returns:
//  - 0: if DS3231 is succesfully put in its default I2C state
//  - 1: if a timeout error occured
uint8_t DS3231_PutInKnownI2CState(void)
{
  GPIO_PinMode(GPIOC, GPIO_PIN_4, GPIO_INPUT, GPIO_PULLUP);
  if(GPIO_ReadPin(GPIOC, GPIO_PIN_4))
    return 0;
  else
  {
    GPIO_PinMode(GPIOC, GPIO_PIN_5, GPIO_OUTPUT, GPIO_NOPULLUP);
    WDT_Init();
    while (GPIO_ReadPin(GPIOC, GPIO_PIN_4) == 0)
    {
      if(WDT_Return_Flag())
      {
        WDT_Reset_Flag();
        return 1;
      }
      GPIO_TogglePin(GPIOC, GPIO_PIN_5);
      Timer0_Delay_us(1000);
    }
    WDT_Disable();
    return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////////////
// Name: DS3231_Init
// Description: This function writes the control register of the DS3231
// Arguments:
//  - struct DS3231_Init_Struct* pStruct: pointer to a DS3231_Init_Struct that contains
//           the parameters for configuring the control register
//
// Returns:
//  - 0: if control register was succesfully written
//  - 1: if a timeout error occured in the TWI driver
//  - else: other codes represent specific TWI status errors, see TWI chapter in
//          ATmega328 datasheet for the meaning of the code
uint8_t DS3231_Init(struct DS3231_Init_Struct* pStruct)
{
  I2C_Init(SYSCLOCKFREQ, I2C_NORMAL_SPEED_MODE, I2C_PULLUP_EN);
  uint8_t ctrl_dat = pStruct->EnableOscillator | pStruct->SquareWaveOrInterrupt | pStruct->BatteryBackedSquareWave | pStruct->SquareWaveFreq | pStruct->Alarm1InterruptEnable | pStruct->Alarm2InterruptEnable;
  uint8_t ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, CTRL_ADDRESS, ctrl_dat);
  if(ret != 0)
    return ret;

  uint8_t buf;
  I2C_Read_From_Slave(DS3231_I2C_ADDRESS, CTRL_ADDRESS, &buf);
  char buffer[100];
  snprintf(&buffer[0], 100, "value in CTRL-reg: %X\n", buf);
  USART_PrintString(buffer);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Name: DS3231_SetTime
// Description: This function sets the time in the timekeeping registers of the
//              DS3231.
// Arguments:
//  - uint8_t seconds: seconds that will be stored in seconds registers
//  - uint8_t minutes: minutes that will be stored in minutes registers
//  - uint8_t hours: hours that will be stored in hours registers
//
// Returns:
//  - 0: if time was succesfully set
//  - 1: if a timeout error occured in the TWI driver
//  - else: other codes represent specific TWI status errors, see TWI chapter in
//          ATmega328 datasheet for the meaning of the code
uint8_t DS3231_SetTime(uint8_t seconds, uint8_t minutes, uint8_t hours)
{
  uint8_t ret;

  if(seconds > 59)
    return 2;
  ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, SECONDS_ADDRESS, ((seconds%10) | ((seconds/10)<<4)));
  if(ret != 0)
    return ret;

  if(minutes > 59)
    return 3;
  ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, MINUTES_ADDRESS, ((minutes%10) | ((minutes/10)<<4)));
  if(ret != 0)
    return ret;

  if(hours > 23)
    return 4;
  ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, HOURS_ADDRESS, ((hours%10) | ((hours/10)<<4)));
  if(ret != 0)
    return ret;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Name: DS3231_ReadTime
// Description: This function reads the time from the timekeeping registers in the
//              DS3231.
// Arguments:
//  - uint8_t* seconds: pointer to a uint8_t variable to store the read seconds
//  - uint8_t* minutes: pointer to a uint8_t variable to store the read minutes
//  - uint8_t* hours:   pointer to a uint8_t variable to store the read hours
//
// Returns:
//  - 0: if time was succesfully read
//  - 1: if a timeout error occured in the TWI driver
//  - else: other codes represent specific TWI status errors, see TWI chapter in
//          ATmega328 datasheet for the meaning of the code
uint8_t DS3231_ReadTime(uint8_t* seconds, uint8_t* minutes, uint8_t* hours)
{
  uint8_t ret;
  uint8_t buf;

  if(seconds != 0)
  {
    ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, SECONDS_ADDRESS, &buf);
    if(ret != 0)
      return ret;
    else
    {
      *seconds = (buf&0x0F) + (((buf&0x70)>>4)*10);
    }
  #ifdef DEBUG_DS3231
    USART_PrintString("seconds succesfully read\n");
  #endif
  }

  if(minutes != 0)
  {
    ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, MINUTES_ADDRESS, &buf);
    if(ret != 0)
      return ret;
    else
    {
      *minutes = (buf&0x0F) + (((buf&0x70)>>4)*10);
    }
    #ifdef DEBUG_DS3231
      USART_PrintString("minutes succesfully read\n");
    #endif
  }

  if(hours != 0)
  {
    ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, HOURS_ADDRESS, &buf);
    if(ret != 0)
      return ret;
    else
    {
      *hours = (buf&0x0F) + (((buf&0x30)>>4)*10);
    }
  #ifdef DEBUG_DS3231
    USART_PrintString("hours succesfully read\n");
  #endif
  }
  return 0;

}

////////////////////////////////////////////////////////////////////////////////////////
// Name: DS3231_SetDate
// Description: This function sets the date in the datekeeping registers in the
//              DS3231.
// Arguments:
//  - uint8_t day_of_month: day of the month will be stored in register 0x04
//  - uint8_t month: month will be stored in register 0x05
//  - uint8_t year: year will be stored in register 0x06
//
// Returns:
//  - 0: if date was succesfully send
//  - 1: if a timeout error occured in the TWI driver
//  - else: other codes represent specific TWI status errors, see TWI chapter in
//          ATmega328 datasheet for the meaning of the code
uint8_t DS3231_SetDate(uint8_t day_of_month, uint8_t month, uint8_t year)
{
  uint8_t ret;

  if(day_of_month > 31)
    return 2;
  ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, DATE_ADDRESS, ((day_of_month%10) | ((day_of_month/10)<<4)));
  if(ret != 0)
    return ret;

  if(month > 12)
    return 3;
  ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, MONTH_ADDRESS, ((month%10) | ((month/10)<<4)));
  if(ret != 0)
    return ret;

  if(year > 99)
    return 4;
  ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, YEAR_ADDRESS, ((year%10) | ((year/10)<<4)));
  if(ret != 0)
    return ret;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// Name: DS3231_ReadDate
// Description: This function reads the date from the datekeeping registers in the
//              DS3231.
// Arguments:
//  - uint8_t* day_of_month: pointer to a uint8_t variable to store the read day of month
//  - uint8_t* month: pointer to a uint8_t variable to store the read month
//  - uint8_t* year: pointer to a uint8_t variable to store the read year
//
// Returns:
//  - 0: if date was succesfully read
//  - 1: if a timeout error occured in the TWI driver
//  - else: other codes represent specific TWI status errors, see TWI chapter in
//          ATmega328 datasheet for the meaning of the code
uint8_t DS3231_ReadDate(uint8_t* day_of_month, uint8_t* month, uint8_t* year)
{
  uint8_t ret;
  uint8_t buf;

  if(day_of_month != 0)
  {
    ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, DATE_ADDRESS, &buf);
    if(ret != 0)
      return ret;
    else
    {
      *day_of_month = (buf&0x0F) + (((buf&0x30)>>4)*10);
    }
    #ifdef DEBUG_DS3231
    USART_PrintString("day of month succesfully read\n");
    #endif
  }

  if(month != 0)
  {
    ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, MONTH_ADDRESS, &buf);
    if(ret != 0)
      return ret;
    else
    {
      *month = (buf&0x0F) + (((buf&0x10)>>4)*10);
    }
    #ifdef DEBUG_DS3231
      USART_PrintString("month succesfully read\n");
    #endif
  }

  if(year != 0)
  {
    ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, YEAR_ADDRESS, &buf);
    if(ret != 0)
      return ret;
    else
    {
      *year = (buf&0x0F) + (((buf&0xF0)>>4)*10);
    }
  #ifdef DEBUG_DS3231
    USART_PrintString("year succesfully read\n");
  #endif
  }

  return 0;

}

////////////////////////////////////////////////////////////////////////////////////////////////
// Name: DS3231_SetAlarm1
// Description: This function sets Alarm 1. With the parameters (seconds, minutes, etc..) the
//              exact time alarm 1 should trigger can be configured, see Arguments for valid values.
//              In addition the frequency on which Alarm 1 triggers can also be configured:
//              - 1. seconds normal value, rest 255: Alarm 1 triggers each time seconds match
//                   with the time (minute interval).
//              - 2. seconds and minutes normal value. rest 255: Alarm 1 triggers each time seconds AND
//                   minutes match with the time (hour interval).
//              - 3. seconds, minutes, and hours normal value, rest 255: Alarm 1 triggers each time seconds
//                   AND minutes AND hours match with the time (daily interval).
//              - 4. seconds, minutes, hours, day normal value, day_of_month 255: Alarm 1 triggers each time
//                   seconds AND minutes AND hours AND day of the week match the time (weekly interval).
//              - 5. seconds, minutes, hours, day_of_month normal values, day 255: Alarm 1 triggers each time
//                   seconds, minutes, hours and day of month match the timer (monthly interval).
//              - 6. Each parameter 255: alarm 1 triggers every second.
//
// Arguments:
//  - uint8_t seconds: the second on which alarm 1 should trigger. This parameter can have
//                     a value of 0 to 59 OR a frequency configure value of 255 see Description.
//  - uint8_t minutes: the minute on which alarm 1 should trigger. This parameter can have
//                     a value of 0 to 59 OR a frequency configure value of 255 see Description.
//  - uint8_t hours: the hour on which alarm 1 should trigger. This parameter can have
//                   a value of 0 to 23 OR a frequency configure value of 255 see Description.
//  - uint8_t day: the day of the week on which alarm 1 should trigger. This parameter can have
//                 a value of 1 to 7 (monday = 1, etc..) OR a frequency configure value of 255 see Description.
//  - uint8_t day_of_month: the day of the month alarm 1 should trigger. This parameter can have a
//                          a value of 1 to 31 OR a frequency configure value of 255 see Description.
// Returns:
//  - 0: if alarm 1 was succesfully set
//  - 1: if a timeout error occured in the TWI driver
//  - else: other codes represent specific TWI status errors, see TWI chapter in
//          ATmega328 datasheet for the meaning of the code
uint8_t DS3231_SetAlarm1(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t day, uint8_t day_of_month)
{
  uint8_t ret;
  if(seconds < 60)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_SEC_ADDRESS, ((seconds%10) | ((seconds/10)<<4)));
    if(ret != 0)
      return ret;
  }
  else if(seconds == 255)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_SEC_ADDRESS, 0x80); // writes a 1 to bit 7 (A1M1)
    if(ret != 0)
      return ret;
  }
  else
    return 2; // signal that an invalid value has been given to the seconds parameter

  if(minutes < 60)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_MIN_ADDRESS, ((minutes%10) | ((minutes/10)<<4)));
    if(ret != 0)
      return ret;
  }
  else if(minutes == 255)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_MIN_ADDRESS, 0x80); // writes a 1 to bit 7 (A1M2) so the alarm doesnt use the minutes for the alarm
    if(ret != 0)
      return ret;
  }
  else
    return 3; // signal that an invalid value has been given to the minutes parameter

  if(hours < 24)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_HOUR_ADDRESS, ((hours%10) | ((hours/10)<<4)));
    if(ret != 0)
      return ret;
  }
  else if(hours == 255)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_HOUR_ADDRESS, 0x80); // writes a 1 to bit 7 (A1M3) so the alarm doesnt use the hours for the alarm
    if(ret != 0)
      return ret;
  }
  else
    return 4; // signal that an invalid value has been given to the hours parameter

  if(day <= 7 && day_of_month <= 31)
    return 5; // signal than an ambigous values has been given to the parameters day and day_of_month: not clear wheter alarm should trigger every day of week or every day of month
  else if(day <= 7 && day_of_month == 255) // in this case the day of the week will be used for the alarm
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_DYDT_ADDRESS, day | 0x40); // write the day to the register and write a 1 to the DY/DT bit to indicate that the value written indicates the day of the week
    if(ret != 0)
      return ret;
  }
  else if(day == 255 && day_of_month <= 31) // in this case the day of the month will be used for the alarm
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_DYDT_ADDRESS, ((day_of_month%10) | ((day_of_month/10)<<4)) ); // write the day to the register and write a 0 to the DY/DT bit to indicate that the value written indicates the day of the month
    if(ret != 0)
      return ret;
  }
  else if(day == 255 && day_of_month == 255) //in this case day of week or day of month will not be used for the alarm
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM1_DYDT_ADDRESS, 0x80); // write a 1 to bit 7 (A1M4) to indicate to not use the day of week/month for the alarm
    if(ret != 0)
      return ret;
  }
  else // other values are invalid
    return 6; // signal than invalid values for both or one of the parameters is given

  return 0; // signal alarm 1 has been set succesfully
}

uint8_t DS3231_ReadAlarm1Flag(void)
{
  uint8_t ret, buf;
  ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, STATUS_ADDRESS, &buf);
  if(ret == 1)
    return 2; // signal a timeout error
  if(ret != 0) // else it is a TWI error code
    return ret; // signal the TWI error code

  if((buf & 0x01) == 0x01)
    return 1; // signal that the flag is high
  else
    return 0; // signal that the flag is low
}

uint8_t DS3231_ClearAlarm1Flag(void)
{
  uint8_t ret, buf;
  ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, STATUS_ADDRESS, &buf); // read the status register
  if(ret != 0) // else it is a TWI error code
    return ret; // signal the TWI error code

  buf = buf & 0b11111110; // clear the alarm 1 flag in the status register

  ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, STATUS_ADDRESS, buf); // write the status reg back
  if(ret != 0)
    return ret;

  return 0; // signal that the alarm 1 flag has been succesfully cleared
}

uint8_t DS3231_ReadAlarm2Flag(void)
{
  uint8_t ret, buf;
  ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, STATUS_ADDRESS, &buf);
  if(ret == 1)
    return 2; // signal a timeout error
  if(ret != 0) // else it is a TWI error code
    return ret; // signal the TWI error code

  if((buf & 0x02) == 0x02)
    return 1; // signal that the flag is high
  else
    return 0; // signal that the flag is low
}

uint8_t DS3231_ClearAlarm2Flag(void)
{
  uint8_t ret, buf;
  ret = I2C_Read_From_Slave(DS3231_I2C_ADDRESS, STATUS_ADDRESS, &buf); // read the status register
  if(ret != 0) // else it is a TWI error code
    return ret; // signal the TWI error code

  buf = buf & 0b11111101; // clear the alarm 2 flag in the status register

  ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, STATUS_ADDRESS, buf); // write the status reg back
  if(ret != 0)
    return ret;

  return 0; // signal that the alarm 2 flag has been succesfully cleared
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Name: DS3231_SetAlarm2
// Description: This function sets alarm 2. With the parameters (minutes, hours etc..) the
//              exact time alarm 2 should trigger can be configured, see Arguments for valid values.
//              In addition the frequency on which Alarm 2 triggers can also be configured, different
//              frequency modes are available:
//              - 1. minutes normal value. rest 255: Alarm 2 triggers each time
//                   minutes match with the time (hour interval).
//              - 2. minutes, and hours normal value, rest 255: Alarm 2 triggers each time
//                   minutes AND hours match with the time (daily interval).
//              - 3. minutes, hours, day normal value, day_of_month 255: Alarm 2 triggers each time
//                   minutes AND hours AND day of the week match the time (weekly interval).
//              - 4. minutes, hours, day_of_month normal values, day 255: Alarm 2 triggers each time
//                   minutes AND hours AND day of month match the timer (monthly interval).
//              - 5. Each parameter 255: alarm 2 triggers every minute.
//
// Arguments:
//  - uint8_t minutes: the minute on which alarm 1 should trigger. This parameter can have
//                     a value of 0 to 59 OR a frequency configure value of 255 see Description.
//  - uint8_t hours: the hour on which alarm 1 should trigger. This parameter can have
//                   a value of 0 to 23 OR a frequency configure value of 255 see Description.
//  - uint8_t day: the day of the week on which alarm 1 should trigger. This parameter can have
//                 a value of 1 to 7 (monday = 1, etc..) OR a frequency configure value of 255 see Description.
//  - uint8_t day_of_month: the day of the month alarm 1 should trigger. This parameter can have a
//                          a value of 1 to 31 OR a frequency configure value of 255 see Description.
// Returns:
//  - 0: if alarm 1 was succesfully set
//  - 1: if a timeout error occured in the TWI driver
//  - else: other codes represent specific TWI status errors, see TWI chapter in
//          ATmega328 datasheet for the meaning of the code
uint8_t DS3231_SetAlarm2(uint8_t minutes, uint8_t hours, uint8_t day, uint8_t day_of_month)
{
  uint8_t ret;
  if(minutes < 60)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM2_MIN_ADDRESS, ((minutes%10) | ((minutes/10)<<4)));
    if(ret != 0)
      return ret;
  }
  else if(minutes == 255)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM2_MIN_ADDRESS, 0x80); // writes a 1 to bit 7 (A1M2) so the alarm doesnt use the minutes for the alarm
    if(ret != 0)
      return ret;
  }
  else
    return 3; // signal that an invalid value has been given to the minutes parameter

  if(hours < 24)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM2_HOUR_ADDRESS, ((hours%10) | ((hours/10)<<4)));
    if(ret != 0)
      return ret;
  }
  else if(hours == 255)
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM2_HOUR_ADDRESS, 0x80); // writes a 1 to bit 7 (A1M3) so the alarm doesnt use the hours for the alarm
    if(ret != 0)
      return ret;
  }
  else
    return 4; // signal that an invalid value has been given to the hours parameter

  if(day <= 7 && day_of_month <= 31)
    return 5; // signal than an ambigous values has been given to the parameters day and day_of_month: not clear wheter alarm should trigger every day of week or every day of month
  else if(day <= 7 && day_of_month == 255) // in this case the day of the week will be used for the alarm
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM2_DYDT_ADDRESS, day | 0x40); // write the day to the register and write a 1 to the DY/DT bit to indicate that the value written indicates the day of the week
    if(ret != 0)
      return ret;
  }
  else if(day == 255 && day_of_month <= 31) // in this case the day of the month will be used for the alarm
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM2_DYDT_ADDRESS, ((day_of_month%10) | ((day_of_month/10)<<4)) ); // write the day to the register and write a 0 to the DY/DT bit to indicate that the value written indicates the day of the month
    if(ret != 0)
      return ret;
  }
  else if(day == 255 && day_of_month == 255) //in this case day of week and day of month will not be used for the alarm
  {
    ret = I2C_Write_To_Slave(DS3231_I2C_ADDRESS, ALARM2_DYDT_ADDRESS, 0x80); // write a 1 to bit 7 (A1M4) to indicate to not use the day of week/month for the alarm
    if(ret != 0)
      return ret;
  }
  else // other values are invalid
    return 6; // signal than invalid values for both or one of the parameters is given

  return 0; // signal alarm 1 has been set succesfully
}
