/*
 * RF_RX.c
 *
 *  Created on: 06 Aug 2018
 *      Author: wynan
 */
#include <main.h>
#include "stm32f0xx_hal.h"
#include "cmsis_os.h"
#include "EEPROM.h"


// RF_Rx.c

// Devices
// Tamper all zones	= 0x0E
// Zone 5	= 0x08
// Zone 6	= 0x09
// Zone 7	= 0x0A
// Zone 8	= 0x0B
// Zone 9	= 0x0C
// Zone 10	= 0x0D


#define TRUE								1
#define FALSE								0

#define ARM_BUTTON							0x02
#define DISARM_BUTTON						0x04
#define PANIC_BUTTON						0x01
#define CHIME_BUTTON						0x06

#define DEVICEBIT							0x08
#define ZONE1TRIG							0x01
#define ZONE2TRIG							0x02
#define ZONE3TRIG							0x03
#define ZONE4TRIG							0x04
#define ZONE5TRIG							0x05
#define ZONE6TRIG							0x06



#define EEPROM_RF_DEVICE_START				150
#define EEPROM_RF_AMOUNT_PAGES				3

#define LEARN_DEBOUNCE						3
#define ERASETIME							120

#define AMOUNT_RF_DEVICES					45			// 15 per page to leave the CRC still in use byte 62 + 63 of the page

typedef struct {										// total length = 4 bytes. Reason is 4 x 15 = 60 that fits into a page.
	char DeviceSerial[4];
} TX_Dev;

TX_Dev Device_List[AMOUNT_RF_DEVICES];

extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;

extern unsigned char Arm_Status;
extern unsigned char ChimeSMS_Enabled;
extern unsigned char Buzzer_Time;

extern unsigned char Panic_Out_Time;

extern unsigned char Message[];

unsigned char bitpart = 0;
unsigned char RX_State = 0;
unsigned char preamble = 0;
unsigned char curbit = 0;
unsigned char Bit = 0;
unsigned char button = 0;

extern unsigned char Disable_LED;


unsigned char ZoneTriggers[6] = {0};

unsigned char match = 0;
unsigned char prev_id = 0;
unsigned int prev_id_timer = 0;

unsigned char learnpart = 0;
unsigned char Got_Header = FALSE;
unsigned int Header_Count = 0;
unsigned int Header_Timer = 0;
unsigned char id;
unsigned char pos;

char PrevSerialNum[4];

unsigned char Triggered_Home_Zone = 0;

unsigned char Erase_Flag1 = FALSE;
unsigned char Erase_Flag2 = FALSE;
unsigned char Erase_Flag3 = FALSE;

unsigned int AvrTmr = 0;
unsigned int TempTmr = 0;

unsigned long o, i;

unsigned char RAW_RX_Data[10];
unsigned char RAW_RX_Char = 0;

char SerialNum[4];
unsigned char Sensor_Battery_Low = 0;
unsigned char Button_Push = 0;
unsigned char bp = 0;

unsigned char Silent_Panic_Buzzer_Time;

unsigned char LearnMode = 0;
unsigned char LearnTimeout = 0;

unsigned char Learn_SW_Pressed = 0;
unsigned char LearnStatus = 0;
unsigned int LearnDebounce = 0;
unsigned int Learn_Press_Time = 0;
unsigned long TimerVal = 0;

float Timerval = 0.0;

char Prev_serial[4];

char serial[4];
char serial1[4];
char serial2[4];

unsigned char Data_Received_Flag = FALSE;
unsigned char Reload_Device_Flag = FALSE;
unsigned char Learn_Device_Flag = FALSE;

extern unsigned int Siren_Time;
extern unsigned char Alarm_Out_Time;
unsigned char Siren_Annunciate = 0;
unsigned char Buzzer_Annunciate = 0;

unsigned int Siren_Annunciate_Delay = 0;
unsigned int Buzzer_Annunciate_Delay = 0;

unsigned int no_data_Timer;

extern unsigned char AwayModeZones[];
extern unsigned char ChimeModeZones[];
extern unsigned char AnyModeChimeZones[];

extern IWDG_HandleTypeDef hiwdg;

void Delay_LED(void)
{ unsigned int delay, c, x;
  for(x=0;x<2000;x++) {
  	  for(delay=0;delay<20;delay++) {
    	c++;
      }
   }
}


