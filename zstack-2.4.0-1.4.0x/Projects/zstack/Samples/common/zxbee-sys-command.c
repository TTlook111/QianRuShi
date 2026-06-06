/*********************************************************************************************
* 文件：zxbee-sys-command.c
* 作者：Xuzhy 2018.5.16
* 说明：ZXBee通信协议数据包命令解析
* 修改：fuyou 增加透传驱动
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "string.h"
#include "hal_types.h"
#include "AppCommon.h"
#include "ZComDef.h"
#include "sensor.h"
#include "zxbee.h"
#include "stdlib.h"
#include "stdio.h"
#include "at.h"
/*********************************************************************************************
* 函数原型说明
*********************************************************************************************/
void starReportTPN(uint16 t, uint16 n);
/*********************************************************************************************
* 名称：ZXBeeSysCommandProc()
* 功能：ZXBee通信协议数据包命令解析
* 参数：*ptag -- 变量；*pval -- 值
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
int ZXBeeSysCommandProc(char *ptag, char *pval)
{
    int val;
    int ret = -1;
    char buf[16];
    
    val = atoi(pval);
    if (0 == strcmp("ECHO", ptag)) {
        ZXBeeAdd("ECHO", pval);
        ret = 1;
    } else  
      if (0 == strcmp("RSSI", ptag)) {
        if (0 == strcmp("?", pval)){
          int8 getLastPkgRssi(void);
          sprintf(buf, "%d", getLastPkgRssi());
          ZXBeeAdd("RSSI", buf);
        }
      } else if (0 == strcmp("LQI", ptag)) {
        if (0 == strcmp("?", pval)){
          uint8 getLastPkgLQI(void);
          sprintf(buf, "%d", getLastPkgLQI());
          ZXBeeAdd("LQI", buf);
        }
      } else
        if (0 == strcmp("PANID", ptag)) { 
            if (0 == strcmp("?", pval)) {
                uint16 tmp16 = GetPanId();
                sprintf(buf, "%u", tmp16);
                ZXBeeAdd("PANID", buf); 
            } else {
                SetPanId(val);
            }
            ret = 1;
        } else
            if (0 == strcmp("CHANNEL", ptag)) { 
                if (0 == strcmp("?", pval)) {
                    sprintf(buf, "%u", GetChannel());
                    ZXBeeAdd("CHANNEL", buf);
                    ret = 1;
                } else {
                    SetChannel(val);
                }
                ret = 1;
            }
            else if (0 == strcmp("TYPE", ptag))
            {
                if (0 == strcmp("?", pval))
                {
#ifndef CC2530_Serial
                    sprintf(buf, "%d%d%s", NODE_CATEGORY, GetCurrentLogicalType(), NODE_NAME);
                    ZXBeeAdd("TYPE", buf);
                    ret = 1;
#else
                    //at_notify_data("AT+TYPE?\r\n",strlen("AT+TYPE?\r\n"));
#endif
                }
            }
            else if (0 == strcmp("TPN", ptag))
            {
                /*  参数格式 x/y  表示在y分钟内上报x次数据
                *  x = 0 停止上报,
                *  限制每分钟最大上报6次,最少上报1次
                */
                char *s = strchr(pval, '/');
                if (s != NULL)
                {
                    int v1, v2;
                    
                    *s = 0;
                    v1 = atoi(pval);
                    v2 = atoi(s+1);
                    
                    if (v1 > 0 && v2 > 0)
                    {
                        uint16 delay = v2*60/v1;
                        uint16 cnt = v1;
                        if (delay >= 10 && delay <= 65)
                        {
                            starReportTPN(delay, cnt);
                        }
                    }
                }
                ret = 1;
            } //TPN
    return ret;
}