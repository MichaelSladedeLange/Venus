// SIM800H Lib
#include "stm32f0xx_hal.h"
#include <main.h>
#include "SIM800H.h"
#include "EEPROM.h"
#include "..\Inc\main.h"
#include <string.h>
#include <stdlib.h>
#include "cmsis_os.h"


#define HTTP_URL_1	"http://bot.fineautomation.co.za:50123/api/v1/whatsapp-alert?phone="
#define HTTP_URL_2	"&text="
#define HTTP_URL_3	"&apikey="

#define	GSM_DEFAULTS_HTTP	"AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\";+SAPBR=3,1,\"APN\",\"internet\";+SAPBR=1,1\r\n"


// HTTP
#define POST 					"POST \0"
#define GET 					"GET \0"
#define HEAD 					"HEAD \0"
#define HOST					"Host: \0"
#define HTTP 					" HTTP/1.1\0"
#define HTTPPAGE				"/receive_gprs.asp\0"
#define REQUESTPAGE				"/process_alti_request.asp\0"
#define PWD						"&pwd=\0"
#define GSM_SERIALNUM			"&gsm_serial=\0"
#define DLINE					"dline=\0"
#define REQUEST					"req=\0"
#define Content_Length_part		"Content-Length: \0"
#define Colon					":\0"
#define Connection				"Connection: persistent\r\n"     // close
#define User_Agent				"User-Agent: \0"

#define TIMESYNC_SMS					100
#define PASSWORDS						101
#define BASE							102
#define TIME_24H_MESSAGE				103
#define INPUT_STATUS1_2					104
#define OUTPUTS_TIMER_VALUES			105
#define SIGNAL_STRENGTH					106
#define SOFTWARE_VERSION				107
#define AIRTIME							108
#define DATETIME						109
#define CELLREQ							110
#define OUTPUTS_STATUS					111
#define GPRS_SETTINGS_MESSAGE			112
#define GPRS_STATUS_MESSAGE				113
#define STATUS							114
#define CELL_LOCATION					115
#define ZONESETUP						116

#define GSM_DEFAULTS	"AT+IPR=115200;+CMGF=1;+CSCS=\"GSM\";+CNMI=2,1,0,0;+IFC=0;+CMIC=0,15;S0=2;&W\r\n\0"

#define APN_NAME		"internet"
#define APN_USERNAME	""
#define APN_PASSWORD	""

#define TRUE			1
#define FALSE			0

#define MAX_GPRS_MESSAGE_RETRIES	4
#define MAX_SMS_MESSAGE_RETRIES		6
#define MAX_GSM_MESSAGE_RETRIES		6

extern unsigned char Update_Mem_Flag;
extern unsigned char ChimeSMS_Enabled;
extern unsigned char Prev_Mains;
extern unsigned char Arm_Status;
extern unsigned char Siren_Annunciate;
extern unsigned int Siren_Annunciate_Delay;
extern unsigned char Buzzer_Annunciate;
//extern unsigned int Buzzer_Annunciate_Delay;
extern unsigned int Buzzer_Time;
extern unsigned int Siren_Time;
//extern unsigned char Alarm_Out_Time;
extern unsigned char Panic_Out_Time;
extern unsigned char Silent_Panic_Buzzer_Time;
extern unsigned char ZoneTriggers[];

unsigned char NO_SIM = 0;

unsigned char Restart_GSM_Connection = 0;

unsigned char Incomming_SMS_Number[30];

unsigned char MQTT_Start_Flag;
unsigned char MQTT_RX_Mode;
unsigned char MQTT_RX_Length;
unsigned char MQTT_RXP;
unsigned char MQTT_RX_Message_Flag;
unsigned char MQTT_RX_Packet[255];
unsigned int  MQTT_Start_Pointer;

unsigned int mesid = 0;
unsigned char MesQue_Head = 0;
unsigned char MesQue_Tail = 0;

unsigned char Whats_Message[160];

unsigned char mqttbuffer[10];  // 500

unsigned char Out_Message[160];

unsigned char SimPos = 0;

unsigned char creg_timer = 0;

unsigned char Update_Settings_Timer = 0;

unsigned char GSM_Signal = 0;

unsigned char GSM_Registered = 0;
unsigned char Request_AitTime_Flag = 0;

unsigned char AirTime[200];

unsigned int Airtime_Failure = 0;

unsigned char Own_Number[30];

char buf[2000];

extern unsigned char Airtime_Req_Timer;
extern unsigned char Reset_GSM_Flag;
extern unsigned char GPRS_Enabled;
extern unsigned char GPRS_Barear_Started;
extern unsigned char Message[];

extern unsigned int year;
extern unsigned char month;
extern unsigned char day;
extern unsigned char hours;
extern unsigned char minutes;
extern unsigned char seconds;

extern IWDG_HandleTypeDef hiwdg;

extern RTC_HandleTypeDef hrtc;


char * CellnumPoint;
char * DatePoint;

typedef struct
{
    char    Message[10];
	char 	Cellnumber;
	char    Send;
    char    Retries;
	char    Send_Flags;
	char    Send_Only_GPRS;
} MesLog;

MesLog MessageLog[25];

char CCID[30];


extern UART_HandleTypeDef huart2;

unsigned int U2Rx_indx;
unsigned char U2Rx_data[2], U2Rx_Buffer[512], U2Transfer_cplt;

float BatteryVoltage;



static unsigned char SIM800H_Send_SMS(char * number, char * message);
unsigned char Send_GPRS_SIM800(char * serv_num, char * message, unsigned char Mes_Amount);
static unsigned char SIMA76_HTTP_Send_Message(char * number, char * message, unsigned char cellPosition);



unsigned char Send_Message(char * message)
{  unsigned char r = 0;
   unsigned char Mask = 0;
   unsigned char Mask_Flags = 0;
   unsigned char SendFlag_Pos = 0;
   unsigned char Number_Of_Messages = 0;
   char cellnum[65];

    SIM800H_Clear_Buffers();

	if(SIM800H_Register_Status()) {
	Mask = 0x80;
	Mask_Flags = 0xFF;
	for(SendFlag_Pos = 1; SendFlag_Pos < 10; SendFlag_Pos++) {
		if(Mask & MessageLog[MesQue_Tail].Send_Flags) {
			Mask_Flags >>= 1;
			break;
		} else {
			Mask >>= 1;
			Mask_Flags >>= 1;
		}
	}
	if(Mask == 0) {   // if no flags is set
		MessageLog[MesQue_Tail].Send = TRUE;
		MessageLog[MesQue_Tail].Cellnumber = 0;
		MessageLog[MesQue_Tail].Message[0] = 0;
		MessageLog[MesQue_Tail].Retries = 0;
		return(FALSE);
	}

	if(SendFlag_Pos == 1) {   // Base
		strcpy(cellnum, Read_Settings(EE_GPRS_BASE));
		if((GPRS_Enabled == TRUE)&&(strlen(cellnum) > 4)&&(MessageLog[MesQue_Tail].Retries < MAX_GPRS_MESSAGE_RETRIES)) {
			if(strlen(message) > 3) {
				MessageLog[MesQue_Tail].Retries++;
				if(GPRS_Barear_Started) {
					if(!Send_GPRS_SIM800(cellnum, message, 0)) {
						if(MessageLog[MesQue_Tail].Retries > MAX_GPRS_MESSAGE_RETRIES) {
							MessageLog[MesQue_Tail].Retries = 0;
							MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
						}
						if(MessageLog[MesQue_Tail].Retries == (MAX_GPRS_MESSAGE_RETRIES - 1)) {
							Reset_GSM_Flag = 1;
							return(0);
						}
					} else {
						MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
						MessageLog[MesQue_Tail].Retries = 0;
					//	//sprintf(Message, "NUMBER OF GPRS MESSAGES SEND = %d\r\n", Number_Of_Messages);
					//	//UARTX_TX(Message);

					}
				}
			} else {
				MessageLog[MesQue_Tail].Send = TRUE;
				MessageLog[MesQue_Tail].Cellnumber = 0;
				MessageLog[MesQue_Tail].Message[0] = 0;
				MessageLog[MesQue_Tail].Send_Flags = 0;
			}
		} else {
		    strcpy(cellnum, Read_Settings(EE_SMS_BASE));
		    if((strlen(cellnum) > 4)&&(!MessageLog[MesQue_Tail].Send_Only_GPRS)) {
				if(strlen(message) > 3) {
					MessageLog[MesQue_Tail].Retries++;
					if(!SIM800H_Send_SMS(cellnum, message)) {
						if(MessageLog[MesQue_Tail].Retries > MAX_SMS_MESSAGE_RETRIES) {
							MessageLog[MesQue_Tail].Retries = 0;
							MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
						}
						Reset_GSM_Flag = 1;
						return(0);
					} else {   // if message is send
						MessageLog[MesQue_Tail].Retries = 0;
						MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
					}
				} else {
						MessageLog[MesQue_Tail].Send = TRUE;
						MessageLog[MesQue_Tail].Cellnumber = 0;
						MessageLog[MesQue_Tail].Message[0] = 0;
						MessageLog[MesQue_Tail].Send_Flags = 0;
				}
			} else {
				MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
			}
		}
	} else {
		if((SendFlag_Pos > 1)&&(SendFlag_Pos < 8)) {
			strcpy(cellnum, Read_Settings(EE_SMS_CELL1 + (SendFlag_Pos - 2)));
			if((strlen(cellnum) > 4)&&(!MessageLog[MesQue_Tail].Send_Only_GPRS)) {
				if(strlen(message) > 3) {
					MessageLog[MesQue_Tail].Retries++;
					if(SIMA76_HTTP_Send_Message(cellnum, Whats_Message, SendFlag_Pos)) {
						if(!SIM800H_Send_SMS(cellnum, message)) {
							if(MessageLog[MesQue_Tail].Retries > MAX_SMS_MESSAGE_RETRIES) {
								MessageLog[MesQue_Tail].Retries = 0;
								MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
							}
							Reset_GSM_Flag = 1;
							return(0);
						} else {   // if message is send
							MessageLog[MesQue_Tail].Retries = 0;
							MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
						}
				  }else {   // if message is send
						MessageLog[MesQue_Tail].Retries = 0;
						MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
					}
				} else {
						MessageLog[MesQue_Tail].Send = TRUE;
						MessageLog[MesQue_Tail].Cellnumber = 0;
						MessageLog[MesQue_Tail].Message[0] = 0;
						MessageLog[MesQue_Tail].Send_Flags = 0;
				}
			} else {
				MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
				MessageLog[MesQue_Tail].Retries = 0;
			}
		} else if((SendFlag_Pos == 8)) {
			if(MessageLog[MesQue_Tail].Cellnumber == 0) {
				MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
				MessageLog[MesQue_Tail].Retries = 0;

			} else {
				strcpy(cellnum, Read_Settings(MessageLog[MesQue_Tail].Cellnumber));
				MessageLog[MesQue_Tail].Retries++;
				if((strlen(cellnum) > 4)&&(!MessageLog[MesQue_Tail].Send_Only_GPRS)) {
					if(strlen(message) > 3) {
						if(!SIM800H_Send_SMS(cellnum, message)) {
							if(MessageLog[MesQue_Tail].Retries > MAX_SMS_MESSAGE_RETRIES) {
								MessageLog[MesQue_Tail].Retries = 0;
								MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
							}
							Reset_GSM_Flag = 1;
							return(0);
						} else {   // if message is send
							MessageLog[MesQue_Tail].Retries = 0;
							MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
						}
					} else {
						MessageLog[MesQue_Tail].Send = TRUE;
						MessageLog[MesQue_Tail].Cellnumber = 0;
						MessageLog[MesQue_Tail].Message[0] = 0;
						MessageLog[MesQue_Tail].Send_Flags = 0;
					}
				} else {
					MessageLog[MesQue_Tail].Send_Flags &= Mask_Flags;
				}
			}
		}
	}
	}
	SIM800H_Clear_Buffers();

	return(TRUE);
}

