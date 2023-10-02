/*
 * EEPROM.c
 *
 *  Created on: 01 Aug 2018
 *      Author: wynan
 */
#include "EEPROM.h"
#include "stm32f0xx_hal.h"
#include <cmsis_os.h>

#define ZONECOUNT				6

#define EEPROM_ADDRESS          0xA0
#define EEPROM_PAGESIZE         64

/* Maximum Timeout values for flags waiting loops. These timeouts are not based
   on accurate values, they just guarantee that the application will not remain
   stuck if the I2C communication is corrupted.
   You may modify these timeout values depending on CPU frequency and application
   conditions (interrupts routines ...). */
#define I2C_XFER_TIMEOUT_MAX    300
/* Maximum number of trials for HAL_I2C_IsDeviceReady() function */
#define EEPROM_MAX_TRIALS       300

extern I2C_HandleTypeDef hi2c1;
extern unsigned char Silent_Panic_Buzzer_Time;

char * tpointer;

unsigned char ChimeSMS_Enabled = 0;
extern unsigned char GPRS_Enabled;
extern unsigned char Own_Number[];

unsigned char Health_TX_Hour = 0;
unsigned char Health_TX_Minute = 0;
unsigned char Health_TX_days = 0;

float Bat_Low_V = 0;
float Bat_Ok_V = 0;

unsigned char AwayModeZones[ZONECOUNT];
unsigned char ChimeModeZones[ZONECOUNT];
unsigned char AnyModeChimeZones[ZONECOUNT];

unsigned char SPIPageBUF[128];
unsigned char EEPROM_Field[128];

/* Buffer used for transmission */
unsigned char aTxBuffer[128];

/* Buffer used for reception */
unsigned char aRxBuffer[128];

/* Useful variables during communication */
unsigned int Memory_Address;
int Remaining_Bytes;

static unsigned int Buffercmp(unsigned char *pBuffer1, unsigned char *pBuffer2, unsigned int BufferLength);
unsigned char IIC_EEPROM_Write_Page(unsigned long Address, unsigned char length);
void IIC_EEPROM_Read_Page(unsigned long Address, unsigned char length);



void Update_Mem_Values(void)
{  char * pointer;
   unsigned int tempint;

    pointer = Read_Settings(EE_AWAY_MODE_ZONES_ENABLED);
    for(tempint = 0; tempint < ZONECOUNT; tempint++) {
    	AwayModeZones[tempint] = pointer[0];
    	pointer++;
    }

    pointer = Read_Settings(EE_CHIME_MODE_ZONES_ENABLED);
    for(tempint = 0; tempint < ZONECOUNT; tempint++) {
      	ChimeModeZones[tempint] = pointer[0];
       	pointer++;
    }

    pointer = Read_Settings(EE_CHIME_ANYMODE_ZONES);
    for(tempint = 0; tempint < ZONECOUNT; tempint++) {
       	AnyModeChimeZones[tempint] = pointer[0];
       	pointer++;
    }

    pointer = Read_Settings(EE_SETTING_SEND_SMS_IN_CHIME);
	if(pointer[0] == '0') {
		ChimeSMS_Enabled = 0;
	} else ChimeSMS_Enabled = 1;

	pointer = Read_Settings(EE_SETTING_COMMS_GPRS);
	if(strstr(pointer, "SMS") == NULL) {
		GPRS_Enabled = 1;
	} else GPRS_Enabled = 0;

	pointer = Read_Settings(EE_HEALTH_REPORT_TIME);
	tempint = atoi(pointer);
	if((tempint == 0)||(tempint > 2400)) {
		Health_TX_Hour = 0xFF;
		Health_TX_Minute = 0xFF;
	} else {
		Health_TX_Hour = (int) tempint / 100;
		Health_TX_Minute = tempint % 100;
	}

	pointer = Read_Settings(EE_HEALTH_REPORT_DAYS);
	tempint = atoi(pointer);
	if((tempint > 0)&&(tempint < 32)) {
		Health_TX_days = tempint;
	} else {
		Health_TX_days = 1;
	}

	Silent_Panic_Buzzer_Time = atoi(Read_Settings(EE_SILENT_PANIC_BUZZER_TIME));

	pointer = Read_Settings(EE_BAT_LOW_HIGH_VALUE);
	pointer = strtok(pointer, " ");
	Bat_Low_V = atof(pointer);

	pointer = strtok(pointer, " ");
	Bat_Ok_V = atof(pointer);

	if((Bat_Ok_V < 10)||(Bat_Ok_V > 15)) {
		Bat_Ok_V = 12.5;
	}

	if((Bat_Low_V < 9)||(Bat_Low_V > 15)) {
		Bat_Low_V = 10.5;
	}

	strcpy(Own_Number, Read_Settings(EE_OWN_NUMBER));
}


void Save_Settings(unsigned int page, char * savestring)
{  unsigned char leng = 0;
	if((strlen(savestring) > 61)&&(page == EE_AT_MESSAGE)) {
		if(page == 1) {  // if at message and message longer than 62 chars
			leng = strlen(savestring);
			if(leng > 124) {
				leng = 124;
			}
			leng = leng - 62;

			memcpy(aTxBuffer, savestring + 61, 64);

			IIC_EEPROM_Write_Page((EE_AT_MESSAGE + 1) * 64, 64);
		} else savestring[61] = 0;
	}
	memcpy(aTxBuffer, savestring, 64);

	IIC_EEPROM_Write_Page(page * 64, 64);
}

