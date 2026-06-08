/*********************************************************************************************
* �ļ���sensor.c
* ���ߣ�Xuzhy 2018.5.16
* ˵����xLab Sensor-C����������
* �޸ģ�fuyou 2018.08.03 ���Ӷ�ӦӲ���汾������
* ע�ͣ�
*********************************************************************************************/
/*
 * mode��������ѡ���ʶ���������ߵĲ��?mode
 *  1: ѡ���������?��������&����&���洫����
 *  2��ѡ�����������������ϳɴ�����
 *
 */
/*********************************************************************************************
* ͷ�ļ�
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
#include "gpio_int.h"
#include "security_logic.h"
/*********************************************************************************************
* �궨��
*********************************************************************************************/

/* GPIO�????模�??�????: 1=使�?�中???模�??, 0=�?询模�? */
#define USE_GPIO_INTERRUPT      1

/* �???�中???�?件�??�? (??��?? OSAL �?件�?????) */
#define SECURITY_EVT_PIR        0x0100  /* PIR 人�??红�??�???? */
#define SECURITY_EVT_VIBRATION  0x0200  /* ?????�触??? */
#define SECURITY_EVT_HALL       0x0400  /* ??��????��???????? */
#define SECURITY_EVT_TOUCH      0x0800  /* �???��?��??�???? */

/*********************************************************************************************
* ȫ�ֱ���
*********************************************************************************************/
static uint8  D0 = 0xFF;                                        // Ĭ�ϴ������ϱ�����
static uint8  D1 = 0;                                           // �̵�����ʼ״̬Ϊȫ��
static uint8  A0 = 0;                                           // mode=1 Ϊ���������?mode=2 Ϊ�������?
static uint8  A1 = 0;                                           // mode=1 Ϊ�𶯼��?mode=2 û�ж�Ӧ������
static uint8  A2 = 0;                                           // mode=1 Ϊ�������?mode=2 û�ж�Ӧ������
static uint8  A3 = 0;                                           // mode=1 Ϊ�����⣬mode=2 û�ж�Ӧ������
static uint8  A4 = 0;                                           // ��ȼ������
static uint8  A5 = 0;                                           // �����դ���
static uint16 V0 = 30;                                          // V0����Ϊ�ϱ�ʱ������Ĭ��Ϊ30s
static char*  V1;                                               // ����ת�١�����������Ƶ��
static uint8 mode = 1;

/* GPIO �????模�?? ??? �???��??件�??�? (ISR???�?�?设置, MyEventProcess�?�????) */
#if USE_GPIO_INTERRUPT
static volatile uint16 g_security_events = 0;  /* �?�????�???��??件�??�? */

/*********************************************************************************************
* ???称�??_pir_security_callback()
* ?????��??PIR 人�??红�??�??????�中??????�? ??? 设置�???��??件�??�?
* ?????��??state  ??? �??????�平
*       pdata  ??? ??��?��?��??(???使�??)
* �????�????
* �???��??[??��??]
* 注�??�???? ISR �?�????�?�???? ??? �?设置???�?�?�???��???????��??�?
*********************************************************************************************/
static void _pir_security_callback(uint8 state, void *pdata)
{
    (void)pdata;
    if (state == 1) {  /* �???�平 = ???人�??�? */
        g_security_events |= SECURITY_EVT_PIR;
    }
}

/*********************************************************************************************
* ???称�??_vibration_security_callback()
* ?????��???????��???????�中??????�? ??? 设置�???��??件�??�?
* ?????��??state  ??? �??????�平
*       pdata  ??? ??��?��?��??(???使�??)
* �????�????
* �???��??[??��??]
*********************************************************************************************/
static void _vibration_security_callback(uint8 state, void *pdata)
{
    (void)pdata;
    if (state == 0) {  /* �???�平 = �?�???��????? */
        g_security_events |= SECURITY_EVT_VIBRATION;
    }
}

/*********************************************************************************************
* ???称�??_hall_security_callback()
* ?????��????��??�??????�中??????�? ??? 设置�???��??件�??�?
* ?????��??state  ??? �??????�平
*       pdata  ??? ??��?��?��??(???使�??)
* �????�????
* �???��??[??��??]
*********************************************************************************************/
static void _hall_security_callback(uint8 state, void *pdata)
{
    (void)pdata;
    if (state == DOOR_STATE_OPEN) {  /* ??��?????�? */
        g_security_events |= SECURITY_EVT_HALL;
    }
}

