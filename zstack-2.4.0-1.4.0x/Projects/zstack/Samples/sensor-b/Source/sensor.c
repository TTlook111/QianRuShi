/*********************************************************************************************
* ???sensor.c
* ???Xuzhy 2018.5.16
* ???xLab Sensor-B?????
* ???
* ???
*********************************************************************************************/

/*********************************************************************************************
* ???11222555
*********************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sapi.h"
#include "osal_nv.h"
#include "addrmgr.h"
#include "mt.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_uart.h"
#include "sensor.h"
#include "led.h"
#include "rgb.h"
#include "fan.h"
#include "beep.h"
#include "stepmotor.h"
#include "relay.h"
#include "timer.h"
#include "zxbee.h"
#include "zxbee-inf.h"
/*********************************************************************************************
* ???
*********************************************************************************************/

/*********************************************************************************************
* ????
*********************************************************************************************/
static uint8  D0 = 0xFF;                                        // ??????????
static uint8  D1 = 0;                                           // ??????????
static uint16 V0 = 30;                                          // V0?????????????30s
static uint8  mode = 0;                                         // ?????
/*********************************************************************************************
* ???updateV0()
* ?????V0??
* ???*val -- ??????
* ???
* ???
* ???
*********************************************************************************************/
void updateV0(char *val)
{
  //??????val???????????
  V0 = atoi(val);                                 // ???????????
}
/*********************************************************************************************
* ???sensorInit()
* ???????????
* ????
* ????
* ???
* ???
*********************************************************************************************/
void sensorInit(void)
{
  ZXBeeInfSetLocalAddr(ZXBEE_ADDR_SENSOR_B);
  // ????????
  // ????P0_3 ?????????????????????
  P0SEL &= ~0x08;                                               // ???????IO??
  P0DIR &= !0x08;  
  
  if(P0_3 == 0){
    mode = 1;
    stepmotor_init();                                           // ???????
    fan_init();                                                 // ?????
  }else{
    mode = 2;
    rgb_init();                                                 // RGB????
    beep_init();                                                // ??????
  }
  
  led_init();                                                   // LED????
  relay_init();                                                 // ??????
  
  // ??????????????????MY_REPORT_EVT
  osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, (uint16)((osal_rand()%10) * 1000));
}
/*********************************************************************************************
* ???sensorLinkOn()
* ????????????????
* ????
* ????
* ???
* ???
*********************************************************************************************/
void sensorLinkOn(void)
{
  sensorUpdate();
}
/*********************************************************************************************
* ???sensorUpdate()
* ????????????
* ????
* ????
* ???
* ???
*********************************************************************************************/
void sensorUpdate(void)
{ 
  char pData[16];
  char *p = pData;
  
  ZXBeeBegin();
  
  sprintf(p, "%u", D1);                                  // ?????? 
  ZXBeeAdd("state", p);
  
  p = ZXBeeEnd();                                               // ?????????
  if (p != NULL) {												
    ZXBeeInfSend(p, strlen(p));	                                // ??????????????????zb_SendDataRequest()??????
  }
}
/*********************************************************************************************
* ???sensorCheck()
* ????????
* ????
* ????
* ???
* ???
*********************************************************************************************/
void sensorCheck(void)
{
  
}
/*********************************************************************************************
* ???sensorControl()
* ????????
* ???cmd - ????
* ????
* ???
* ???
*********************************************************************************************/
void sensorControl(uint8 cmd)
{
  static uint8 stepmotor_flag = 0;
  // ??cmd???????????
  if(mode == 1){                                                  // ????????
    if ((cmd & 0x04) == 0x04) {                                   // ????????bit2
      if(stepmotor_flag != 1) {                                   // ??????
        stepmotor_flag = 1; 
        forward(5000);
      }
    } else {                                                      // ??????
      if(stepmotor_flag != 0) {
        stepmotor_flag = 0; 
        reversion(5000);
      }
    }
	
	if(cmd & 0x08) {                                               // ??????bit3
      FANIO = FAN_ON;                                                  // ????
    } else {
      FANIO = FAN_OFF;                                                 // ????       
    }
  }
  
  if(mode == 2){                                                  // ????????
    if ((cmd & 0x01) == 0x01){                                    // RGB?????bit0~bit1
      if ((cmd & 0x02) == 0x02){                                  // cmd?3????
        RGB_R = OFF;
        RGB_G = OFF;
        RGB_B = ON;
      }
      else{                                                       // cmd?1????
        RGB_R = ON;
        RGB_G = OFF;
        RGB_B = OFF;
      }
    }
    else if ((cmd & 0x02) == 0x02){                               // cmd?2????
      RGB_R = OFF;
      RGB_G = ON;
      RGB_B = OFF;
    }
    else{                                                         // cmd?1????
      RGB_R = OFF;
      RGB_G = OFF;
      RGB_B = OFF;
    }
		
	if(cmd & 0x08) {                                              // ???????bit3
      BEEPIO = ON;                                                // ?????
    } else {
      BEEPIO = OFF;                                               // ?????      
    }
  }
  

  if(cmd & 0x20){                                               // LED2?????bit5
    LED2 = ON;                                                  // ??LED2
  }
  else{
    LED2 = OFF;                                                 // ??LED2        
  }
  if(cmd & 0x10){                                               // LED1?????bit4
    LED1 = ON;                                                  // ??LED1
  }
  else{
    LED1 = OFF;                                                 // ??LED1
  }
  
  if(cmd & 0x80){                                               // ???2????bit7
    RELAY2 = ON;                                                // ?????2
  }
  else{
    RELAY2 = OFF;                                               // ?????2        
  }
  if(cmd & 0x40){                                               // ???1????bit6
    RELAY1 = ON;                                                // ?????1
  }
  else{
    RELAY1 = OFF;                                               // ?????1
  }
}
/*********************************************************************************************
* ???ZXBeeUserProcess()
* ????????????
* ???*ptag -- ??????
*       *pval -- ??????
* ???ret -- p?????
* ???
* ???
*********************************************************************************************/
static void sensorSetRgb(char *val)
{
  int r = 0;
  int g = 0;
  int b = 0;
  if (val == NULL) return;
  sscanf(val, "[%d,%d,%d]", &r, &g, &b);
  D1 &= ~0x03;
  if (b > 0) {
    D1 |= 0x03;
  } else if (g > 0) {
    D1 |= 0x02;
  } else if (r > 0) {
    D1 |= 0x01;
  }
  sensorControl(D1);
}
int ZXBeeUserProcess(char *ptag, char *pval)
{ 
  int val;
  int ret = 0;	
  char pData[16];
  char *p = pData;
  
  // ??????pval???????????
  val = atoi(pval);
  if (0 == strcmp("unlock", ptag)){
    if (val) D1 |= 0x40;
    else D1 &= ~0x40;
    sensorControl(D1);
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("buzz", ptag)){
    if (val) D1 |= 0x08;
    else D1 &= ~0x08;
    sensorControl(D1);
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("rgb", ptag)){
    sensorSetRgb(pval);
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("reset", ptag)){
    D1 = 0;
    sensorControl(D1);
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("reset", p);
  }	
  // ??????
  if (0 == strcmp("CD0", ptag)){                                // ?D0???????CD0???????
    D0 &= ~val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("OD0", ptag)){                                // ?D0???????OD0???????
    D0 |= val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("D0", ptag)){                                 // ????????
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D0);
      ZXBeeAdd("D0", p);
    } 
  }
  if (0 == strcmp("CD1", ptag)){                                // ?D1???????CD1???????
    D1 &= ~val;
    sensorControl(D1);                                          // ??????
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("OD1", ptag)){                                // ?D1???????OD1???????
    D1 |= val;
    sensorControl(D1);                                          // ??????
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("D1", ptag)){                                 // ?????????
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D1);
      ZXBeeAdd("state", p);
    } 
  }
  if (0 == strcmp("V0", ptag)){
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", V0);                         	// ??????
      ZXBeeAdd("V0", p);
    }else{
      updateV0(pval);
    }
  } 
  return ret;
}
/*********************************************************************************************
* ???MyEventProcess()
* ??????????
* ???event -- ????
* ????
* ???
* ???
*********************************************************************************************/
void MyEventProcess( uint16 event )
{
  if (event & MY_REPORT_EVT) { 
    sensorUpdate();
    //???????????MY_REPORT_EVT 
    osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, V0*1000);
  }  
}