void Clear_RF_Device_List(void)
{  unsigned char p;
   char tempstr[64];
	memset(Device_List, 0, AMOUNT_RF_DEVICES * 4);
	memset(tempstr, 0, 64);
	for(p = 0; p < EEPROM_RF_AMOUNT_PAGES; p++) {
		HAL_IWDG_Refresh(&hiwdg);
		Save_Settings(EEPROM_RF_DEVICE_START + p, tempstr);
	}

	LearnMode = 0;

}

void Save_RF_Device_List(void)
{
	unsigned char p;
	unsigned char q;
    char tempstr[64];

	for(p = 0; p < EEPROM_RF_AMOUNT_PAGES; p++) {
		q = p * 15;
		memset(tempstr, 0, 64);
		memcpy(tempstr, Device_List[q].DeviceSerial, 60);
		Save_Settings(EEPROM_RF_DEVICE_START + p, tempstr);
	}
}

void Load_RF_Device_List(void)
{
	unsigned char p;
    unsigned char q;
	char * tempPoint;

	for(p = 0; p < EEPROM_RF_AMOUNT_PAGES; p++) {
		q = p * 15;
		tempPoint = Read_Settings(EEPROM_RF_DEVICE_START + p);

		memcpy(&Device_List[q], tempPoint, 60);
	}
}

unsigned char Search_Device(char * inserial)
{ unsigned char p = 0;
  unsigned int tempserial = 0;

    tempserial = inserial[0] + inserial[1] + inserial[2] + inserial[3];
	if(tempserial == 0) {
		return(FALSE);
	}
	for(p = 0; p < AMOUNT_RF_DEVICES; p++) {
		if(strstr(Device_List[p].DeviceSerial, inserial) != NULL) {
			return(p+1);
		}
	}
	return(FALSE);
}

unsigned char Get_Space(char * inserial)
{ unsigned char p, u = 0;
	for(p = 0; p < AMOUNT_RF_DEVICES; p++) {
		if(strstr(Device_List[p].DeviceSerial, inserial) != NULL) {
			Disable_LED = 1;
			for(u = 0; u < 3; u++) {
				HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_SET);
				Delay_LED();	Delay_LED();	Delay_LED();	Delay_LED();	Delay_LED();	Delay_LED();

				HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_RESET);
				Delay_LED();	Delay_LED();	Delay_LED();	Delay_LED();	Delay_LED();	Delay_LED();
			}
			Disable_LED = 0;
			return(0);
		}
	}
	for(p = 0; p < AMOUNT_RF_DEVICES; p++) {
		if((Device_List[p].DeviceSerial[0] == 0)&&(Device_List[p].DeviceSerial[1] == 0)&&(Device_List[p].DeviceSerial[2] == 0)&&(Device_List[p].DeviceSerial[3] == 0)) {
			return(p + 1);
		}
	}
	return(0);
}