void Get_DateTime(void)
{
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;

	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	hours = sTime.Hours;
	minutes = sTime.Minutes;
	seconds = sTime.Seconds;

	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	month = sDate.Month;
	day = sDate.Date;
	year = sDate.Year;
}

char * Build_Message(char * InStr, char * mes)
{  unsigned char tp, x, j;
   unsigned char length;
   char tempstr[10];
   char * temppoint;

	tp = 0;
	mes[0] = 0;
	while(InStr[0] != 0) {
		if(InStr[0] < EE_END) {
			temppoint = Read_Settings(InStr[0]);
			if(tp == 0) {
				strncpy(tempstr, temppoint, 8);
				x = 0;
				for(j = 0; j < 8; j++) {
					if((tempstr[j] == '0')||(tempstr[j] == '1')) {
						x++;
					}
				}
				if(x == 8) {
					temppoint = temppoint + 8;
				}
			}
			if(strlen(mes) > 0) strcat(mes, " ");
			strcat(mes, temppoint);
		} else if(InStr[0] == TIMESYNC_SMS) {
			strcpy(mes, "TIME SYNC");
		} else if(InStr[0] == CELLREQ) {
			strcpy(mes, "Cell 1:");
			temppoint = Read_Settings(EE_SMS_CELL1);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}
			strcat(mes, ", Cell 2:");
			temppoint = Read_Settings(EE_SMS_CELL2);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}
			strcat(mes, ", Cell 3:");
			temppoint = Read_Settings(EE_SMS_CELL3);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}
			strcat(mes, ", Cell 4:");
			temppoint = Read_Settings(EE_SMS_CELL4);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}
			strcat(mes, ", Cell 5:");
			temppoint = Read_Settings(EE_SMS_CELL5);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}
			strcat(mes, ", Cell 6:");
			temppoint = Read_Settings(EE_SMS_CELL6);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}
		} else if(InStr[0] == ZONESETUP) {
			strcpy(mes, "Away Mode Zones:");
			temppoint = Read_Settings(EE_AWAY_MODE_ZONES_ENABLED);
			strcat(mes, temppoint);

			strcat(mes, ", Chime Mode Zones:");
			temppoint = Read_Settings(EE_CHIME_MODE_ZONES_ENABLED);
			strcat(mes, temppoint);

			strcat(mes, ", Chime Away Mode Zones:");
			temppoint = Read_Settings(EE_CHIME_ANYMODE_ZONES);
			strcat(mes, temppoint);

		} else if(InStr[0] == PASSWORDS) {
			strcpy(mes, "Program Password:");
			temppoint = Read_Settings(EE_PROG_PASS);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}
			strcat(mes, ", User Password:");
			temppoint = Read_Settings(EE_USER_PASS);
			strcat(mes, temppoint);

			strcat(mes, ", Own Number:");
			temppoint = Read_Settings(EE_OWN_NUMBER);
			strcat(mes, temppoint);
		} else if(InStr[0] == BASE) {
			strcpy(mes, "GPRS Base:");
			temppoint = Read_Settings(EE_GPRS_BASE);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}
			strcat(mes, ", SMS Base:");
			temppoint = Read_Settings(EE_SMS_BASE);
			if((strlen(temppoint) > 0)&&(temppoint[0] > 31)&&(temppoint[0] < 127)) {
				strcat(mes, temppoint);
			}

		} else if(InStr[0] == GPRS_SETTINGS_MESSAGE) {
			strcpy(mes, "GPRS APN:");
			temppoint = Read_Settings(EE_APN_NAME);
			strcat(mes, temppoint);
			strcat(mes, ", Username:");
			temppoint = Read_Settings(EE_APN_USERNAME);
			strcat(mes, temppoint);
			strcat(mes, ", Password:");
			temppoint = Read_Settings(EE_APN_PASSWORD);
			strcat(mes, temppoint);
			strcat(mes, ", HTTP Serv:");
			temppoint = Read_Settings(EE_TCP_SERV);
			strcat(mes, temppoint);
			strcat(mes, ", HTTP Port:");
			temppoint = Read_Settings(EE_TCP_PORT);
			strcat(mes, temppoint);
			strcat(mes, ", HTTP Username:");
			temppoint = Read_Settings(EE_HTTP_USERNAME);
			strcat(mes, temppoint);
			strcat(mes, ", HTTP Password:");
			temppoint = Read_Settings(EE_HTTP_PASSWORD);
			strcat(mes, temppoint);

		} else if(InStr[0] == GPRS_STATUS_MESSAGE) {
			strcpy(mes, "GPRS Status:");
			if(GPRS_Enabled) {
				strcat(mes, "Enabled");
			} else strcat(mes, "Disabled");

			if(GPRS_Barear_Started) {
				strcat(mes, ", GPRS is Connected.");
			} else strcat(mes, ", GPRS is Disconnected");
		} else if(InStr[0] == STATUS) {
			tp = 0;
			temppoint = Read_Settings(EE_HEALTH_TEST_PREPAID_MESSAGE);
			if(tp == 0) {
				strncpy(tempstr, temppoint, 8);
				x = 0;
				for(j = 0; j < 8; j++) {
					if((tempstr[j] == '0')||(tempstr[j] == '1')) {
						x++;
					}
				}
				if(x == 8) {
					temppoint = temppoint + 8;
				}
			}
			if(strlen(mes) > 0) strcat(mes, ", ");
			strcpy(mes, temppoint);

			tp = 0;
			if(Arm_Status == 1) {
				temppoint = Read_Settings(EE_AWAY_MODE);
			} else if (Arm_Status == 2) {
				temppoint = Read_Settings(EE_MONITOR_MODE);
			} else {
				temppoint = Read_Settings(EE_SYSTEM_OFF_MODE);
			}

			if(tp == 0) {
				strncpy(tempstr, temppoint, 8);
				x = 0;
				for(j = 0; j < 8; j++) {
					if((tempstr[j] == '0')||(tempstr[j] == '1')) {
						x++;
					}
				}
				if(x == 8) {
					temppoint = temppoint + 8;
				}
			}
			if(strlen(mes) > 0) strcat(mes, ", ");
			strcat(mes, temppoint);

			if(Prev_Mains == 1) {
				temppoint = Read_Settings(EE_MAINS_ON);
			} else {
				temppoint = Read_Settings(EE_MAINS_OFF);
			}

			if(tp == 0) {
				strncpy(tempstr, temppoint, 8);
				x = 0;
				for(j = 0; j < 8; j++) {
					if((tempstr[j] == '0')||(tempstr[j] == '1')) {
						x++;
					}
				}
				if(x == 8) {
					temppoint = temppoint + 8;
				}
			}
			if(strlen(mes) > 0) strcat(mes, ", ");
			strcat(mes, temppoint);


			strcat(mes, ", Bat Volts: ");
			sprintf(tempstr, "%1.1fV", BatteryVoltage);
			strcat(mes, tempstr);
		} else if(InStr[0] == TIME_24H_MESSAGE) {
			strcpy(mes, "24H Test Message Time:");
			temppoint = Read_Settings(EE_HEALTH_REPORT_TIME);
			strcat(mes, temppoint);
			strcat(mes, ", Report Days:");
			temppoint = Read_Settings(EE_HEALTH_REPORT_DAYS);
			strcat(mes, temppoint);
			strcat(mes, ", Bat Volts: ");
			sprintf(tempstr, "%1.1fV", BatteryVoltage);
			strcat(mes, tempstr);
		} else if(InStr[0] == SIGNAL_STRENGTH) {
			SIM800H_CSQ();
			sprintf(mes, "Signal Strength: %d/31", GSM_Signal);
		} else if(InStr[0] == SOFTWARE_VERSION) {
			strcpy(mes, FIRMWAREVERSION);
			strcat(mes, " ");
			temppoint = Read_Settings(EE_AT_MESSAGE);
			strcat(mes, temppoint);
		} else if(InStr[0] == AIRTIME) {
			if(Airtime_Req_Timer == 0) {
		    	Get_AirTime();
		    	if(!Airtime_Failure) {
		    		Airtime_Req_Timer = 120;
		    	}
		    }
		    sprintf(mes, "%s", AirTime);
			strcat(mes, " ");
		} else if(InStr[0] == DATETIME) {
			Get_DateTime();
			sprintf(mes, "Date Time: %02d/%02d/%02d %02d:%02d:%02d ", year, month, day, hours, minutes, seconds);
			temppoint = Read_Settings(EE_AT_MESSAGE);
			strcat(mes, temppoint);
		} else if(InStr[0] == 127) {  // Cellnumber will follow to include in message
			InStr++;
			length = InStr[0];
			InStr++;
			strcat(mes, " ");
			x = strlen(mes);
			while(length > 0) {
				mes[x] = InStr[0];
				InStr++;
				tp++;
				x++;
				length--;
			}
			mes[x] = 0;
		} else if(InStr[tp] == 126) {   // ';'
			strcat(mes, ";");
		} else if(InStr[tp] == 125) {   // Battery Spanning
			strcat(mes, " ");
			sprintf(tempstr, "%1.1fV", BatteryVoltage);
			strcat(mes, tempstr);
		}
		InStr++;
		tp++;
		if(tp > 8) break;
	}
	return mes;
}


