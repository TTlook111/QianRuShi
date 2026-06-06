/*********************************************************************************************
* ???sensor.c
* ???Xuzhy 2018.5.16
* ???xLab Sensor-C?????
* ???fuyou 2018.08.03 ???????????
* ???
*********************************************************************************************/
/*
 * mode????????????????mode
 *  1: ????????????&??&?????
 *  2????????????????
 *
 */
/*********************************************************************************************
* ???
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
#include "Flame.h"
#include "grating.h"
#include "hall.h"
#include "relay.h"
#include "infrared.h"
#include "MP-4.h"
#include "Touch.h"
#include "vibration.h"
#include "syn6288.h"
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
static uint8  A0 = 0;                                           // mode=1 ????????mode=2 ?????
static uint8  A1 = 0;                                           // mode=1 ??????mode=2 ???????
static uint8  A2 = 0;                                           // mode=1 ??????mode=2 ???????
static uint8  A3 = 0;                                           // mode=1 ??????mode=2 ???????
static uint8  A4 = 0;                                           // ??????
static uint8  A5 = 0;                                           // ??????
static uint16 V0 = 30;                                          // V0?????????????30s
static char*  V1;                                               // ????????????
static uint8 mode = 1;  
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
  V0 = atoi(val);                                               // ???????????
}
/*********************************************************************************************
* ???updateA0()
* ?????A0??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA0(void)
{
   static uint32 ct = 0;
   
  if (mode == 1) {                                               // ???????????
    A0 = get_infrared_status();
    if (A0 != 0) {
      ct = osal_GetSystemClock();
    } else if (osal_GetSystemClock() > ct+1000) {
      ct = 0;
      A0 = 0;
    } else {
      A0 = 1;
    }
  }
}
/*********************************************************************************************
* ???updateA1()
* ?????A1??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA1(void)
{
   static uint32 ct = 0;
  
  if (mode == 1) {                                              // ?????????
    A1 = get_vibration_status();
    if (A1 != 0) {
      ct = osal_GetSystemClock();
    } else if (osal_GetSystemClock() > ct+2000) {
      ct = 0;
      A1 = 0;
    } else {
      A1 = 1;
    }
  }
}
/*********************************************************************************************
* ???updateA2()
* ?????A2??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA2(void)
{
  if (mode == 1) {
    A2 = get_hall_status();                                     // ?????????
  }
}
/*********************************************************************************************
* ???updateA3()
* ?????A3??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA3(void)
{
  static uint32 ct = 0;
  
  if (mode == 1) {
    A3 = get_flame_status();                                    // ?????????
    if (A3 != 0) {
      ct = osal_GetSystemClock();
    } else if (osal_GetSystemClock() > ct+2000) {
      ct = 0;
      A3 = 0;
    } else {
      A3 = 1;
    }
  }
}
/*********************************************************************************************
* ???updateA4()
* ?????A4??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA4(void)
{
  A4 = get_combustiblegas_data();                               // ?????????
}
/*********************************************************************************************
* ???updateA5()
* ?????A5??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA5(void)                                             // ?????????
{
  A5 = get_grating_status();
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
  ZXBeeInfSetLocalAddr(ZXBEE_ADDR_SENSOR_C);
  // ????????
  vibration_init();                                             // ????????????????????????????
  infrared_init();                                              // ??????????
  flame_init();                                                 // ????????
  hall_init();                                                  // ????????
  combustiblegas_init();                                        // ????????
  grating_init();                                               // ????????
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
  
  ZXBeeBegin();                                                 // ?????????
  
  // ??D0???????????????
  if ((D0 & 0x01) == 0x01){                                     // ?????????pData???????????
    updateA0();
    sprintf(p, "%u", A0); 
    ZXBeeAdd("pir", p);
  }
  if (mode == 1) {
    if ((D0 & 0x02) == 0x02){                                     // ?????????pData???????????
      updateA1();
      sprintf(p, "%u", A1); 
      ZXBeeAdd("tch", p);
    }   
    if ((D0 & 0x04) == 0x04){                                     // ?????????pData???????????
      updateA2();
      sprintf(p, "%u", A2);  
      ZXBeeAdd("door", p);
    }
    if ((D0 & 0x08) == 0x08){                                     // ???????????pData?????????????
      updateA3();
      sprintf(p, "%u", A3);  
      ZXBeeAdd("alert", p);
    }
  }
  if ((D0 & 0x10) == 0x10){                                     // ???????????pData?????????????
    updateA4();
    sprintf(p, "%u", A4);
    ZXBeeAdd("A4", p);
  }
  if ((D0 & 0x10) == 0x10){                                     // ???????????pData?????????????
    updateA5();
    sprintf(p, "%u", A5);
    ZXBeeAdd("A5", p);
  }
  
  sprintf(p, "%u", D1);                                  // ?????? 
  ZXBeeAdd("D1", p);
  
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
  static char lastA0 = 0,lastA1=0,lastA2=0,lastA3=0,lastA4=0,lastA5=0;
  static uint32 ct0=0, ct1=0, ct2=0, ct3=0, ct4=0, ct5=0;
  char pData[16];
  char *p = pData;
  
  updateA0();
  updateA1();
  updateA2();
  updateA3();
  updateA4();
  updateA5();
  
  ZXBeeBegin();
  
  if (lastA0 != A0 
      || ( //??1????????????????3S?????2????????????????1??  
          (mode == 1)&&ct0 != 0 && osal_GetSystemClock() > (ct0+3000)) //??3???
      ) {
    if (D0 & 0x01) {
      sprintf(p, "%u", A0);
      ZXBeeAdd("pir", p);
      ct0 = osal_GetSystemClock();
    }
    if (A0 == 0) {
        ct0 = 0;
    }
    lastA0 = A0;
  }
  if (mode == 1) {   // ??1??????????????
    if (lastA1 != A1 || (ct1 != 0 && osal_GetSystemClock() > (ct1+3000))) {  // ???????
      if (D0 & 0x02) {
        sprintf(p, "%u", A1);
        ZXBeeAdd("tch", p);
        ct1 = osal_GetSystemClock();
      }
      if (A1 == 0) {
        ct1 = 0;
      }
      lastA1 = A1;
    }
    if (lastA2 != A2 || (ct2 != 0 && osal_GetSystemClock() > (ct2+3000))) {  // ???????
      if (D0 & 0x04) {
        sprintf(p, "%u", A2);
        ZXBeeAdd("door", p);
        ct2 = osal_GetSystemClock();
      }
      if (A2 == 0) {
        ct2 = 0;
      }
      lastA2 = A2;
    }
    
    if (lastA3 != A3 || (ct3 != 0 && osal_GetSystemClock() > (ct3+3000))) {  // ???????
      if (D0 & 0x08) {
        sprintf(p, "%u", A3);
        ZXBeeAdd("alert", p);
        ct3 = osal_GetSystemClock();
      }
      if (A3 == 0) {
        ct3 = 0;
      }
      lastA3 = A3;
    }
  }
  if (lastA4 != A4 || (ct4 != 0 && osal_GetSystemClock() > (ct4+3000))) {  // ???????
    if (D0 & 0x10) {
      sprintf(p, "%u", A4);
      ZXBeeAdd("A4", p);
      ct4 = osal_GetSystemClock();
    }
    if (A4 == 0) {
      ct4 = 0;
    }
    lastA4 = A4;
  }
  if (lastA5 != A5 || (ct5 != 0 && osal_GetSystemClock() > (ct5+3000))) {  // ???????
    if (D0 & 0x01) {
      sprintf(p, "%u", A5);
      ZXBeeAdd("A5", p);
      ct5 = osal_GetSystemClock();
    }
    if (A5 == 0) {
      ct5 = 0;
    }
    lastA5 = A5;
  }

  p = ZXBeeEnd();
  if (p != NULL) {
    int len = strlen(p); 
    ZXBeeInfSend(p, len);
  }
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
  // ??cmd???????????
  if(cmd & 0x40){ 
    RELAY1 = ON;                                                 // ?????1
  }
  else{
    RELAY1 = OFF;                                                 // ?????1
  }
  if(cmd & 0x80){  
    RELAY2 = ON;                                                 // ?????2
  }
  else{
    RELAY2 = OFF;                                                 // ?????2        
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
int ZXBeeUserProcess(char *ptag, char *pval)
{ 
  int val;
  int ret = 0;	
  char pData[16];
  char *p = pData;
  
  // ??????pval???????????
  val = atoi(pval);	
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
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("OD1", ptag)){                                // ?D1???????OD1???????
    D1 |= val;
    sensorControl(D1);                                          // ??????
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("D1", ptag)){                                 // ?????????
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D1);
      ZXBeeAdd("D1", p);
    } 
  }
  if (0 == strcmp("A0", ptag) || 0 == strcmp("pir", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA0();
      ret = sprintf(p, "%u", A0);   
      ZXBeeAdd("pir", p);
    } 
  }
  if (0 == strcmp("A1", ptag) || 0 == strcmp("tch", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA1();
      ret = sprintf(p, "%u", A1);  
      ZXBeeAdd("tch", p);
    } 
  }
  if (0 == strcmp("A2", ptag) || 0 == strcmp("door", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA2();
      ret = sprintf(p, "%u", A2); 
      ZXBeeAdd("door", p);
    } 
  }
  if (0 == strcmp("A3", ptag) || 0 == strcmp("alert", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA3();
      ret = sprintf(p, "%u", A3); 
      ZXBeeAdd("alert", p);
    } 
  }
  if (0 == strcmp("A4", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA4();
      ret = sprintf(p, "%u", A4);
      ZXBeeAdd("A4", p);
    } 
  }  
  if (0 == strcmp("A5", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA5();
      ret = sprintf(p, "%u", A5);   
      ZXBeeAdd("A5", p);
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
  if (0 == strcmp("V1", ptag)){
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%s", V1);                
      ZXBeeAdd("V1", p);
    }else{
      if(mode == 2){
        int n = strlen(pval)/2;
        syn6288_play_unicode(hex2unicode(pval),n);
      }
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
  //???????????????????
  static char check_start_flag = 0;
  if(check_start_flag == 0) {
    if (event & MY_REPORT_EVT) { 
      // ????????????????MY_CHECK_EVT
      osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);
      check_start_flag = 1;
    }
  }
  
  if (event & MY_REPORT_EVT) { 
    sensorUpdate();
    //???????????MY_REPORT_EVT 
    osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, V0*1000);
  }  
  if (event & MY_CHECK_EVT) { 
    sensorCheck(); 
    // ???????????MY_CHECK_EVT 
    osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);
  } 
}