void Check_RF_Flags(void)
{
	if(Data_Received_Flag == TRUE) {

		memcpy(serial, SerialNum, 4);
		if(Disable_LED != 1) {
			HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_SET);
		}



		button = Button_Push;


		if(LearnMode == 1) {

				LearnMode = 0;
				pos = Get_Space(serial);
				if(pos > 0) {
					memcpy(&Device_List[pos-1].DeviceSerial[0], serial, 4);
					Save_RF_Device_List();
					Disable_LED = 1;
		  			for(unsigned char u = 0; u < 20; u++) {

		  				HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_SET);
		  				Delay_LED();   Delay_LED();  	Delay_LED();   Delay_LED();

		  				HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_RESET);
		  				Delay_LED();  Delay_LED();  	Delay_LED();   Delay_LED();
		  			}
					Disable_LED = 0;
					LearnMode = 0;
				}

			memcpy(Prev_serial, serial, 4);
		} else if(id > 0) {
			if((prev_id == id)&&(prev_id_timer > 0)) {
				button = 0;
				id = 0;
			} else {
				prev_id_timer = 100;
				prev_id = id;
			}

			if(Sensor_Battery_Low == 1) {

				Message[3] = 0;

				Message[0] = EE_SENSOR_LOW_BATTERY;
				Message[1] = 127;
				Message[2] = 1;

				if(button == (ZONE1TRIG | DEVICEBIT)) {
					Message[3] = '1';
				} else if(button == (ZONE2TRIG | DEVICEBIT)) {
					Message[3] = '2';
				} else if(button == (ZONE3TRIG | DEVICEBIT)) {
					Message[3] = '3';
				} else if(button == (ZONE4TRIG | DEVICEBIT)) {
					Message[3] = '4';
				} else if(button == (ZONE5TRIG | DEVICEBIT)) {
					Message[3] = '5';
				} else if(button == (ZONE6TRIG | DEVICEBIT)) {
					Message[3] = '6';
				}
				if(Message[3] > 0) {
					Message[4] = EE_BY_MESSAGE;
					Message[5] = EE_AT_MESSAGE;
					Message[6] = 0;

					Add_Message_to_Que(Message, 0, 0, 0, TRUE);
				}
			}

			if(button == DISARM_BUTTON) {			// Disarm
				ZoneTriggers[0] = 0;
				ZoneTriggers[1] = 0;
				ZoneTriggers[2] = 0;
				ZoneTriggers[3] = 0;
				ZoneTriggers[4] = 0;
				ZoneTriggers[5] = 0;

				if((Arm_Status == 1)||(Arm_Status == 2)) {
					Message[0] = EE_SYSTEM_OFF_MODE;
					Message[1] = EE_BY_MESSAGE;
					Message[2] = EE_KEYRING;
					Message[3] = EE_AT_MESSAGE;
					Message[4] = 0;

					Add_Message_to_Que(Message, 0, 0, 0, TRUE);

					if(Arm_Status == 2) {
						Buzzer_Annunciate = 2;
					} else {
						Siren_Annunciate = 2;
					}

					Siren_Annunciate_Delay = 0;

					Arm_Status = 0;
				}
				HAL_GPIO_WritePin(OUT_ONOFF_GPIO_Port, OUT_ONOFF_Pin, GPIO_PIN_RESET);

				HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB, RELAY2_Pin, GPIO_PIN_RESET);
				Siren_Time = 0;
				Buzzer_Time = 0;
			} else if(button == ARM_BUTTON) {		// Arm
				if(Arm_Status != 1) {
					memset(Message, 0, 10);
					Message[0] = EE_AWAY_MODE;
					Message[1] = EE_BY_MESSAGE;
					Message[2] = EE_KEYRING;
					Message[3] = EE_AT_MESSAGE;
					Message[4] = 0;

					Add_Message_to_Que(Message, 0, 0, 0, TRUE);

					HAL_GPIO_WritePin(OUT_ONOFF_GPIO_Port, OUT_ONOFF_Pin, GPIO_PIN_SET);
					Siren_Annunciate = 1;
					Siren_Annunciate_Delay = 0;

					Arm_Status = 1;
				}
			} else if(button == CHIME_BUTTON) {		// Chime
				if(Arm_Status != 2) {
					memset(Message, 0, 10);
					Message[0] = EE_MONITOR_MODE;
					Message[1] = EE_BY_MESSAGE;
					Message[2] = EE_KEYRING;
					Message[3] = EE_AT_MESSAGE;
					Message[4] = 0;


					Add_Message_to_Que(Message, 0, 0, 0, TRUE);

					Arm_Status = 2;

					Buzzer_Annunciate = 3;
					Siren_Annunciate_Delay = 0;

				}
			} else if((button == (PANIC_BUTTON))) {
				memset(Message, 0, 10);
				Message[0] = EE_PANIC;
				Message[1] = EE_BY_MESSAGE;
				Message[2] = EE_KEYRING;
				Message[3] = EE_AT_MESSAGE;
				Message[4] = 0;

				Add_Message_to_Que(Message, 0, 0, 0, TRUE);
				Panic_Out_Time = OUTPUT_ON_TIME + 1;
				Siren_Time = SIREN_SECONDS;


			} else if(button == PANIC_BUTTON|ARM_BUTTON) {
				memset(Message, 0, 10);
				Message[0] = EE_SILENT_PANIC;
				Message[1] = EE_BY_MESSAGE;
				Message[2] = EE_KEYRING;
				Message[3] = EE_AT_MESSAGE;
				Message[4] = 0;

				Add_Message_to_Que(Message, 0, 0, 0, TRUE);
				Panic_Out_Time = OUTPUT_ON_TIME + 1;
				Buzzer_Time = Silent_Panic_Buzzer_Time;


			} else if(button&DEVICEBIT) {
				if(button == (ZONE1TRIG | DEVICEBIT)) {
					if((AwayModeZones[0] == '1')&&(Arm_Status == 1)&&(ZoneTriggers[0] < 5)) {
						ZoneTriggers[0]++;
						memset(Message, 0, 10);
						Message[0] = EE_ZONE1_MESSAGE;
						Message[1] = EE_AT_MESSAGE;
						Message[2] = 0;

						Add_Message_to_Que(Message, 0, 0, 0, TRUE);

						if(AnyModeChimeZones[0] == '0') {
							Siren_Time = SIREN_SECONDS;
							Alarm_Out_Time = OUTPUT_ON_TIME + 1;
						} else {
							Buzzer_Annunciate = 6;
						}
					} else if((ChimeModeZones[0] == '1')&&(Arm_Status == 2)) {
						if(ChimeSMS_Enabled == 1) {
							memset(Message, 0, 10);
							Message[0] = EE_ZONE1_MESSAGE;
							Message[1] = EE_AT_MESSAGE;
							Message[2] = 0;

							Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						}
						//  Chime Trigger
						Buzzer_Annunciate = 6;
					}
				} else if(button == (ZONE2TRIG | DEVICEBIT)) {
					if((AwayModeZones[1] == '1')&&(Arm_Status == 1)&&(ZoneTriggers[1] < 5)) {
						ZoneTriggers[1]++;
						memset(Message, 0, 10);
						Message[0] = EE_ZONE2_MESSAGE;
						Message[1] = EE_AT_MESSAGE;
						Message[2] = 0;

						Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						if(AnyModeChimeZones[1] == '0') {
							Siren_Time = SIREN_SECONDS;
							Alarm_Out_Time = OUTPUT_ON_TIME + 1;
						} else {
							Buzzer_Annunciate = 6;
						}	Siren_Time = SIREN_SECONDS;

						Alarm_Out_Time = OUTPUT_ON_TIME + 1;
					} else if((ChimeModeZones[1] == '1')&&(Arm_Status == 2)) {
						if(ChimeSMS_Enabled == 1) {
							memset(Message, 0, 10);
							Message[0] = EE_ZONE2_MESSAGE;
							Message[1] = EE_AT_MESSAGE;
							Message[2] = 0;

							Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						}
						//  Chime Trigger
						Buzzer_Annunciate = 6;
					}
				} else if(button == (ZONE3TRIG | DEVICEBIT)) {
					if((AwayModeZones[2] == '1')&&(Arm_Status == 1)&&(ZoneTriggers[2] < 5)) {
						ZoneTriggers[2]++;
						memset(Message, 0, 10);
						Message[0] = EE_ZONE3_MESSAGE;
						Message[1] = EE_AT_MESSAGE;
						Message[2] = 0;

						Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						Siren_Time = SIREN_SECONDS;

						Alarm_Out_Time = OUTPUT_ON_TIME + 1;
					} else if((ChimeModeZones[2] == '1')&&(Arm_Status == 2)) {
						if(ChimeSMS_Enabled == 1) {
							memset(Message, 0, 10);
							Message[0] = EE_ZONE3_MESSAGE;
							Message[1] = EE_AT_MESSAGE;
							Message[2] = 0;

							Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						}
						//  Chime Trigger
						Buzzer_Annunciate = 6;
					}
				} else if(button == (ZONE4TRIG | DEVICEBIT)) {
					if((AwayModeZones[3] == '1')&&(Arm_Status == 1)&&(ZoneTriggers[3] < 5)) {
						ZoneTriggers[3]++;
						memset(Message, 0, 10);
						Message[0] = EE_ZONE4_MESSAGE;
						Message[1] = EE_AT_MESSAGE;
						Message[2] = 0;

						Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						if(AnyModeChimeZones[3] == '0') {
							Siren_Time = SIREN_SECONDS;
							Alarm_Out_Time = OUTPUT_ON_TIME + 1;
						} else {
							Buzzer_Annunciate = 6;
						}
					} else if((ChimeModeZones[3] == '1')&&(Arm_Status == 2)) {
						if(ChimeSMS_Enabled == 1) {
							memset(Message, 0, 10);
							Message[0] = EE_ZONE4_MESSAGE;
							Message[1] = EE_AT_MESSAGE;
							Message[2] = 0;

							Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						}
						//  Chime Trigger
						Buzzer_Annunciate = 6;
					}
				} else if(button == (ZONE5TRIG | DEVICEBIT)) {
					if((AwayModeZones[4] == '1')&&(Arm_Status == 1)&&(ZoneTriggers[4] < 5)) {
						ZoneTriggers[4]++;
						memset(Message, 0, 10);
						Message[0] = EE_ZONE5_MESSAGE;
						Message[1] = EE_AT_MESSAGE;
						Message[2] = 0;

						Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						if(AnyModeChimeZones[4] == '0') {
							Siren_Time = SIREN_SECONDS;
							Alarm_Out_Time = OUTPUT_ON_TIME + 1;
						} else {
							Buzzer_Annunciate = 6;
						}
					} else if((ChimeModeZones[4] == '1')&&(Arm_Status == 2)) {
						if(ChimeSMS_Enabled == 1) {
							memset(Message, 0, 10);
							Message[0] = EE_ZONE5_MESSAGE;
							Message[1] = EE_AT_MESSAGE;
							Message[2] = 0;

							Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						}
						//  Chime Trigger
						Buzzer_Annunciate = 6;
					}
				} else if(button == (ZONE6TRIG | DEVICEBIT)) {
					if((AwayModeZones[5] == '1')&&(Arm_Status == 1)&&(ZoneTriggers[5] < 5)) {
						ZoneTriggers[5]++;
						memset(Message, 0, 10);
						Message[0] = EE_ZONE6_MESSAGE;
						Message[1] = EE_AT_MESSAGE;
						Message[2] = 0;

						Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						if(AnyModeChimeZones[5] == '0') {
							Siren_Time = SIREN_SECONDS;
							Alarm_Out_Time = OUTPUT_ON_TIME + 1;
						} else {
							Buzzer_Annunciate = 6;
						}
					} else if((ChimeModeZones[5] == '1')&&(Arm_Status == 2)) {
						if(ChimeSMS_Enabled == 1) {
							memset(Message, 0, 10);
							Message[0] = EE_ZONE6_MESSAGE;
							Message[1] = EE_AT_MESSAGE;
							Message[2] = 0;

							Add_Message_to_Que(Message, 0, 0, 0, TRUE);
						}
						//  Chime Trigger
						Buzzer_Annunciate = 6;
					}
				}

			}
		} else {

			Delay_LED();
		}

		Data_Received_Flag = FALSE;
	}

	if(Reload_Device_Flag == TRUE) {
		Reload_Device_Flag = FALSE;
		Load_RF_Device_List();
	}
}