char * Read_Settings(unsigned int page)
{  unsigned char i;
    IIC_EEPROM_Read_Page(page * 64, 64);			// First Page
    for(i = 0; i < 64; i++) {
    	if(aRxBuffer[i] == 0xFF) {
    		aRxBuffer[i] = 0;
    		break;
    	}
    }
    memcpy(SPIPageBUF, aRxBuffer, 64);
    memcpy(EEPROM_Field, aRxBuffer, 64);

	//if(page == 1) {						// add first 2 pages for at message
	//	if(strlen(EEPROM_Field) > 61) {
	//		IIC_EEPROM_Read_Page(2 * 64);
	//		if(Check_Settings()) {
	//			strcat(EEPROM_Field, SPIPageBUF);
	//		}
	//	}
	//}
	tpointer = EEPROM_Field;
	return(tpointer);
}

void IIC_EEPROM_Read_Page(unsigned long Address, unsigned char length)
{
    /* Initialize Remaining Bytes Value to TX Buffer Size */
    Remaining_Bytes = length;

    /* Check if the EEPROM is ready for a new operation */
    while (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDRESS, EEPROM_MAX_TRIALS, I2C_XFER_TIMEOUT_MAX) == HAL_TIMEOUT);

    /* Wait for the end of the transfer */
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);


    /*##-3- Start reading process ##############################################*/
    if (HAL_I2C_Mem_Read(&hi2c1, (unsigned int)EEPROM_ADDRESS, Address, I2C_MEMADD_SIZE_16BIT, (unsigned char *)aRxBuffer, length, 1000) != HAL_OK)
    {
      /* Reading process Error */
      Error_Handler();
    }

    /* Wait for the end of the transfer */
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    }
}


unsigned char IIC_EEPROM_Write_Page(unsigned long Address, unsigned char length)
{
	/* Initialize Remaining Bytes Value to TX Buffer Size */
    Remaining_Bytes = length;

  	while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);

    /* Check if the EEPROM is ready for a new operation */
    while (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDRESS, EEPROM_MAX_TRIALS, I2C_XFER_TIMEOUT_MAX) == HAL_TIMEOUT);
    if(length > 64) {
    	/* Write EEPROM_PAGESIZE */
    	if(HAL_I2C_Mem_Write(&hi2c1 , (unsigned int)EEPROM_ADDRESS, Address, I2C_MEMADD_SIZE_16BIT, (unsigned char*)(aTxBuffer), 64, 1000)!= HAL_OK)
    	{
    		/* Writing process Error */
    		Error_Handler();
    	}
    	while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);

   	    /* Check if the EEPROM is ready for a new operation */
   	    while (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDRESS, EEPROM_MAX_TRIALS, I2C_XFER_TIMEOUT_MAX) == HAL_TIMEOUT);

   	    /* Write EEPROM_PAGESIZE */
   	    if(HAL_I2C_Mem_Write(&hi2c1 , (unsigned int)EEPROM_ADDRESS, Address, I2C_MEMADD_SIZE_16BIT, (unsigned char*)(aTxBuffer[64]), length - 64, 1000)!= HAL_OK)
   	    {
 	   	      /* Writing process Error */
  	   	      Error_Handler();
  	    }

    } else {
    	while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);

   	    /* Check if the EEPROM is ready for a new operation */
   	    //while (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDRESS, EEPROM_MAX_TRIALS, I2C_XFER_TIMEOUT_MAX) == HAL_TIMEOUT);

   	    /* Write EEPROM_PAGESIZE */
   	    if(HAL_I2C_Mem_Write(&hi2c1 , (unsigned int)EEPROM_ADDRESS, Address, I2C_MEMADD_SIZE_16BIT, (unsigned char*)(aTxBuffer), length, 1000)!= HAL_OK)
   	    {
   	      /* Writing process Error */
   	      Error_Handler();
   	    }
    }

    	/* Wait for the end of the transfer */
    	/*  Before starting a new communication transfer, you need to check the current
        	state of the peripheral; if it�s busy you need to wait for the end of current
        	transfer before starting a new one.
        	For simplicity reasons, this example is just waiting till the end of the
        	transfer, but application may perform other tasks while transfer operation
        	is ongoing. */


    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);

    /* Check if the EEPROM is ready for a new operation */
    while (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDRESS, EEPROM_MAX_TRIALS, I2C_XFER_TIMEOUT_MAX) == HAL_TIMEOUT);

    /* Wait for the end of the transfer */
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);

    /*##-3- Start reading process ##############################################*/
    if (HAL_I2C_Mem_Read(&hi2c1, (unsigned int)EEPROM_ADDRESS, Address, I2C_MEMADD_SIZE_16BIT, (unsigned char *)aRxBuffer, length, 1000) != HAL_OK)
    {
      /* Reading process Error */
      Error_Handler();
    }

    /* Wait for the end of the transfer */
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    }

    /*##-4- Compare the sent and received buffers ##############################*/
    if (Buffercmp((unsigned char *)aTxBuffer, (unsigned char *)aRxBuffer, length) > 0)
    {
    	return(1);  // write error
    }
    return(0); // ok
}


static unsigned int Buffercmp(unsigned char *pBuffer1, unsigned char *pBuffer2, unsigned int BufferLength)
{
   while (BufferLength--)
   {
      if ((*pBuffer1) != *pBuffer2)
      {
         return BufferLength;
      }
      pBuffer1++;
      pBuffer2++;
   }
   return 0;
}
