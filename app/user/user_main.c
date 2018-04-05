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

#include "gpio.h"  //�˿ڿ�����Ҫ��ͷ�ļ�

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

    //��ʼ��ȡ��һ�ε����� ����·����
    wifi_station_get_config(&inf);

    //os_sprintf(inf.ssid,"hysiry");
    // os_sprintf(inf.password,"jiayouba");


    // wifi_station_set_config(&inf);

    if(os_strlen(inf.ssid))
    {

        os_printf("***************auto connect router******\n");
        os_printf("connect to router ssid=%s\n",inf.ssid);
        os_printf("connect to router pwd=%s\n",inf.password);


        wifi_station_connect();//�����ɹ���Ͳ��������ü�������
        wifi_station_set_auto_connect(true);////���Զ����ӹ���

        os_printf("********os_strlen(inf.ssid)*************\n");


    }
    else//ģ���һ��ִ��Ĭ������
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

	//os_printf("zmytest");      //���Զ�ʱ��

	if (status == STATION_GOT_IP) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(15), 0);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(12), 1);
		wifi_set_opmode(0x01);
	} else {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(12), 0); //GPIO15 �͵�ƽ���
		GPIO_OUTPUT_SET(GPIO_ID_PIN(15), 1); //GPIO15 �͵�ƽ���
	}
}
#endif

void WIFI_Init() {
	struct softap_config apConfig;
	wifi_set_opmode(0x03);    //����ΪAPģʽ�����浽 flash
	apConfig.ssid_len = 10;						//����ssid����
	os_strcpy(apConfig.ssid, "SOMPUTON");	    //����ssid����
	os_strcpy(apConfig.password, "12345678");	//��������
	apConfig.authmode = 3;                      //���ü���ģʽ
	apConfig.beacon_interval = 100;            //�ű���ʱ��100 ~ 60000 ms
	apConfig.channel = 1;                      //ͨ����1 ~ 13
	apConfig.max_connection = 4;               //���������
	apConfig.ssid_hidden = 0;                  //����SSID

	wifi_softap_set_config(&apConfig);		//���� WiFi soft-AP �ӿ����ã������浽 flash
}

struct espconn user_tcp_espconn;


void ICACHE_FLASH_ATTR server_recv(void *arg, char *pdata, unsigned short len) {

	os_printf("zmytcp");

}


void ICACHE_FLASH_ATTR server_sent(void *arg) {
	os_printf("send data succeed��\r");
	if (isConnected) {
		wifi_set_opmode(0x01); //����ΪSTATIONģʽ
	}
}

void ICACHE_FLASH_ATTR server_discon(void *arg) {
	os_printf("conect diable! \r");
}

void ICACHE_FLASH_ATTR server_listen(void *arg) //ע�� TCP ���ӳɹ�������Ļص�����
{
	struct espconn *pespconn = arg;
	espconn_regist_recvcb(pespconn, server_recv); //����
	espconn_regist_sentcb(pespconn, server_sent); //����
	espconn_regist_disconcb(pespconn, server_discon); //�Ͽ�
}

void ICACHE_FLASH_ATTR server_recon(void *arg, sint8 err) //ע�� TCP ���ӷ����쳣�Ͽ�ʱ�Ļص������������ڻص������н�������
{
	os_printf("���Ӵ��󣬴������Ϊ��%d\r\n", err); //%d,�������ʮ��������
}




void Inter213_InitTCP(uint32_t Local_port) {
	user_tcp_espconn.proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp)); //����ռ�
	user_tcp_espconn.type = ESPCONN_TCP; //��������ΪTCPЭ��
	user_tcp_espconn.proto.tcp->local_port = Local_port; //���ض˿�

	//ע�����ӳɹ��ص��������������ӻص�����
	espconn_regist_connectcb(&user_tcp_espconn, server_listen); //ע�� TCP ���ӳɹ�������Ļص�����
	espconn_regist_reconcb(&user_tcp_espconn, server_recon); //ע�� TCP ���ӷ����쳣�Ͽ�ʱ�Ļص������������ڻص������н�������
	espconn_accept(&user_tcp_espconn); //���� TCP server����������
	espconn_regist_time(&user_tcp_espconn, 180, 0); //���ó�ʱ�Ͽ�ʱ�� ��λ���룬���ֵ��7200 ��
	//�����ʱʱ������Ϊ 0��ESP8266 TCP server ��ʼ�ղ���Ͽ��Ѿ�������ͨ�ŵ� TCP client������������ʹ�á�
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


	os_timer_disarm(&checkTimer_wifistate); //ȡ����ʱ����ʱ
	os_timer_setfn(&checkTimer_wifistate, (os_timer_func_t *) Check_WifiState,
	NULL); //���ö�ʱ���ص�����
	os_timer_arm(&checkTimer_wifistate, 1000, true); //������ʱ������λ������


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