void BuzzerTMR_Int(void)
{
	if(Siren_Annunciate_Delay > 0) {
		Siren_Annunciate_Delay--;
	} else {
		Siren_Annunciate_Delay = 10;
		if(Siren_Annunciate > 0) {
			if(HAL_GPIO_ReadPin(GPIOB, RELAY1_Pin) == 0) {
				HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_SET);
				Siren_Annunciate_Delay = 1;
			} else {
				HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_RESET);
				Siren_Annunciate--;
			}
		}
	}

	if(Buzzer_Annunciate_Delay > 0) {
		Buzzer_Annunciate_Delay--;
	} else {
		Buzzer_Annunciate_Delay = 10;
		if(Buzzer_Annunciate > 0) {
			if(HAL_GPIO_ReadPin(GPIOB, RELAY2_Pin) == 0) {
				HAL_GPIO_WritePin(GPIOB, RELAY2_Pin, GPIO_PIN_SET);
				Buzzer_Annunciate_Delay = 1;
			} else {
				HAL_GPIO_WritePin(GPIOB, RELAY2_Pin, GPIO_PIN_RESET);
				Buzzer_Annunciate--;
			}
		}
	}
}

void RF_PacketTMR_Int(void)
{
	if(RX_State == 0) {   // preamble
		AvrTmr = 0;
		preamble = 0;
	} else if(RX_State == 1) {   // header receive state.  If timeout then error
		RX_State = 2;
	}
	if(Disable_LED != 1) {
		HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_RESET);
	}
}