/*********************************************************************************************
* ???称�??_touch_security_callback()
* ?????��??�???��???????�中??????�? ??? 设置??��??�?件�??�?
* ?????��??state  ??? �??????�平
*       pdata  ??? ??��?��?��??(???使�??)
* �????�????
* �???��??[??��??]
*********************************************************************************************/
static void _touch_security_callback(uint8 state, void *pdata)
{
    (void)pdata;
    if (state == 0) {  /* �???�平 = �????/?????��?? */
        g_security_events |= SECURITY_EVT_TOUCH;
    }
}

/*********************************************************************************************
* ???称�??_sensor_int_init()
* ?????��??�?�????�??????????�???��???????��?? GPIO �????模�??
* ?????��?????
* �????�????
* �???��??[??��??]
* 注�??�?�???? USE_GPIO_INTERRUPT=1 ??��??�?
*********************************************************************************************/
static void _sensor_int_init(void)
{
    /* 1. ??��?? GPIO �???????�???? (P0 �????) */
    gpio_int_init_all();

    /* 2. Mode=1: ???�???? PIR + ?????? + ??��?? �???? */
    if (mode == 1) {
        infrared_int_init(_pir_security_callback, NULL);
        vibration_int_init(_vibration_security_callback, NULL);
        hall_int_init(_hall_security_callback, NULL);
    }

    /* 3. Mode=2: ???�????�???��????? + �???��?????�???? */
    if (mode == 2) {
        touch_int_init(_touch_security_callback, NULL);
    }
}
#endif /* USE_GPIO_INTERRUPT */
/*********************************************************************************************
* ���ƣ�updateV0()
* ���ܣ�����V0��ֵ
* ������*val -- �����µı���
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void updateV0(char *val)
{
  //���ַ�������val����ת��Ϊ���ͱ�����ֵ
  V0 = atoi(val);                                               // ��ȡ�����ϱ�ʱ������?
}
/*********************************************************************************************
* ���ƣ�updateA0()
* ���ܣ�����A0��ֵ
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void updateA0(void)
{
   static uint32 ct = 0;
   
  if (mode == 1) {                                               // �����������?������ֵ
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
* ���ƣ�updateA1()
* ���ܣ�����A1��ֵ
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void updateA1(void)
{
   static uint32 ct = 0;
  
  if (mode == 1) {                                              // �����񶯴�������ֵ
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
* ���ƣ�updateA2()
* ���ܣ�����A2��ֵ
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void updateA2(void)
{
  if (mode == 1) {
    A2 = get_hall_status();                                     // ���»�����������ֵ
  }
}
/*********************************************************************************************
* ���ƣ�updateA3()
* ���ܣ�����A3��ֵ
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void updateA3(void)
{
  static uint32 ct = 0;
  
  if (mode == 1) {
    A3 = get_flame_status();                                    // ���»��洫������ֵ
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
* ���ƣ�updateA4()
* ���ܣ�����A4��ֵ
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void updateA4(void)
{
  A4 = get_combustiblegas_data();                               // ����ȼ����������ֵ
}
/*********************************************************************************************
* ���ƣ�updateA5()
* ���ܣ�����A5��ֵ
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void updateA5(void)                                             // ���¹�դ��������ֵ
{
  A5 = get_grating_status();
}
/*********************************************************************************************
* ���ƣ�sensorInit()
* ���ܣ�������Ӳ����ʼ��
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensorInit(void)
{
  /* ???�??????????�??????? (�?询模�???��?????�?) */
  vibration_init();                                             /* ?????��???????��??�???? */
  infrared_init();                                              /* 人�??红�??�??????��??�???? */
  flame_init();                                                 /* ?????��???????��??�???? */
  hall_init();                                                  /* ???�?/??��??�??????��??�???? */
  combustiblegas_init();                                        /* ??????�?�??????��??�???? */
  grating_init();                                               /* ??????�??????��??�???? */
  relay_init();                                                 /* 继�?��?��??�???? */

  /*
   * GPIO �????模�?????�???? (??�代�?询�??�???��??件�??�?)
   * ???�? USE_GPIO_INTERRUPT �???��?��??�? (�???????件顶??��??�?�?)
   */
#if USE_GPIO_INTERRUPT
  _sensor_int_init();                                           /* ???�???? PIR/??��??/?????? �???? */
