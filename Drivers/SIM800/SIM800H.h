/*
 * SIM800H.h
 *
 *  Created on: Jul 6, 2016
 *      Author: User
 */

#ifndef SIM800H_H_
#define SIM800H_H_

#include "stm32f0xx_hal_def.h"

unsigned char SIM800H_Start(void);
unsigned char SIM800H_Restart(void);
static void SIM800H_Shutdown(void);
static void SIM800H_Defaults(void);
static void SIM800H_Init(void);
static void SIM800H_Clear_Buffers(void);
static void SIM800H_TX(char *message);
void SIM800H_RX(void);
unsigned char SIM800H_Register_Status(void);
unsigned char * SIM800H_CCID(void);
unsigned char SIM800H_CSQ(void);
unsigned char SIM800H_CPIN(void);
char * SIM800H_Check_SMS(void);
unsigned char SIM800H_Delete_SMS(unsigned char simp);
static unsigned char SIM800_Wait_for_Responce(unsigned int timeout);
static unsigned char SIM800H_GPRS_Setup(void);
unsigned char SIM800H_Connect_Data(char * server, char * port);
static unsigned char SIM800H_Disconnect_Data(void);
static unsigned char SIM800H_Send_Data(unsigned char * buf, unsigned int len);
char* Replace_Spaces_Plus(char* inputString);
unsigned char SIMA76_HTTP_Start(void);

// MQTT //
static unsigned char SIM800H_Connect_MQTT(char * ClientID);
static unsigned char SIM800H_Send_MQTT(char * topic, char * message, unsigned char retry);
static unsigned char SIM800H_Ping_MQTT(void);

extern UART_HandleTypeDef huart2;
extern unsigned char U2Rx_data[2];

#endif /* SIM800H_H_ */
