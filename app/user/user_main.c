/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"

#include "smartconfig.h"

#include "gpio.h"  //端口控制需要的头文件

#include "espconn.h"

#include "mem.h"

#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif

void user_rf_pre_init(void)
{
}


void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            os_printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            os_printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
			sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            os_printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;

	        wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            os_printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
				//SC_TYPE_ESPTOUCH
                uint8 phone_ip[4] = {0};

                os_memcpy(phone_ip, (uint8*)pdata, 4);
                os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            } else {
            	//SC_TYPE_AIRKISS - support airkiss v2.0
				//airkiss_start_discover();
            }
            smartconfig_stop();
            break;
    }

}


void ICACHE_FLASH_ATTR
STA_init(void)
{
    struct station_config inf;

    wifi_set_opmode(STATIONAP_MODE);

    //wifi_station_disconnect();

    //开始读取上一次的配置 连接路由器
    wifi_station_get_config(&inf);

    //os_sprintf(inf.ssid,"hysiry");
    // os_sprintf(inf.password,"jiayouba");


    // wifi_station_set_config(&inf);

    if(os_strlen(inf.ssid))
    {

        os_printf("***************auto connect router******\n");
        os_printf("connect to router ssid=%s\n",inf.ssid);
        os_printf("connect to router pwd=%s\n",inf.password);


        wifi_station_connect();//重连成功后就不按新配置继续连接
        wifi_station_set_auto_connect(true);////打开自动连接功能

        os_printf("********os_strlen(inf.ssid)*************\n");


    }
    else//模块第一次执行默认连接
    {
		//smartconfig_start(smartconfig_done1);
		os_sprintf(inf.ssid,"@PHICOMM_78");
		os_sprintf(inf.password,"12345678");
		inf.bssid_set=0;
		wifi_station_set_config(&inf);
    }
}





os_timer_t checkTimer_wifistate;
bool isConnected = false;


#if 0
void Check_WifiState(void) {

	uint8 status = wifi_station_get_connect_status();

	//os_printf("zmytest");      //测试定时器

	if (status == STATION_GOT_IP) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(15), 0);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(12), 1);
		wifi_set_opmode(0x01);
	} else {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(12), 0); //GPIO15 低电平输出
		GPIO_OUTPUT_SET(GPIO_ID_PIN(15), 1); //GPIO15 低电平输出
	}
}
#endif

void WIFI_Init() {
	struct softap_config apConfig;
	wifi_set_opmode(0x03);    //设置为AP模式，保存到 flash
	apConfig.ssid_len = 10;						//设置ssid长度
	os_strcpy(apConfig.ssid, "SOMPUTON");	    //设置ssid名字
	os_strcpy(apConfig.password, "12345678");	//设置密码
	apConfig.authmode = 3;                      //设置加密模式
	apConfig.beacon_interval = 100;            //信标间隔时槽100 ~ 60000 ms
	apConfig.channel = 1;                      //通道号1 ~ 13
	apConfig.max_connection = 4;               //最大连接数
	apConfig.ssid_hidden = 0;                  //隐藏SSID

	wifi_softap_set_config(&apConfig);		//设置 WiFi soft-AP 接口配置，并保存到 flash
}

struct espconn user_tcp_espconn;


void ICACHE_FLASH_ATTR server_recv(void *arg, char *pdata, unsigned short len) {

	os_printf("zmytcp");

}


void ICACHE_FLASH_ATTR server_sent(void *arg) {
	os_printf("send data succeed！\r");
	if (isConnected) {
		wifi_set_opmode(0x01); //设置为STATION模式
	}
}

void ICACHE_FLASH_ATTR server_discon(void *arg) {
	os_printf("conect diable! \r");
}

void ICACHE_FLASH_ATTR server_listen(void *arg) //注册 TCP 连接成功建立后的回调函数
{
	struct espconn *pespconn = arg;
	espconn_regist_recvcb(pespconn, server_recv); //接收
	espconn_regist_sentcb(pespconn, server_sent); //发送
	espconn_regist_disconcb(pespconn, server_discon); //断开
}

void ICACHE_FLASH_ATTR server_recon(void *arg, sint8 err) //注册 TCP 连接发生异常断开时的回调函数，可以在回调函数中进行重连
{
	os_printf("连接错误，错误代码为：%d\r\n", err); //%d,用来输出十进制整数
}




void Inter213_InitTCP(uint32_t Local_port) {
	user_tcp_espconn.proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp)); //分配空间
	user_tcp_espconn.type = ESPCONN_TCP; //设置类型为TCP协议
	user_tcp_espconn.proto.tcp->local_port = Local_port; //本地端口

	//注册连接成功回调函数和重新连接回调函数
	espconn_regist_connectcb(&user_tcp_espconn, server_listen); //注册 TCP 连接成功建立后的回调函数
	espconn_regist_reconcb(&user_tcp_espconn, server_recon); //注册 TCP 连接发生异常断开时的回调函数，可以在回调函数中进行重连
	espconn_accept(&user_tcp_espconn); //创建 TCP server，建立侦听
	espconn_regist_time(&user_tcp_espconn, 180, 0); //设置超时断开时间 单位：秒，最大值：7200 秒
	//如果超时时间设置为 0，ESP8266 TCP server 将始终不会断开已经不与它通信的 TCP client，不建议这样使用。
}





/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{



	tcp_client_init();

	//uart_init(57600,57600);
#if 0
    os_printf("SDK version:%s\n", system_get_sdk_version());


	wifi_station_set_auto_connect(1);
	wifi_set_sleep_type(NONE_SLEEP_T);                     //set none sleep mode

	STA_init();


	os_timer_disarm(&checkTimer_wifistate); //取消定时器定时
	os_timer_setfn(&checkTimer_wifistate, (os_timer_func_t *) Check_WifiState,
	NULL); //设置定时器回调函数
	os_timer_arm(&checkTimer_wifistate, 1000, true); //启动定时器，单位：毫秒


	WIFI_Init();


	Inter213_InitTCP(22);

#endif



#if 0
    wifi_set_opmode(STATION_MODE); //config mode  ap and sta

    smartconfig_set_type(SC_TYPE_ESPTOUCH); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS

	smartconfig_start(smartconfig_done);
#endif




#if ESP_PLATFORM
    /*Initialization of the peripheral drivers*/
    /*For light demo , it is user_light_init();*/
    /* Also check whether assigned ip addr by the router.If so, connect to ESP-server  */
    //user_esp_platform_init();
#endif
    /*Establish a udp socket to receive local device detect info.*/
    /*Listen to the port 1025, as well as udp broadcast.
    /*If receive a string of device_find_request, it rely its IP address and MAC.*/
    //user_devicefind_init();

    /*Establish a TCP server for http(with JSON) POST or GET command to communicate with the device.*/
    /*You can find the command in "2B-SDK-Espressif IoT Demo.pdf" to see the details.*/
    /*the JSON command for curl is like:*/
    /*3 Channel mode: curl -X POST -H "Content-Type:application/json" -d "{\"period\":1000,\"rgb\":{\"red\":16000,\"green\":16000,\"blue\":16000}}" http://192.168.4.1/config?command=light      */
    /*5 Channel mode: curl -X POST -H "Content-Type:application/json" -d "{\"period\":1000,\"rgb\":{\"red\":16000,\"green\":16000,\"blue\":16000,\"cwhite\":3000,\"wwhite\",3000}}" http://192.168.4.1/config?command=light      */
#ifdef SERVER_SSL_ENABLE
    //user_webserver_init(SERVER_SSL_PORT);
#else
    //user_webserver_init(SERVER_PORT);
#endif
}