void RX_Pin_Int(void)
{
	if(RX_State == 0) {
		RAW_RX_Char = 0;
		RAW_RX_Data[0] = 0;		//0-8
		RAW_RX_Data[1] = 0;		//9-16
		RAW_RX_Data[2] = 0;		//17-24
		RAW_RX_Data[3] = 0;		//25-32
		RAW_RX_Data[4] = 0;		//33-40
		RAW_RX_Data[5] = 0;		//41-48
		RAW_RX_Data[6] = 0;		//49-56
		RAW_RX_Data[7] = 0;		//57-64
		RAW_RX_Data[8] = 0;		//65-72

		Bit = 0;
		if(preamble == 0) {
			if(HAL_GPIO_ReadPin(RFRX_IN_GPIO_Port, RFRX_IN_Pin)) {
				preamble++;
				htim6.Instance->CNT = 0;
			}
		} else {
			TimerVal = htim6.Instance->CNT;  //__HAL_TIM_GET_COUNTER(&htim6);
			if((TimerVal > 100)&&(TimerVal < 250)) {   //    0x022D = 280us   0x04D1 = 620us
				preamble++;
				AvrTmr = AvrTmr + TimerVal;
				htim6.Instance->CNT = 0;
			} else {
				preamble = 0;
				AvrTmr = 0;
			}
		}
		if(preamble > 23) {   // preamble received
			TempTmr = AvrTmr / (preamble - 1);
			TempTmr = TempTmr * 9.5;  // calc Header time - 5% error
			RX_State = 1;
			memset(RAW_RX_Data, 0, 10);
			htim6.Instance->CNT = 0;
			htim6.Instance->ARR = TempTmr;
			Got_Header = 0;
		}
	} else if(RX_State == 1) {   // error while header
		RX_State = 0;
		AvrTmr = 0;
		preamble = 0;
	} else if((RX_State == 2)&&(HAL_GPIO_ReadPin(RFRX_IN_GPIO_Port, RFRX_IN_Pin))) {  // header ok, first byte start rx
		Got_Header = 1;
		Header_Timer = 0;

		RX_State = 3;
		RAW_RX_Char = 0;
		Bit = 0;
		// start timer to check bit value
		bitpart = 0;

		htim6.Instance->CNT = 0;
		htim6.Instance->ARR = TempTmr;

		Timerval = (TempTmr / 9.5) * 1.5;

	} else if(RX_State == 3) {
		if(HAL_GPIO_ReadPin(RFRX_IN_GPIO_Port, RFRX_IN_Pin) == 0) {
			if(Disable_LED != 1) {
				HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_RESET);
			}
			if(htim6.Instance->CNT > Timerval) {
				curbit = 0;
			} else {
				curbit = 1;
			}
			htim6.Instance->CNT = 0;
			bitpart++;
			RAW_RX_Data[RAW_RX_Char] = RAW_RX_Data[RAW_RX_Char] << 1;
			RAW_RX_Data[RAW_RX_Char] |= curbit;
			bitpart++;
		} else {
			htim6.Instance->CNT = 0;
			if(Disable_LED != 1) {
				HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_SET);
			}

			if(Bit > 6) {
				if(Disable_LED != 1) {
					HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_RESET);
				}
				Bit = 0;
				RAW_RX_Char++;
				if(RAW_RX_Char == 9) {
					__HAL_DBGMCU_FREEZE_IWDG();
					memset(SerialNum,0,4);
					memcpy(SerialNum, &RAW_RX_Data[4], 4);
					bp = (unsigned char) SerialNum[3];
					SerialNum[3] = SerialNum[3] & 0xF0;
					id = Search_Device(SerialNum);
					if(id != 0) {

							bp = bp & 0x0F;

							Sensor_Battery_Low = (RAW_RX_Data[8]&0x80) >> 7;

							Button_Push = ((bp >> 2) & 0x01);
							Button_Push |= (bp & 0x02);
							Button_Push |= ((bp << 2) & 0x04);
							Button_Push |= (bp & 0x08);

							Data_Received_Flag = TRUE;
							Check_RF_Flags();

					} else {
						Data_Received_Flag = TRUE;
						Check_RF_Flags();
					}
					RX_State = 0;
				}
			} else Bit++;
			if((RAW_RX_Char == 9)&&(Bit == 2)) {
				Bit = 7;
			}

			bitpart = 0;

		}
	} else if(HAL_GPIO_ReadPin(RFRX_IN_GPIO_Port, RFRX_IN_Pin)) {
		RX_State = 0;
		if(Disable_LED != 1) {
			HAL_GPIO_WritePin(GPIOB, RFRX_LED_Pin, GPIO_PIN_RESET);
		}
	}
}