unsigned char Get_SendFlags(unsigned char mespos)
{ unsigned char result;
  unsigned char j;
  char tempbuf[9];

	result = 0;
	strncpy(tempbuf, Read_Settings(mespos), 8);
	for(j = 0; j < 8; j++) {
		if((tempbuf[j] == '0')||(tempbuf[j] == '1')) {
			result <<= 1;
			result |= (tempbuf[j] - 0x30);
		} else {
			return(0xFF);
		}
	}
	return(result);
}

void Add_Message_to_Que(char * in_mes, unsigned char Send_Only_To_Sender, unsigned char CellNumber, unsigned char Send_Only_GPRS, unsigned char reply_message_flag)
{ unsigned char temp;
  unsigned char i = 0;
  unsigned char found = 0;
  unsigned char tpos;
  unsigned char cellnumber = 0;
  unsigned char byte1 = 0;
  unsigned char Send_Flags = 0;
  unsigned char tempstr[20] = {0};
    temp = 0;
    cellnumber = CellNumber;
    byte1 = in_mes[0];
    if(byte1 < EE_END) {		// checked
    	if(strlen(Read_Settings(byte1)) == 0) {
	    	temp = 30;  // exit proc
		}
    }

    __HAL_DBGMCU_FREEZE_IWDG();

    if(Send_Only_To_Sender == 1) {
    	Send_Flags = 1;
	    if(CellNumber == 0) {
		    temp = 30;
		} else if(strlen(Read_Settings(CellNumber)) == 0) {
			temp = 30;
		}
    	strcpy(tempstr, Read_Settings(CellNumber));
		for(i = 0; i < 6; i++) {
			if(strstr(Read_Settings(EE_SMS_CELL1 + i), &tempstr[3]) != NULL) {
				cellnumber = 0;
				Send_Flags = 0x80;
				Send_Flags >>= (i+1);
				break;
			}
		}
	} else {
		if(in_mes[0] == AIRTIME) {
			Send_Flags = Get_SendFlags(EE_HEALTH_TEST_PREPAID_MESSAGE);
		} else {
			Send_Flags = Get_SendFlags(in_mes[0]);
			strcpy(tempstr, Read_Settings(CellNumber));
			for(i = 0; i < 6; i++) {
				if(strstr(Read_Settings(EE_SMS_CELL1 + i), &tempstr[3]) != NULL) {
					//found = 1;
					HAL_GPIO_TogglePin(GSM_PWR_GPIO_Port, GSM_PWR_Pin);
					cellnumber = 0;
					break;
				}
			}
			if(found == 0) {
				Send_Flags = Send_Flags | 0x80;//if cannot find a stored number, sends to base
			}

		}

	}

    if(temp == 0) {
    	MesQue_Head++;
		tpos = MesQue_Head;
		if(tpos > 24) {
       		tpos = 0;
    	}
	}

	while(temp < 25) {
		if(MessageLog[tpos].Message[0] == 0) {
			MessageLog[tpos].Retries = 0;
			MessageLog[tpos].Send = FALSE;
			strcpy(MessageLog[tpos].Message, in_mes);
			MessageLog[tpos].Send_Flags = Send_Flags;
			MessageLog[tpos].Send_Only_GPRS = Send_Only_GPRS;
			MessageLog[tpos].Cellnumber = cellnumber;
			MesQue_Head = tpos;
			break;
		} else if(MessageLog[tpos].Send == TRUE) {
            MessageLog[tpos].Retries = 0;
			MessageLog[tpos].Send = FALSE;
			strcpy(MessageLog[tpos].Message, in_mes);
			MessageLog[tpos].Send_Flags = Send_Flags;
			MessageLog[tpos].Send_Only_GPRS = Send_Only_GPRS;
			MessageLog[tpos].Cellnumber = cellnumber;
			MesQue_Head = tpos;
			break;
        } else if(MessageLog[tpos].Retries > MAX_GSM_MESSAGE_RETRIES) {
            MessageLog[tpos].Retries = 0;
			MessageLog[tpos].Send = FALSE;
			strcpy(MessageLog[tpos].Message, in_mes);
			MessageLog[tpos].Send_Flags = Send_Flags;
			MessageLog[tpos].Send_Only_GPRS = Send_Only_GPRS;
			MessageLog[tpos].Cellnumber = cellnumber;
			MesQue_Head = tpos;
			break;
        }
        if(tpos > 23) {
       		tpos = 0;
    	} else tpos++;
	    temp++;
    }
	if((temp > 24)&&(temp != 30)) {
        tpos = MesQue_Head;
		if(tpos > 24) {
       		tpos = 0;
    	}
		MessageLog[tpos].Retries = 0;
		MessageLog[tpos].Send = FALSE;
		strcpy(MessageLog[tpos].Message, in_mes);
		MessageLog[tpos].Send_Flags = Send_Flags;
		MessageLog[tpos].Send_Only_GPRS = Send_Only_GPRS;
		MessageLog[tpos].Cellnumber = cellnumber;
		MesQue_Head = tpos;
	}
}


unsigned char Check_MessageQue(void)
{ unsigned char temp;
	temp = 0;
    if(MesQue_Tail > 24) {
       MesQue_Tail = 0;
    }

	while(temp < 25) {
		if((MessageLog[MesQue_Tail].Message[0] != 0)&&(MessageLog[MesQue_Tail].Retries < MAX_GSM_MESSAGE_RETRIES)&&(MessageLog[MesQue_Tail].Send == FALSE)) {
			MessageLog[MesQue_Tail].Retries++;
			memset(Out_Message, 0, 160);

				Build_Message(MessageLog[MesQue_Tail].Message, Out_Message);
				Replace_Spaces_Plus(Out_Message);
				if(strlen(Out_Message) < 3) {
					MessageLog[MesQue_Tail].Retries = MAX_GSM_MESSAGE_RETRIES;
					return(FALSE);
				} else {
					return(TRUE);   // message in que to send
				}
		}
        if(MesQue_Tail > 23) {
       		MesQue_Tail = 0;
    	} else MesQue_Tail++;
	    temp++;
    }
	return(FALSE);	// no message in que to send
}


