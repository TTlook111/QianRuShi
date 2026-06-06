/*********************************************************************************************
* ???sensor.c
* ???Xuzhy 2018.5.16
* ???xLab Sensor-A?????
* ???
* ???
*********************************************************************************************/

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
#include "htu21d.h"
#include "fbm320.h"
#include "BH1750.h"
#include "iic.h"
#include "lis3dh.h"
#include "MP-503.h"
#include "stadiometry.h"
#include "relay.h"
#include "ld3320.h"
#include "math.h"
#include "zxbee.h"
#include "zxbee-inf.h"
/*********************************************************************************************
* ???
*********************************************************************************************/

/*********************************************************************************************
* ????
*********************************************************************************************/
static uint8 D0 = 0xFF;                                         // ??????????
static uint8 D1 = 0;                                            // ??????????
static float A0 = 0.0;                                          // A0?????
static float A1 = 0.0;                                          // A1?????
static float A2 = 0.0;                                          // A2?????
static int A3 = 0;                                              // A3??????
static float A4 = 0.0;                                          // A4?????
static int A5 = 0;                                              // A5??????
static float A6 = 0.0;                                          // A6?????
static int A7 = 0;                                              // A7???????
static uint16 V0 = 30;                                          // V0?????????????30s
static uint8 lis3dh_status = 0;                                 // ???????
/*********************************************************************************************
* ???fallDect()
* ???????????
* ???x?y?z - ?????????x?y?z?
* ???int - ????
* ???
* ???
*********************************************************************************************/
int fallDect(float x, float y, float z)
{
  float x2 = x * x;
  float y2 = y * y;
  float z2 = z * z;
  float a;
  
  a = sqrt(x2 + y2 + z2);
  if (a<5) {
    return 1;
  }    
  return 0;
}
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
* ???updateA0()
* ?????A0??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA0(void)
{
  // ?????????A0
  A0 = (htu21d_get_data(TEMP)/100.0f); 
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
  // ?????????A1
  A1 = (htu21d_get_data(HUMI)/100.0f);
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
  // ?????????A2
  A2 = bh1750_get_data();
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
  // ???????????????A3
  A3 = get_airgas_data(); 
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
  // ???????????A4
  float temperature = 0;                                        // ????????
  long pressure = 0;                                            // ????????

  fbm320_data_get(&temperature,&pressure);                      // ?????????
  A4 = pressure/100.0f;                                         // ?????????

}
/*********************************************************************************************
* ??? updateA5()
* ????????????
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA5(void)
{
  static uint32 ct = 0;
  float accx,accy,accz;
  
  if (A5 == 0) {
    lis3dh_read_data(&accx,&accy,&accz);
    if(!(accx==0&&accy==0&&accz==0)) {
      A5 = fallDect(accx,accy,accz);
      ct = osal_GetSystemClock();
    } else {
      A5 = 0;
    }
  } else if (A5!=0 && osal_GetSystemClock()>(ct+2000)) {
    A5 = 0;
  }
}
/*********************************************************************************************
* ???updateA6()
* ?????A6??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA6(void)
{
  // ???????????A6
  A6 = get_stadiometry_data();
}
/*********************************************************************************************
* ??? updateA7()
* ??????????????A7??
* ???
* ???
* ???
* ???
*********************************************************************************************/
void updateA7(void)
{
  A7 = ld3320_read();                                       // ??????????????
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
  ZXBeeInfSetLocalAddr(ZXBEE_ADDR_SENSOR_A);
  // ????????
  htu21d_init();                                                // ?????????
  bh1750_init();                                                // ?????????
  airgas_init();                                                // ??????????
  fbm320_init();                                                // ??????????
  lis3dh_status = lis3dh_init();                                // ????????
  stadiometry_init();                                           // ??????????
  ld3320_init();                                                // ??????????
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
    sprintf(p, "%.1f", A0); 
    ZXBeeAdd("temp", p);
  }
  if ((D0 & 0x02) == 0x02){                                     // ?????????pData???????????
    updateA1();
    sprintf(p, "%.1f", A1); 
    ZXBeeAdd("humi", p);
  }   
  if ((D0 & 0x04) == 0x04){                                     // ?????????pData???????????
    updateA2();
    sprintf(p, "%.1f", A2);  
    ZXBeeAdd("lux", p);
  }
  if ((D0 & 0x08) == 0x08){                                     // ???????????pData?????????????
    updateA3();
    sprintf(p, "%u", A3);  
    ZXBeeAdd("A3", p);
  }
  if ((D0 & 0x10) == 0x10){                                     // ???????????pData?????????????
    updateA4();
    sprintf(p, "%.1f", A4);
    ZXBeeAdd("A4", p);
  }
  if ((D0 & 0x10) == 0x10){                                     // ???????????pData?????????????
    updateA5();
    sprintf(p, "%u", A5);
    ZXBeeAdd("A5", p);
  }
  if ((D0 & 0x40) == 0x40){                                     // ???????????pData?????????????
    updateA6();
    sprintf(p, "%.1f", A6); 
    ZXBeeAdd("A6", p);
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
  char pData[16];
  char *p = pData;
  static int lastA5 = 0;
 
  if(lis3dh_status == 0) updateA5();                            // ????
  updateA7();                                                   // ??????
  
  ZXBeeBegin();
  
  if (A5 != lastA5) {                                           // A5????2???????
    sprintf(p,"%u",A5);
    ZXBeeAdd("A5", p);
    lastA5 = A5;
  }

  if (A7 != 0){                                                 // ???????A7
    sprintf(p,"%u",A7);
    ZXBeeAdd("A7", p);
  }

  p = ZXBeeEnd();
  if (p != NULL) {
    ZXBeeInfSend(p, strlen(p));
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
* ???<0:??????>=0 ?????
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
  if (0 == strcmp("A0", ptag) || 0 == strcmp("temp", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA0();
      ret = sprintf(p, "%.1f", A0);     
      ZXBeeAdd("temp", p);
    } 
  }
  if (0 == strcmp("A1", ptag) || 0 == strcmp("humi", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA1();
      ret = sprintf(p, "%.1f", A1);  
      ZXBeeAdd("humi", p);
    } 
  }  
  if (0 == strcmp("A2", ptag) || 0 == strcmp("lux", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA2();
      ret = sprintf(p, "%.1f", A2);  
      ZXBeeAdd("lux", p);
    } 
  }
  if (0 == strcmp("A3", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA3();
      ret = sprintf(p, "%u", A3);    
      ZXBeeAdd("A3", p);
    } 
  }
  if (0 == strcmp("A4", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA4();
      ret = sprintf(p, "%.1f", A4); 
      ZXBeeAdd("A4", p);
    } 
  }  
  if (0 == strcmp("A6", ptag)){ 
    if (0 == strcmp("?", pval)){
      updateA6();
      ret = sprintf(p, "%.1f", A6);   
      ZXBeeAdd("A6", p);
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