#endif

  /* ?????��????��??�???��??�??????��?��??�???��????��??�? MY_REPORT_EVT */
  /* 初始化安防逻辑模块 (告警分级/徘徊检测/夜间判断) */
  security_logic_init();
  osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, (uint16)((osal_rand()%10) * 1000));
}
/*********************************************************************************************
* ���ƣ�sensorLinkOn()
* ���ܣ��������ڵ������ɹ����ú���
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensorLinkOn(void)
{
  sensorUpdate();
}
/*********************************************************************************************
* ���ƣ�sensorUpdate()
* ���ܣ����������ϱ�������
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensorUpdate(void)
{
  char pData[32];
  char *p = pData;
  const security_state_t *sec;

  /* ���´��������� */
  updateA0();
  updateA1();
  updateA2();
  updateA3();
  updateA4();
  updateA5();

  /* ���ð����߼�: �澯�ּ� + �ǻ���� + ҹ���ж� */
  security_logic_update(A0, A1, A2, osal_GetSystemClock());
  sec = security_logic_get_state();

  ZXBeeBegin();

  /* ===== Mode=1: ��������ֶ� (ʹ����Ŀͳһ�ֶ���) ===== */
  if (mode == 1) {
    /* ������� -> pir (ԭ A0) */
    if (D0 & 0x01) {
      sprintf(p, "%u", A0);
      ZXBeeAdd("pir", p);
    }
    /* ����/�� -> tch (ԭ A1) */
    if (D0 & 0x02) {
      sprintf(p, "%u", A1);
      ZXBeeAdd("tch", p);
    }
    /* �Ŵ� -> door (ԭ A2) */
    if (D0 & 0x04) {
      sprintf(p, "%u", A2);
      ZXBeeAdd("door", p);
    }
    /* ���� -> flame */
    if (D0 & 0x08) {
      sprintf(p, "%u", A3);
      ZXBeeAdd("flame", p);
    }
  }

  /* ===== Mode=2: ����+���� ===== */
  if (mode == 2) {
    if (D0 & 0x01) {
      sprintf(p, "%u", A0);
      ZXBeeAdd("touch", p);
    }
  }

  /* ��ȼ�� -> gas */
  if (D0 & 0x10) {
    sprintf(p, "%u", A4);
    ZXBeeAdd("gas", p);
  }
  /* ��դ -> grating */
  if (D0 & 0x20) {
    sprintf(p, "%u", A5);
    ZXBeeAdd("grating", p);
  }

  /* ===== �����߼��ֶ� (�� security_logic ����) ===== */
  sprintf(p, "%u", sec->alert);
  ZXBeeAdd("alert", p);

  sprintf(p, "%u", sec->stay);
  ZXBeeAdd("stay", p);

  sprintf(p, "%u", sec->night);
  ZXBeeAdd("night", p);

  /* ����״̬ */
  sprintf(p, "%u", D1);
  ZXBeeAdd("D1", p);

  /* ȷ���¼��ѱ���ȡ */
  security_logic_ack_event();

  p = ZXBeeEnd();
  if (p != NULL) {
    ZXBeeInfSend(p, strlen(p));
  }
}
/*********************************************************************************************
* ���ƣ�sensorCheck()
* ���ܣ����������?
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensorCheck(void)
{
  static char lastA0 = 0,lastA1=0,lastA2=0,lastA3=0,lastA4=0,lastA5=0;
  static uint32 ct0=0, ct1=0, ct2=0, ct3=0, ct4=0, ct5=0;
  char pData[32];
  char *p = pData;
  const security_state_t *sec;
  int has_change = 0;

  /* ���´��������� */
  updateA0();
  updateA1();
  updateA2();
  updateA3();
  updateA4();
  updateA5();

  /* ���ð����߼�: �澯�ּ� + �ǻ���� + ҹ���ж� */
  security_logic_update(A0, A1, A2, osal_GetSystemClock());
  sec = security_logic_get_state();

  /* �ж��Ƿ��б仯��Ҫ�ϱ� */
  if (lastA0 != A0 || lastA1 != A1 || lastA2 != A2 ||
      lastA3 != A3 || lastA4 != A4 || lastA5 != A5 ||
      sec->event_changed) {
    has_change = 1;
  }

  /* ǿ�� 3s �ر� (����״̬����) */
  if (mode == 1 && ct0 != 0 && osal_GetSystemClock() > (ct0 + 3000)) has_change = 1;
  if (ct1 != 0 && osal_GetSystemClock() > (ct1 + 3000)) has_change = 1;

  if (!has_change) return;

  ZXBeeBegin();

  /* ===== Mode=1: ��������ֶ� (��Ŀͳһ�ֶ���) ===== */
  if (mode == 1) {
    if (lastA0 != A0 || (ct0 != 0 && osal_GetSystemClock() > (ct0 + 3000))) {
      if (D0 & 0x01) {
        sprintf(p, "%u", A0);
        ZXBeeAdd("pir", p);
        ct0 = osal_GetSystemClock();
      }
      if (A0 == 0) ct0 = 0;
      lastA0 = A0;
    }

    if (lastA1 != A1 || (ct1 != 0 && osal_GetSystemClock() > (ct1 + 3000))) {
      if (D0 & 0x02) {
        sprintf(p, "%u", A1);
        ZXBeeAdd("tch", p);
        ct1 = osal_GetSystemClock();
      }
      if (A1 == 0) ct1 = 0;
      lastA1 = A1;
    }

    if (lastA2 != A2 || (ct2 != 0 && osal_GetSystemClock() > (ct2 + 3000))) {
      if (D0 & 0x04) {
        sprintf(p, "%u", A2);
        ZXBeeAdd("door", p);
        ct2 = osal_GetSystemClock();
      }
      if (A2 == 0) ct2 = 0;
      lastA2 = A2;
    }

    if (lastA3 != A3 || (ct3 != 0 && osal_GetSystemClock() > (ct3 + 3000))) {
      if (D0 & 0x08) {
        sprintf(p, "%u", A3);
        ZXBeeAdd("flame", p);
        ct3 = osal_GetSystemClock();
      }
      if (A3 == 0) ct3 = 0;
      lastA3 = A3;
    }
  }

  /* ��ȼ�� */
  if (lastA4 != A4 || (ct4 != 0 && osal_GetSystemClock() > (ct4 + 3000))) {
    if (D0 & 0x10) {
      sprintf(p, "%u", A4);
      ZXBeeAdd("gas", p);
      ct4 = osal_GetSystemClock();
    }
    if (A4 == 0) ct4 = 0;
    lastA4 = A4;
  }

  /* ��դ */
  if (lastA5 != A5 || (ct5 != 0 && osal_GetSystemClock() > (ct5 + 3000))) {
    if (D0 & 0x20) {
      sprintf(p, "%u", A5);
      ZXBeeAdd("grating", p);
      ct5 = osal_GetSystemClock();
    }
    if (A5 == 0) ct5 = 0;
    lastA5 = A5;
  }

  /* ===== �����߼��ֶ� (�¼������ϱ�) ===== */
  if (sec->event_changed || sec->alert > ALERT_NONE) {
    sprintf(p, "%u", sec->alert);
    ZXBeeAdd("alert", p);

    sprintf(p, "%u", sec->stay);
    ZXBeeAdd("stay", p);

    sprintf(p, "%u", sec->night);
    ZXBeeAdd("night", p);
  }

  security_logic_ack_event();

  p = ZXBeeEnd();
  if (p != NULL) {
    int len = strlen(p);
    ZXBeeInfSend(p, len);
  }
}
/*********************************************************************************************
* ���ƣ�sensorControl()
* ���ܣ�����������
* ������cmd - ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void sensorControl(uint8 cmd)
{
  // ����cmd����������Ӧ�Ŀ��Ƴ���
  if(cmd & 0x40){ 
    RELAY1 = ON;                                                 // �����̵���1
  }
  else{
    RELAY1 = OFF;                                                 // �رռ̵���1
  }
  if(cmd & 0x80){  
    RELAY2 = ON;                                                 // �����̵���2
  }
  else{
    RELAY2 = OFF;                                                 // �رռ̵���2        
  }
}
/*********************************************************************************************
* ���ƣ�ZXBeeUserProcess()
* ���ܣ������յ��Ŀ�������
* ������*ptag -- ������������
*       *pval -- �����������?
* ���أ�ret -- p�ַ�������
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
int ZXBeeUserProcess(char *ptag, char *pval)
{ 
  int val;
  int ret = 0;	
  char pData[16];
  char *p = pData;
  
  // ���ַ�������pval����ת��Ϊ���ͱ�����ֵ
  val = atoi(pval);	
  // �����������?
  if (0 == strcmp("CD0", ptag)){                                // ��D0��λ���в�����CD0��ʾλ�������?
    D0 &= ~val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("OD0", ptag)){                                // ��D0��λ���в�����OD0��ʾλ��һ����
    D0 |= val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("D0", ptag)){                                 // ��ѯ�ϱ�ʹ�ܱ���
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D0);
      ZXBeeAdd("D0", p);
    } 
  }
  /* ===== ��Ŀ�ֶ�: �澯��� ===== */
  if (0 == strcmp("reset", ptag)){
    if (val == 1) {
      security_logic_reset_alert();
      sensorControl(0);        /* �رռ̵��� */
    }
    ret = sprintf(p, "reset");
    ZXBeeAdd("reset", p);
  }
  if (0 == strcmp("CD1", ptag)){                                // ��D1��λ���в�����CD1��ʾλ�������?
    D1 &= ~val;
    sensorControl(D1);                                          // ����ִ������
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("OD1", ptag)){                                // ��D1��λ���в�����OD1��ʾλ��һ����
    D1 |= val;
    sensorControl(D1);                                          // ����ִ������
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("D1", ptag)){                                 // ��ѯִ�����������?
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D1);
      ZXBeeAdd("D1", p);
    } 
  }
  /* 兼容旧字段名 A0 和新字段名 pir */
  if (0 == strcmp("A0", ptag) || 0 == strcmp("pir", ptag)){
    if (0 == strcmp("?", pval)){
      updateA0();
      ret = sprintf(p, "%u", A0);
      ZXBeeAdd("pir", p);
    }
  }
  /* 兼容旧字段名 A1 和新字段名 tch */
  if (0 == strcmp("A1", ptag) || 0 == strcmp("tch", ptag)){
    if (0 == strcmp("?", pval)){
      updateA1();
      ret = sprintf(p, "%u", A1);
      ZXBeeAdd("tch", p);
    }
  }
  /* 兼容旧字段名 A2 和新字段名 door */
  if (0 == strcmp("A2", ptag) || 0 == strcmp("door", ptag)){
    if (0 == strcmp("?", pval)){
      updateA2();
      ret = sprintf(p, "%u", A2);
      ZXBeeAdd("door", p);
    }
  }
  /* 兼容旧字段名 A3 和新字段名 alert */
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
      ret = sprintf(p, "%u", V0);                         	// �ϱ�ʱ����
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
* ���ƣ�MyEventProcess()
* ���ܣ��Զ����¼�����
* ������event -- �¼����?
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void MyEventProcess( uint16 event )
{
  /* �?次�????��??�?�???��??延�??�????100ms巡�??�???��?? */
  static char check_start_flag = 0;

  /*
   * GPIO �????模�??: �????�???��???????�中???�?�?
   * ISR ???�?�?设置??? g_security_events ???�???��?��??�?�?�????
   */
#if USE_GPIO_INTERRUPT
  if (g_security_events) {
    uint16 events = g_security_events;
    g_security_events = 0;                    /* ???�?�???? ??? ??��??丢失??��??�? */

    if (events & SECURITY_EVT_PIR)       { /* PIR 人�??红�??�???? ??? �???��????? */   sensorUpdate(); }
    if (events & SECURITY_EVT_VIBRATION) { /* ?????�触??? ??? �???��????? */           sensorUpdate(); }
    if (events & SECURITY_EVT_HALL)      { /* ??��????��???????? ??? �???��????? */       sensorUpdate(); }
    if (events & SECURITY_EVT_TOUCH)     { /* �???��?��??�???? ??? �???��????? */       sensorUpdate(); }
  }
#endif

  if(check_start_flag == 0) {
    if (event & MY_REPORT_EVT) { 
      /* ?????��????��??�???��??�??????��?????巡�??�?�? MY_CHECK_EVT */
      osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);
      check_start_flag = 1;
    }
  }
  
  if (event & MY_REPORT_EVT) { 
    sensorUpdate();
    /* ??????�???��??�???��??�?�???��?????�???��??�? MY_REPORT_EVT */
    osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, V0*1000);
  }  
  if (event & MY_CHECK_EVT) { 
    sensorCheck(); 
    /* ??????�???��??�???��??�??????��?????巡�??�?�? MY_CHECK_EVT */
    osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);
  } 
}