unsigned char Check_for_Messages(void)
{
   unsigned char UserPass_Flag = FALSE;
   unsigned char ProgPass_Flag = FALSE;
   unsigned char BackdoorPass_Flag = FALSE;
   unsigned int tempyear = 0;
   unsigned int tempmonth = 0;
   unsigned int tempday = 0;
   unsigned int temphour = 0;
   unsigned int tempminute = 0;
   unsigned int tempsecond = 0;
   unsigned int tempres = 0;
   unsigned int num = 0;
   char passw[10];
   char * smspoint;
   char * temppassword;
   char * pointer;


	SIM800H_Clear_Buffers();
	smspoint = SIM800H_Check_SMS();
		if(smspoint != NULL) {
			strcpy(Incomming_SMS_Number, CellnumPoint);
			BackdoorPass_Flag = FALSE;
			UserPass_Flag = FALSE;
			ProgPass_Flag = FALSE;
			temppassword = Read_Settings(EE_USER_PASS);
			strcpy(passw, "#");
			strcat(passw, temppassword);
			if(strncmp(smspoint, "#7050", 5) == 0) {
				BackdoorPass_Flag = TRUE;
			} else if(strncmp(smspoint, passw, 5) == 0) {
				UserPass_Flag = TRUE;
			} else {
				temppassword = Read_Settings(EE_PROG_PASS);
				strcpy(passw, "#");
				strcat(passw, temppassword);
				if(strncmp(smspoint, passw, 5) == 0) {
					ProgPass_Flag = TRUE;
				}
			}

			if((ProgPass_Flag == TRUE)||(BackdoorPass_Flag == TRUE)) {
				Update_Settings_Timer = 60;   // 1 minute after programming the unit will update the settings from ram
				smspoint = smspoint + 5;  // move pointer to begining of prg
				if(strstr(smspoint, "prg") != NULL) {  // the string after the pasword starts with "prg"
					smspoint = smspoint + 3;  // move to beginnig of prog number (3 digits)
					smspoint = strtok(smspoint, "\"");
					num = atoi(smspoint);
					if(smspoint[4] == '"') {  // if clean setting
						smspoint[4] = ' ';
						smspoint[5] = '"';
					}
					smspoint = strtok(NULL, "\"");
					if(num < EE_END) {
						Save_Settings(num, smspoint);
						UserPass_Flag = FALSE;
						BackdoorPass_Flag = FALSE;
					}
					if(num > 99) {   // if numbers for drop call list
						Save_Settings(num, smspoint);
						UserPass_Flag = FALSE;
						BackdoorPass_Flag = FALSE;
					}
					if(num == EE_OWN_NUMBER) {
						Message[0] = TIMESYNC_SMS;
						Message[1] = 0;
						Add_Message_to_Que(Message, 0x01, EE_OWN_NUMBER, 0, TRUE);
						strcpy(Own_Number, Read_Settings(EE_OWN_NUMBER));
						UserPass_Flag = FALSE;
						BackdoorPass_Flag = FALSE;
					}
					Update_Mem_Flag = TRUE;
				}
			}

			if((UserPass_Flag == TRUE)||(BackdoorPass_Flag == TRUE)) {
				if(ProgPass_Flag != TRUE) {
					if(BackdoorPass_Flag != TRUE) {
						smspoint = smspoint + 5;  // move pointer to begining of rest of message
					}
				}
				if((smspoint[0] == ';')&&(smspoint[1] == '#')) {	// change user password
					smspoint = smspoint + 2;
					strcpy(passw, smspoint);
					if(strlen(passw) == 4) {
						Save_Settings(EE_USER_PASS, passw);
					}
				} else if(smspoint[0] == 'L') {  // return Status to sender
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = STATUS;
				    Message[1] = EE_AT_MESSAGE;
				    Message[2] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if(smspoint[0] == 'I') {  // return Base number to sender
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = BASE;
					Message[1] = 0;

					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if(smspoint[0] == 'J') {  // return List of Cell numbers, Call and SMS numbers to sender
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = CELLREQ;
					Message[1] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if(smspoint[0] == 'K') {  // return Passwords to sender and own number
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = PASSWORDS;
					Message[1] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if((smspoint[0] == 'G')&&(smspoint[1] == '1')) {  // Set GPRS as Main Coms method and save setting!!
				    Save_Settings(EE_SETTING_COMMS_GPRS, "GPRS");
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = EE_GPRS_ENABLED;
					Message[1] = EE_AT_MESSAGE;
					Message[2] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if((smspoint[0] == 'G')&&(smspoint[1] == '0')) {  // Set SMS as Main Coms method and save setting!!
				    Save_Settings(EE_SETTING_COMMS_GPRS, "SMS");
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = EE_GPRS_DISABLED;
					Message[1] = EE_AT_MESSAGE;
					Message[2] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if(smspoint[0] == 'T') {  // return Signal strength to sender
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = SIGNAL_STRENGTH;
					Message[1] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if(smspoint[0] == 'v') {  // return Version and Product ID to sender
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = SOFTWARE_VERSION;
					Message[1] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if(smspoint[0] == 't') {  // return Date Time to sender
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = DATETIME;
					Message[1] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if(smspoint[0] == 's') {  // return zone setup
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = ZONESETUP;
					Message[1] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
				} else if(smspoint[0] == 'Z') {  // return AirTime
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
				    Message[0] = AIRTIME;
					Message[1] = 0;
					Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
					Request_AitTime_Flag = TRUE;
				} else if(smspoint[0] == 'C') {  // Enable or Disable SMS in Chime mode
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
					if(smspoint[1] == '1') {
						ChimeSMS_Enabled = 1;
						Save_Settings(EE_SETTING_SEND_SMS_IN_CHIME, "1");

						Message[0] = EE_SMS_IN_CHIME_MODE;
						Message[1] = EE_BY_MESSAGE;
						Message[2] = EE_INCOMMING_MESSAGE_NUMBER;
						Message[3] = EE_AT_MESSAGE;
						Message[4] = 0;
						Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
					} else if(smspoint[1] == '0') {
						ChimeSMS_Enabled = 0;
						Save_Settings(EE_SETTING_SEND_SMS_IN_CHIME, "0");

						Message[0] = EE_SMS_NOT_IN_CHIME_MODE;
						Message[1] = EE_BY_MESSAGE;
						Message[2] = EE_INCOMMING_MESSAGE_NUMBER;
						Message[3] = EE_AT_MESSAGE;
						Message[4] = 0;
						Add_Message_to_Que(Message, BackdoorPass_Flag, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
					}

				} else if(smspoint[0] == 'A') {  // ARM
					if(Arm_Status != 1) {
						Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
						Message[0] = EE_AWAY_MODE;
						Message[1] = EE_BY_MESSAGE;
						Message[2] = EE_INCOMMING_MESSAGE_NUMBER;
						Message[3] = EE_AT_MESSAGE;
						Message[4] = 0;

						Add_Message_to_Que(Message, 0, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);

						HAL_GPIO_WritePin(OUT_ONOFF_GPIO_Port, OUT_ONOFF_Pin, GPIO_PIN_SET);
						Siren_Annunciate = 1;
						Siren_Annunciate_Delay = 0;
						Arm_Status = 1;
					}
				} else if(smspoint[0] == 'O') {  // OFF Mode
					ZoneTriggers[0] = 0;
					ZoneTriggers[1] = 0;
					ZoneTriggers[2] = 0;
					ZoneTriggers[3] = 0;
					ZoneTriggers[4] = 0;
					ZoneTriggers[5] = 0;

					if((Arm_Status == 1)||(Arm_Status == 2)) {
						Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
						Message[0] = EE_SYSTEM_OFF_MODE;
						Message[1] = EE_BY_MESSAGE;
						Message[2] = EE_INCOMMING_MESSAGE_NUMBER;
						Message[3] = EE_AT_MESSAGE;
						Message[4] = 0;

						Add_Message_to_Que(Message, 0, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);

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
				} else if(smspoint[0] == 'M') {  // Chime Mode
					if(Arm_Status != 2) {
						Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
						Message[0] = EE_MONITOR_MODE;
						Message[1] = EE_BY_MESSAGE;
						Message[2] = EE_INCOMMING_MESSAGE_NUMBER;
						Message[3] = EE_AT_MESSAGE;
						Message[4] = 0;

						Add_Message_to_Que(Message, 0, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
						Arm_Status = 2;

						Buzzer_Annunciate = 3;
						Siren_Annunciate_Delay = 0;
					}
				} else if(smspoint[0] == 'P') {  // Panic Mode
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
					Message[0] = EE_PANIC;
					Message[1] = EE_BY_MESSAGE;
					Message[2] = EE_INCOMMING_MESSAGE_NUMBER;
					Message[3] = EE_AT_MESSAGE;
					Message[4] = 0;

					Add_Message_to_Que(Message, 0, EE_INCOMMING_MESSAGE_NUMBER, 0, !UserPass_Flag);
					Panic_Out_Time = OUTPUT_ON_TIME + 1;
					Siren_Time = SIREN_SECONDS;
				} else if(smspoint[0] == 'H') {  // Silent Panic Mode
					Save_Settings(EE_INCOMMING_MESSAGE_NUMBER, Incomming_SMS_Number);
					Message[0] = EE_SILENT_PANIC;
					Message[1] = EE_BY_MESSAGE;
					Message[2] = EE_INCOMMING_MESSAGE_NUMBER;
					Message[3] = EE_AT_MESSAGE;
					Message[4] = 0;

					Add_Message_to_Que(Message, 0, 0, EE_INCOMMING_MESSAGE_NUMBER, !UserPass_Flag);
					Buzzer_Time = Silent_Panic_Buzzer_Time;
					Panic_Out_Time = OUTPUT_ON_TIME + 1;

				}

			}

			if(strncmp(smspoint, "TIME SYNC", 9) == 0) {
				// PROBLEM if +272 or 082.....
				if(strstr(Incomming_SMS_Number, Own_Number) != NULL) {   // if this message came fom the unit itself. Sync date time from the message
				//// Sync Time Start
					pointer = strtok(DatePoint, "/");
					tempyear = atoi(pointer);
					pointer = strtok(NULL, "/");
					tempmonth = atoi(pointer);
					pointer = strtok(NULL, ",");
					tempday = atoi(pointer);
					pointer = strtok(NULL, ":");
					temphour = atoi(pointer);
					pointer = strtok(NULL, ":");
					tempminute = atoi(pointer);
					pointer = strtok(NULL, "+-");
					tempsecond = atoi(pointer);

					if((tempyear > 10)&&(tempyear < 50)) {
						if((tempmonth > 0)&&(tempmonth < 13)) {
							if((tempday > 0)&&(tempday < 32)) {
								if(temphour < 24) {
									if(tempminute < 60) {
										if(tempsecond < 60) {
											RTC_TimeTypeDef sTime;
											RTC_DateTypeDef sDate;

											sTime.Hours = temphour;
											sTime.Minutes = tempminute;
											sTime.Seconds = tempsecond;
											sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
											sTime.StoreOperation = RTC_STOREOPERATION_RESET;
											if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
											{
											  Error_Handler();
											}

											sDate.WeekDay = RTC_WEEKDAY_MONDAY;

											sDate.Month = tempmonth;
											sDate.Date = tempday;
											sDate.Year = tempyear;

											if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
											{
											   Error_Handler();
											}
										}
									}
								}
							}
						}
					}
				//// Sync Time end
				}
			}
			osDelay(2000);
			SIM800H_Delete_SMS(SimPos);
			SIM800H_Clear_Buffers();
			return(TRUE);
		}
		SIM800H_Clear_Buffers();

		return(FALSE);
}


void Get_AirTime(void)
{
	 char * tpoint = 0;
	 char * tpointend = 0;
	 unsigned int count = 0;

	    SIM800H_TX("AT+CUSD=1,\"");
	    SIM800H_TX(Read_Settings(EE_PREPAID_USSD));
	    SIM800H_TX("\",15\r\n");
	    osDelay(500);
		SIM800H_Clear_Buffers();

		if(SIM800_Wait_for_Responce(200)) {
				U2Transfer_cplt = 0;
				tpoint = strstr(U2Rx_Buffer, "+CUSD:");
				if(tpoint != NULL) {
					memset(AirTime, 0, 80);
					//Check to see if system returned "unavailable" or "Advertise", if so try again
					if(strstr(U2Rx_Buffer, "Airtime is") || strstr(U2Rx_Buffer, "airtime is") != NULL){
						if(tpoint[4] != '4') {
							tpoint = strstr(U2Rx_Buffer, "Airtime is");
							tpointend = strstr(U2Rx_Buffer, "SMS") + 3;
							while(tpoint != tpointend && count < 60){
								AirTime[count++] = *tpoint++;
							}
							AirTime[count] = '\0';
							//strncpy(AirTime, tpoint, 60);
							strcat(AirTime, " fineautomation.co.za");
							Request_AitTime_Flag = FALSE;
							Airtime_Failure = 0;
						}
					}
					else{
						sprintf(AirTime,"Service provider did not provide correct details");
						Airtime_Failure = 1;
					}

					SIM800H_TX("AT+CUSD=2\r\n");
					osDelay(500);

				}
		}
}

char* Replace_Spaces_Plus(char* inputString){

	unsigned int length = 0;
	unsigned int currentPos = 0;

	length = strlen(inputString);
	for(unsigned int i = 0; i < length; i++){
		if(inputString[i] == ' ' || inputString[i] == '#' || inputString[i] == '\''){
			Whats_Message[currentPos++] = '+';
		}
		else{
			Whats_Message[currentPos++] = inputString[i];
		}
	}
	Whats_Message[currentPos] = '\0';

	return Whats_Message;
}

unsigned char SIMA76_HTTP_Start(void)
{
	unsigned int i = 0;
	unsigned char a;

	for(i = 0; i < 10; i++) {
		SIM800H_TX("AT+HTTPINIT\r\n");
		SIM800H_Clear_Buffers();
		for(a = 0; a < 10; a++) {
	  		if(SIM800H_Wait_for_Responce(10)) {
	  			U2Transfer_cplt = 0;
	  			if(strstr(U2Rx_Buffer, "OK") != NULL) {
	  	  			return(0);
	  	  		}
	  			if(strstr(U2Rx_Buffer, "ERROR") != NULL) {
	  				break;
	  			}
	  	  	}
	  	}
		if(strstr(U2Rx_Buffer, "OK") != NULL) {
			return(0);
		}
		else{
			SIM800H("AT+HTTPTERM\r\n");
			return(1);
		}
	}

}

static unsigned char SIMA76_HTTP_Send_Message(char * number, char * message, unsigned char cellPosition)
{
//Cell Position is a number from 2 to 7 (2-cell1, 3-cell2, 4-cell3, 5-cell4, 6-cell5, 7-cell6)
	//______________________________________________________________________________________
	//
	// This function first checks if a APIKEY for the relevant cellPosition is available, otherwise
	// it cannot send message. Return (1). If available, try sending HTTP POST at least 3 times
	// before returning a (1). If successfully send message return (0).
	//
	//______________________________________________________________________________________

	if(strlen(Read_Settings(EE_CELL1_API+(cellPosition-2))) < 4){//no valid APIKEY is present return (1)
		return(1);
	}

	for(unsigned int i = 0; i < 3; i++){

		HAL_IWDG_Refresh(&hiwdg);
		SIM800H_TX("AT+HTTPINIT\r\n");
		osDelay(50);
		char wlgMessage[300];
		char* cellnum = Read_Settings(EE_CELL1_API+(cellPosition-2));
		sprintf(wlgMessage,"AT+HTTPPARA=\"URL\",\"%s%s%s%s%s%s\"\r\n",HTTP_URL_1,number,HTTP_URL_2,message,HTTP_URL_3,cellnum);
		SIM800H_TX(wlgMessage);
		SIM800H_Clear_Buffers();

		__HAL_DBGMCU_FREEZE_IWDG();
		for(unsigned int j = 0; j < 5; j++){
			if(SIM800_Wait_for_Responce(20) || j == 4) {
				U2Transfer_cplt = 0;
				if(strstr(U2Rx_Buffer, "OK") != NULL) {
					HAL_IWDG_Refresh(&hiwdg);
					// send HTTP request
					SIM800H_TX("AT+HTTPACTION=0\r\n");
					SIM800H_Clear_Buffers();
					//This for loop takes place of delay and waits for a response
					//from the unit and therefore less chance that it misses response
					for(unsigned int i = 0; i < 4; i++){
						HAL_IWDG_Refresh(&hiwdg);
					if(SIM800_Wait_for_Responce(100)) {
						U2Transfer_cplt = 0;
						if(strstr(U2Rx_Buffer, ",200,") != NULL) {
							return(0);
						}
						if(strstr(U2Rx_Buffer, ",601,") != NULL) {
							SIM800H_TX("AT+SAPBR=1,1\r\n");
							osDelay(3000);
						}
					}
				  }
				}
				else if(strstr(U2Rx_Buffer, "ERROR") != NULL){
					return(1);
				}
			}
		}
	}

		return(1);
}


unsigned char SIM800_Wait_for_Responce(unsigned int timeout)  // 100ms intervals
{ unsigned int a = 0;
	for(a = 0; a < timeout; a++) {
		osDelay(100);
		if(U2Transfer_cplt == 1) {
			return(1);
		}
	}
	return(0);
}


unsigned char SIM800H_Start(void)
{
   	HAL_GPIO_WritePin(GSM_PWR_GPIO_Port, GSM_PWR_Pin, 1);
   	osDelay(500);
   	HAL_GPIO_WritePin(GSM_ONOFF_GPIO_Port, GSM_ONOFF_Pin, 1);
   	osDelay(1000);
   	HAL_GPIO_WritePin(GSM_ONOFF_GPIO_Port, GSM_ONOFF_Pin, 0);
   	osDelay(3000);
   	U2Rx_indx = 0;
	SIM800H_TX("AT\r\n");
	osDelay(1000);
	SIM800H_TX("AT\r\n");
	osDelay(1000);
	HAL_IWDG_Refresh(&hiwdg);
	if(U2Rx_indx < 2) {
		HAL_GPIO_WritePin(GSM_ONOFF_GPIO_Port, GSM_ONOFF_Pin, 1);
		osDelay(1000);
		HAL_GPIO_WritePin(GSM_ONOFF_GPIO_Port, GSM_ONOFF_Pin, 0);
		osDelay(2000);
		U2Rx_indx = 0;
		SIM800H_TX("AT\r\n");
		osDelay(1000);
		SIM800H_TX("AT\r\n");
	}
	HAL_IWDG_Refresh(&hiwdg);

	osDelay(6000);
	HAL_IWDG_Refresh(&hiwdg);
	if(!SIM800H_CPIN()){
		SIM800H_Defaults();
		SIM800H_Register_Status();
		strcpy(CCID, SIM800H_CCID());
		SIM800H_CSQ();
		 HAL_IWDG_Refresh(&hiwdg);
		if(SIM800H_GPRS_Setup() == 0) {
			GPRS_Barear_Started = FALSE;
			Restart_GSM_Connection = TRUE;
			return(0);
		} else {
			GPRS_Barear_Started = TRUE;
			return(1);
		}
	}
	NO_SIM = 1;
	osDelay(1000);
	return(0);

}


unsigned char SIM800H_Restart(void)
{
	 HAL_IWDG_Refresh(&hiwdg);
	SIM800H_Shutdown();
	 HAL_IWDG_Refresh(&hiwdg);
	osDelay(5000);
	 HAL_IWDG_Refresh(&hiwdg);
	if(SIM800H_Start()) {
		return(1);
	} else {
		return(0);
	}
}


static void SIM800H_Shutdown(void)
{
	osDelay(1000);
	SIM800H_TX("AT+CPOWN=1\r\n");
	osDelay(2000);
	HAL_GPIO_WritePin(GSM_PWR_GPIO_Port, GSM_PWR_Pin, 0);
}

static void SIM800H_Defaults(void)
{
	unsigned int i = 0;
	unsigned char a;

	for(i = 0; i < 10; i++) {
		SIM800H_TX(GSM_DEFAULTS);
		osDelay(5);
		HAL_IWDG_Refresh(&hiwdg);
		SIM800H_Clear_Buffers();
		for(a = 0; a < 10; a++) {
	  		if(SIM800_Wait_for_Responce(10)) {
	  			U2Transfer_cplt = 0;
	  			if(strstr(U2Rx_Buffer, "OK") != NULL) {
	  	  			break;
	  	  		}
	  	  	}
	  	}
		if(strstr(U2Rx_Buffer, "OK") != NULL) {
			break;
		}
	}

	for(i = 0; i < 10; i++) {
		SIM800H_TX(GSM_DEFAULTS_HTTP);
		osDelay(5);
		HAL_IWDG_Refresh(&hiwdg);
		SIM800H_Clear_Buffers();
		for(a = 0; a < 10; a++) {
	  		if(SIM800_Wait_for_Responce(10)) {
	  			U2Transfer_cplt = 0;
	  			if(strstr(U2Rx_Buffer, "OK") != NULL) {
	  	  			break;
	  	  		}
	  	  	}
	  	}
		if(strstr(U2Rx_Buffer, "OK") != NULL) {
			break;
		}
	}
}


unsigned char SIM800H_Register_Status(void)
{
	unsigned int i = 0;

	if(creg_timer > 0) {
		return(1);
	}
	for(i = 0; i < 10; i++) {
		SIM800H_TX("AT+CREG?\r\n");
		SIM800H_Clear_Buffers();
		if(SIM800_Wait_for_Responce(20)) {
			U2Transfer_cplt = 0;
			if(strstr(U2Rx_Buffer, "+CREG: 0,1") != NULL) {
				creg_timer = 60;
				return(1);
			} else if(strstr(U2Rx_Buffer, "+CREG: 0,5") != NULL) {
				creg_timer = 60;
				return(1);
			}
		}
		osDelay(1000);
	}
	return(0);
}

// Register status GPRS


unsigned char * SIM800H_CCID(void)
{
	unsigned int i = 0;

	for(i = 0; i < 3; i++) {
		SIM800H_TX("AT+CCID\r\n");
		SIM800H_Clear_Buffers();
		if(SIM800_Wait_for_Responce(20)) {
			U2Transfer_cplt = 0;
			if(strstr(U2Rx_Buffer, "OK") != NULL) {
				return(U2Rx_Buffer);
			}
		}
		osDelay(300);
	}
	return(NULL);
}

unsigned char SIM800H_CSQ(void)
{
	unsigned int i = 0;
    char * tpoint;
    HAL_IWDG_Refresh(&hiwdg);
	for(i = 0; i < 3; i++) {
		SIM800H_TX("AT+CSQ\r\n");
		SIM800H_Clear_Buffers();
		if(SIM800_Wait_for_Responce(10)) {
			U2Transfer_cplt = 0;
			tpoint = strstr(U2Rx_Buffer, "+CSQ:");
			if(tpoint != NULL) {
				osDelay(500);
				tpoint = tpoint + 6;
				tpoint = strtok(tpoint, ",");
				if(strlen(tpoint) > 5) {
					GSM_Signal = 0;
					return(FALSE);
				} else {
					GSM_Signal = atoi(tpoint);
				}
				return(1);
			}
		}
		osDelay(300);
	}
	return(0);
}

unsigned char SIM800H_CPIN(void)
{
	unsigned int i = 0;
    char * tpoint;
    HAL_IWDG_Refresh(&hiwdg);
	SIM800H_TX("AT+CPIN?\r\n");
	SIM800H_Clear_Buffers();
	for(i=0; i<3; i++){
		if(SIM800_Wait_for_Responce(10)) {
			U2Transfer_cplt = 0;
			if(strstr(U2Rx_Buffer, "OK") != NULL){
				return(0);
			}
			if(strstr(U2Rx_Buffer, "ERROR") != NULL){
				return(1);
			}

		}
	}
	osDelay(300);
	return(0);
}

char * SIM800H_Check_SMS(void)
{  char * startpoint;
	HAL_IWDG_Refresh(&hiwdg);
	SIM800H_TX("AT+CMGL=\"ALL\"\r\n");
	SIM800H_Clear_Buffers();
	if(SIM800_Wait_for_Responce(20)) {
		U2Transfer_cplt = 0;
		startpoint = strstr(U2Rx_Buffer, "+CMGL:");
		if(startpoint != NULL) {
			CellnumPoint = strtok(startpoint, " ");

			SimPos = atoi(strtok(NULL, ","));

			CellnumPoint = strtok(NULL, "\"");
			CellnumPoint = strtok(NULL, "\"");
			CellnumPoint = strtok(NULL, "\"");

			DatePoint = strtok(NULL, "\"");
			DatePoint = strtok(NULL, "\"");
			DatePoint = strtok(NULL, "\"");

			startpoint = strtok(NULL, "\r\n");
			if(startpoint[0] == '\n') {
				startpoint++;
			}
			if(strlen(startpoint) == 0) {
				startpoint++;
			}
			return(startpoint);
		}
	}
	return(NULL);
}

unsigned char SIM800H_Delete_SMS(unsigned char simp)
{  char tchar[5] = {0};
	HAL_IWDG_Refresh(&hiwdg);
	SIM800H_TX("AT+CMGD=");
	itoa(simp, tchar, 10);
	SIM800H_TX(tchar);
	SIM800H_TX("\r\n");
	SIM800H_Clear_Buffers();
	if(SIM800_Wait_for_Responce(20)) {
		U2Transfer_cplt = 0;
		if(strstr(U2Rx_Buffer, "+CGMD") != NULL) {
			return(1);
		}
	}
	return(0);
}

static unsigned char SIM800H_Send_SMS(char * number, char * message)
{  unsigned char endchar[2];
HAL_IWDG_Refresh(&hiwdg);
	SIM800H_TX("AT+CMGS=\"");		SIM800H_TX(number); 	SIM800H_TX("\"\r\n");
	SIM800H_Clear_Buffers();
	if(SIM800_Wait_for_Responce(20)) {
		U2Transfer_cplt = 0;
		if(strstr(U2Rx_Buffer, ">") != NULL) {
			SIM800H_TX(message);
			endchar[0] = 0x1a;
			endchar[1] = 0;
			SIM800H_TX(endchar);
			HAL_IWDG_Refresh(&hiwdg);
			// send CNTRL Z
			if(SIM800_Wait_for_Responce(100)) {
				U2Transfer_cplt = 0;
				if(strstr(U2Rx_Buffer, "+CMGS") != NULL) {
					return(1);
				} else return(0);
			}
		}
	}
	return(0);
}

unsigned char SIM800H_Connect_Data(char * server, char * port)
{
	HAL_IWDG_Refresh(&hiwdg);
	SIM800H_TX("AT+CIPSTART=\"TCP\",\"");	SIM800H_TX(server); SIM800H_TX("\",");		SIM800H_TX(port);   SIM800H_TX("\r\n");
	SIM800H_Clear_Buffers();
	if(SIM800_Wait_for_Responce(1500)) {
		HAL_IWDG_Refresh(&hiwdg);
		U2Transfer_cplt = 0;
		osDelay(200);
		if(strstr(U2Rx_Buffer, "CONNECT") == NULL) {
			if(SIM800_Wait_for_Responce(1500)) {
				if(strstr(U2Rx_Buffer, "CONNECT") != NULL) {
					osDelay(100);
					return(1);
				} else if(strstr(U2Rx_Buffer, "ERROR") == NULL) {
					return(0);
				}
			}
		} else if(strstr(U2Rx_Buffer, "ERROR") == NULL) {
			return(0);
		}
	}
	return(0);
}


static unsigned char SIM800H_Disconnect_Data(void)
{

	HAL_IWDG_Refresh(&hiwdg);
	osDelay(1000);

	SIM800H_TX("+");
	osDelay(200);

	SIM800H_TX("+");
	osDelay(200);

	SIM800H_TX("+");
	osDelay(2000);

	HAL_IWDG_Refresh(&hiwdg);
	SIM800H_TX("AT+CIPCLOSE\r\n");
	SIM800H_Clear_Buffers();
	if(SIM800_Wait_for_Responce(100)) {
		U2Transfer_cplt = 0;
		osDelay(500);
		if(strstr(U2Rx_Buffer, "SHUTDOWN") != NULL) {
			return(1);
		}
	}
	return(0);
}

static unsigned char SIM800H_Send_Data(unsigned char * buf, unsigned int len)
{

	//SIM800H_TX("AT+CIPSEND\r\n");
	//SIM800H_Clear_Buffers();
	//if(SIM800_Wait_for_Responce(100)) {
	//	U2Transfer_cplt = 0;
	//	if(strstr(U2Rx_Buffer, ">") != NULL) {
			MQTT_RX_Mode = 0;  // disables the receive of echo'd messages in que
			osDelay(50);
			HAL_UART_Transmit(&huart2, buf, len, 0xFFFF);
			//buf[0] = 0x1A;
			//HAL_UART_Transmit(&huart2, buf, 1, 0xFFFF);
			//osDelay(10);
			SIM800H_Clear_Buffers();
			MQTT_RX_Mode = 1;
			U2Transfer_cplt = 0;
			// Check for Send OK and then clear buffer
	//		if(SIM800_Wait_for_Responce(100)) {
	//				U2Transfer_cplt = 0;
	//				if(strstr(U2Rx_Buffer, "SEND OK") != NULL) {
	//					return(1);
	//				}
	//		}
	//	}
	//}
	MQTT_RX_Mode = 1;
	return(1);
}

static unsigned char SIM800H_GPRS_Setup(void)
{ unsigned char a;

    HAL_IWDG_Refresh(&hiwdg);
	osDelay(500);
  	SIM800H_TX("AT+CIPSHUT\r\n");
  	SIM800H_Clear_Buffers();
  	for(a = 0; a < 5; a++) {
  		if(SIM800_Wait_for_Responce(10)) {
   	  		U2Transfer_cplt = 0;
   	  		if(strstr(U2Rx_Buffer, "OK") != NULL) {
   	   			break;
   	   		}
   	   	}
   	}

  	osDelay(500);
 	SIM800H_TX("AT+CGATT=1\r\n");
 	//osDelay(5);
  	SIM800H_Clear_Buffers();
   	for(a = 0; a < 10; a++) {
   		if(SIM800_Wait_for_Responce(10)) {
   	  		U2Transfer_cplt = 0;
   	  		if(strstr(U2Rx_Buffer, "OK") != NULL) {
   	   			break;
   	   		}
   	   	}
   	}

   	osDelay(500);
   	SIM800H_TX("AT+CIPMUX=0\r\n");
   	//osDelay(5);
   	SIM800H_Clear_Buffers();
   	for(a = 0; a < 10; a++) {
   		if(SIM800_Wait_for_Responce(10)) {
   	  		U2Transfer_cplt = 0;
   	  		if(strstr(U2Rx_Buffer, "OK") != NULL) {
   	   			break;
   	   		}
   	   	}
   	}

   	HAL_IWDG_Refresh(&hiwdg);
   	osDelay(500);
   	SIM800H_TX("AT+CIPMODE=1\r\n");
   	//osDelay(5);
   	SIM800H_Clear_Buffers();
   	for(a = 0; a < 5; a++) {
   		if(SIM800_Wait_for_Responce(10)) {
   	  		U2Transfer_cplt = 0;
   	  		if(strstr(U2Rx_Buffer, "OK") != NULL) {
   	   			break;
   	   		}
   	   	}
   	}

   	osDelay(500);
  	SIM800H_TX("AT+CSTT=\"internet\",\"\",\"\"\r\n");
  	//osDelay(5);
  	SIM800H_Clear_Buffers();
  	for(a = 0; a < 10; a++) {
  		if(SIM800_Wait_for_Responce(50)) {
  			U2Transfer_cplt = 0;
  			if(strstr(U2Rx_Buffer, "OK") != NULL) {
  	  			break;
  	  		} else if(strstr(U2Rx_Buffer, "ERROR") != NULL) {
  				return(FALSE);
  			}
  	  	}
  	}

  	HAL_IWDG_Refresh(&hiwdg);
  	SIM800H_TX("AT+CIICR\r\n");
  	SIM800H_Clear_Buffers();
  	osDelay(20);
  	for(a = 0; a < 10; a++) {
  		if(SIM800_Wait_for_Responce(100)) {
  			osDelay(500);
  			U2Transfer_cplt = 0;
  			if(strstr(U2Rx_Buffer, "OK") != NULL) {
  				break;
  			} else if(strstr(U2Rx_Buffer, "ERROR") != NULL) {
  				return(FALSE);
  			}
  		}
  	}

  	osDelay(20);
  	SIM800H_TX("AT+CIFSR\r\n");
  	SIM800H_Clear_Buffers();
  	for(a = 0; a < 10; a++) {
  		if(SIM800_Wait_for_Responce(100)) {
  			U2Transfer_cplt = 0;
  			if(strstr(U2Rx_Buffer, ".") != NULL) {
  	  			break;
  	  		} else if(strstr(U2Rx_Buffer, "ERROR") != NULL) {
  				return(FALSE);
  			}
  	  	}
  	}

  	osDelay(5);

  	return(TRUE);
}

// Send GPRS


static void SIM800H_Clear_Buffers(void)
{
	unsigned int i = 0;

	U2Rx_indx=0;
	U2Transfer_cplt = 0;
	for (i = 0;i < 512;i++) {
			U2Rx_Buffer[i]=0;
	}
}

///////////////////////////////////////////////////////// MQTT ///////////////////////////////////////////////////////////////
static unsigned char SIM800H_Connect_MQTT(char * ClientID)
{
	unsigned int i = 0;
    unsigned char buffer[100];

    //i = MQTT_Build_Connect_Packet(buffer, "E1D2M3ooottt1257");  // clientID = Etest
    //i = MQTT_Build_Connect_Packet_Username_Password(buffer, "IoT0001", "wyn4nd", "N4cM4cF33gl3!");  // werk maar disconnect as verskillende units gelyk op is.

    // Cognative Systems
    i = MQTT_Build_Connect_Packet_Username_Password(buffer, ClientID, "wyn4nd", "N4cM4cF33gl3!");
    SIM800H_Connect_Data("129.232.211.202", "1883");

    // 1 Worx
//    i = MQTT_Build_Connect_Packet(buffer, ClientID);  // clientID = Etest
//    SIM800H_Connect_Data("mqtt.1worx.co", "1883");    // Normal TCP unencrypted

    //SIM800H_Connect_Data("broker.hivemq.com", "1883");  // Normal TCP unencrypted
    //SIM800H_Connect_Data("iot.eclipse.org", "8883");  // SSL


    SIM800H_Clear_Buffers();
    MQTT_RX_Mode = 0;  // disables the receive of echo'd messages in que
    HAL_UART_Transmit(&huart2, buffer, i, 0xFFFF);
	for(i = 0; i < 100; i++) {
		osDelay(100);
		if(U2Rx_indx > 3) {
			if((U2Rx_Buffer[U2Rx_indx-4] == 0x20)&&(U2Rx_Buffer[U2Rx_indx-3] == 0x02)&&(U2Rx_Buffer[U2Rx_indx-2] == 0)&&(U2Rx_Buffer[U2Rx_indx-1] == 0)) {
				MQTT_RX_Mode = 1;
				return(1);
			}
		}
	}
	MQTT_RX_Mode = 1;
	return(0);
}

static unsigned char SIM800H_Send_MQTT(char * topic, char * message, unsigned char retry)
{
	unsigned int i = 0;
	unsigned int Packet_ID = 0;
    unsigned int TempInt = 0;
    unsigned char P_H = 0;
    unsigned char P_L = 0;

	Packet_ID = Get_MQTT_Packet_ID();
	TempInt = Packet_ID >> 8;
	P_H = TempInt & 0xFF;
	TempInt = Packet_ID & 0xFF;
	P_L = TempInt;
	i = MQTT_Publish_Message(mqttbuffer, topic, message, retry);

	SIM800H_Clear_Buffers();
	MQTT_RX_Mode = 0;  // disables the receive of echo'd messages in que
	HAL_UART_Transmit(&huart2, mqttbuffer, i, 0xFFFF);

	for(i = 0; i < 100; i++) {
		osDelay(100);
		if(U2Rx_indx > 2) {
			if((U2Rx_Buffer[0] == 0x40)&&(U2Rx_Buffer[1] == 0x02)) {  // &&(U2Rx_Buffer[U2Rx_indx-2] == P_H)&&(U2Rx_Buffer[U2Rx_indx-1] == P_L)
				MQTT_RX_Mode = 1;
				return(1);
			}
		}
	}
	MQTT_RX_Mode = 1;
	return(0);
}


static unsigned char SIM800H_Sub_MQTT(char * topic)
{
	unsigned int i = 0;
    unsigned char buffer[100];

    i = MQTT_Subscribe_Message(buffer, 1, topic);

    SIM800H_Clear_Buffers();
    MQTT_RX_Mode = 0;  // disables the receive of echo'd messages in que
    HAL_UART_Transmit(&huart2, buffer, i, 0xFFFF);
	// check for connect  should receive 0x02 0x00 0x00.  Length = 2 and 2 bytes 00 00
	for(i = 0; i < 100; i++) {
		if(U2Rx_indx > 3) {
			if((U2Rx_Buffer[U2Rx_indx-4] == 2)&&(U2Rx_Buffer[U2Rx_indx-3] == 0)&&(U2Rx_Buffer[U2Rx_indx-2] == 0)) {
				MQTT_RX_Mode = 1;
				return(1);
			}
		}
	}
	MQTT_RX_Mode = 1;
	return(0);
}

static unsigned char SIM800H_Ping_MQTT(void)
{
	unsigned int i = 0;
    unsigned char buffer[100];

    i = MQTT_Ping_Message(buffer);

    SIM800H_Clear_Buffers();
    MQTT_RX_Mode = 0;  // disables the receive of echo'd messages in que
    HAL_UART_Transmit(&huart2, buffer, i, 0xFFFF);

	for(i = 0; i < 100; i++) {
		osDelay(100);
		if(U2Rx_indx > 1) {
			if((U2Rx_Buffer[0] == 0xD0)&&(U2Rx_Buffer[1] == 0)) {
				MQTT_RX_Mode = 1;
				return(1);
			}
		}
	}
	MQTT_RX_Mode = 1;
	return(0);
}

static unsigned char SIM800H_Disconnect_MQTT(void)
{
	unsigned int i = 0;
    unsigned char buffer[100];

    i = MQTT_Disconnect_Message(buffer);

    SIM800H_Clear_Buffers();
    MQTT_RX_Mode = 0;  // disables the receive of echo'd messages in que
    HAL_UART_Transmit(&huart2, buffer, i, 0xFFFF);
	for(i = 0; i < 100; i++) {
		osDelay(100);
		if(U2Rx_indx > 3) {
			if((U2Rx_Buffer[U2Rx_indx-2] == 0xD0)&&(U2Rx_Buffer[U2Rx_indx-1] == 0)) {
			MQTT_RX_Mode = 1;
			return(1);
			}
		}
	}
	MQTT_RX_Mode = 1;
	return(0);
}

unsigned char Send_GPRS_SIM800(char * serv_num, char * message, unsigned char Mes_Amount)
{ unsigned char temppoint = 0;
  unsigned char rtries = 0;
  unsigned char Result = 0;
  unsigned int temp = 0;
  unsigned int ContentLength = 0;


  char bytes[10];
  char * numpoint;

    memset(buf, 0, sizeof(buf));
  //  if(GPRS_Failed == TRUE) {
//		return(FALSE);
//	}

    unsigned char server_ip[30];
    unsigned char server_port[10];

  	Result = FALSE;
	temppoint = FALSE;
    rtries = 0;

    strncpy(server_ip, Read_Settings(EE_TCP_SERV), 29);
    strncpy(server_port, Read_Settings(EE_TCP_PORT), 9);

	while(SIM800H_Connect_Data(server_ip, server_port) == 0) {
		if(rtries < 2) {
			rtries++;
		} else return(FALSE);
	}

    while(temppoint == FALSE) {
    	strcpy(buf, POST);
		strcat(buf, "/gprs/gprs.php?cmd=sms&c=");
		numpoint = Own_Number;
		if(Own_Number[0] == '+') {
			numpoint++;
		}
		strcat(buf, numpoint);
		strcat(buf, "&u=");
		strcat(buf, Read_Settings(EE_HTTP_USERNAME));
		strcat(buf, "&p=");
		strcat(buf, Read_Settings(EE_HTTP_PASSWORD));
		strcat(buf, "&id=");
		itoa(mesid, bytes, 10);
		strcat(buf, bytes);
		strcat(buf, HTTP);
		strcat(buf, "\r\n");

		strcat(buf, HOST);
		strcat(buf, Read_Settings(EE_TCP_SERV));
		strcat(buf, Colon);
		strcat(buf, Read_Settings(EE_TCP_PORT));
		strcat(buf, "\r\n");
		strcat(buf, User_Agent);
		strcat(buf, "USER_AGENT");
		strcat(buf, "\r\n");
		strcat(buf, Content_Length_part);
		if(serv_num[0] == '+') serv_num++;
		ContentLength = strlen(message) + 11 + 2 + strlen(serv_num) + 1 + 2;

		itoa(ContentLength, bytes, 10);
		strcat(buf, bytes);

		strcat(buf, "\r\n\r\n");
		mesid++;
		strcat(buf, "<sms>");
		strcat(buf, serv_num);
		strcat(buf, ";");
		strcat(buf, message);
		strcat(buf, " G");
		strcat(buf, "</sms>\r\n");

		HAL_UART_Transmit(&huart2, buf, strlen(buf), 0xFFFF);
		osDelay(50);
		SIM800H_Clear_Buffers();

		temppoint = FALSE;
		if(SIM800_Wait_for_Responce(200)) {
			U2Transfer_cplt = 0;
			if(strstr(U2Rx_Buffer, "HTTP/1.1 200") != NULL) {
				temppoint = TRUE;
				Result = TRUE;
			}
		}

		if(rtries < 2) rtries++;
			else temppoint = TRUE;
	}

    SIM800H_Disconnect_Data();
    osDelay(500);

	return(Result);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void SIM800H_TX(char *message)
{

	if (HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), 0xFFFF) != HAL_OK) {
        Error_Handler();
    }
    //while (UartReady != SET) {
        // do nothing
    //}
}


void SIM800H_RX(void)
{

	// 30 16 00 08 45 31 30 31 54 65 73 74 54 65 73 74 20 4D 65 73 73 61 67 65   // MQTT Message RX
	/*if(MQTT_RX_Mode == 1) {
		if ((U2Rx_data[0] == '0')&&(MQTT_Start_Flag == 0))	//if '0' start of new MQTT received packet
		{
			MQTT_Start_Flag = 1;
			MQTT_Start_Pointer = U2Rx_indx;
		} else if(MQTT_Start_Flag == 1) {
			if(U2Rx_indx == (MQTT_Start_Pointer + 1)) {
				MQTT_RX_Length = U2Rx_data[0];
				MQTT_RXP = 0;
			} else if((U2Rx_indx == (MQTT_Start_Pointer + 2))&&(U2Rx_data[0] != 0)) {
				MQTT_Start_Flag = 0;
				MQTT_Start_Pointer = 0;
			} else {
				if((MQTT_RX_Length > 0)&&(MQTT_RXP < MQTT_RX_Length)) {
					MQTT_RX_Packet[MQTT_RXP++] = U2Rx_data[0];	//add data to Rx_Buffer
					if(MQTT_RXP >= MQTT_RX_Length) {
						MQTT_RX_Message_Flag = 1;

						MQTT_Start_Flag = 0;
						MQTT_Start_Pointer = 0;
					}
				} else {
					MQTT_Start_Flag = 0;
					MQTT_Start_Pointer = 0;
				}
			}
		}
	}*/

	if (U2Rx_data[0]!=13)	//if received data different from ascii 13 (enter)
	{
		if(U2Rx_indx > 510) {
			U2Rx_indx = 0;
		}
		U2Rx_Buffer[U2Rx_indx++] = U2Rx_data[0];	//add data to Rx_Buffer
	}
	else			//if received data = 13
	{
		U2Transfer_cplt=1;//transfer complete, data is ready to read
	}
	HAL_UART_Receive_IT(&huart2, U2Rx_data, 1);	//activate UART receive interrupt every time
}


