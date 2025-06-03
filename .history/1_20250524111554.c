/*
 * bsp_uart.c
 *
 *  Created on: 2025年5月21日
 *      Author: 20148
 */



#include "bsp_uart.h"


#define ID ""
#define PASSWORD "9876543210"

#define SeverIP "192.168.103.166"
#define SeverPort "8000"


void UART0_Init(void)
{
   fsp_err_t err = FSP_SUCCESS;

   err = R_SCI_UART_Open (&g_uart0_ctrl, &g_uart0_cfg);
   assert(FSP_SUCCESS == err);
}


_Bool Uart9_Receive_Flag = false;  //用来判断UART9接收以及发送数据是否完成
_Bool Uart9_Show_flag = false;  //用来判断UART9接收以及发送数据是否完成

/*用来接收UART9数据的缓冲区*/
char At_Rx_Buff[256];
uint8_t Uart9_Num = 0;

void ESP8266_UART9_Init(void)
{
    fsp_err_t err = FSP_SUCCESS;

     R_SCI_UART_Open(g_uart9_esp8266.p_ctrl, g_uart9_esp8266.p_cfg);
     assert(FSP_SUCCESS == err);
}


void ESP8266_ERROR_Alarm(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_06, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_06, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_SECONDS);
}

//使能ESP8266模块
void ESP8266_MODULE_ENABLE(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_15, BSP_IO_LEVEL_HIGH);
}


//关闭ESP8266模块
void ESP8266_MODULE_DISABLE(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_15, BSP_IO_LEVEL_LOW);
}


void Clear_Buff(void)
{
    memset(At_Rx_Buff, 0, sizeof(At_Rx_Buff) );
    Uart9_Num = 0;
}

uint8_t ESP8266_Init[8][100] = {"Initializing ESP8266...\n",
                                "In STA mode setup...\n",
                                "Connecting to WIFI...\n",
                                "Connecting to the server...\n",
                                "Transmitting mode is being configured...\n",
                                "ESP8266->AP mode\n",
                                "ESP8266->STA+AP mode\n",
                                "ESP8266->STA\n"};

/*自动配置ESP8266函数*/
void ESP8266_STA_Test(void)
{
    R_SCI_UART_Write(&g_uart9_esp8266_ctrl, ESP8266_Init[0], strlen((char*)ESP8266_Init[0]));
    ESP8266_UART9_Init();

    R_SCI_UART_Write(&g_uart9_esp8266_ctrl, ESP8266_Init[1], strlen((char*)ESP8266_Init[1]));
    ESP8266_STA();

    R_SCI_UART_Write(&g_uart9_esp8266_ctrl, ESP8266_Init[2], strlen((char*)ESP8266_Init[2]));
    ESP8266_STA_JoinAP(ID, PASSWORD, 20);
    Link_Mode(0);

    R_SCI_UART_Write(&g_uart9_esp8266_ctrl, ESP8266_Init[3], strlen((char*)ESP8266_Init[3]));
    ESP8266_STA_JoinServer(SeverIP, SeverPort, 20);

    R_SCI_UART_Write(&g_uart9_esp8266_ctrl, ESP8266_Init[4], strlen((char*)ESP8266_Init[4]));
    ESP8266_STA_Transmission();
    ESP8266_Send_Data();
}



/*向ESP8266发送AT指令函数*/
void ESP8266_AT_Send(char* cmd )
{
    /*向ESP8266(UART9)发送指令*/
    R_SCI_UART_Write(&g_uart9_esp8266_ctrl, (uint8_t *)cmd, strlen(cmd));

    /*AT指令发送完成标志*/
    Uart9_Receive_Flag = false;

}


 /*设置ESP8266为 STA 模式*/
void ESP8266_STA(void)
{
    ESP8266_AT_Send ( "AT+CWMODE=1\r\n" );

    /*等待设置完成*/
    while (!Uart9_Receive_Flag)
    {
        if (strstr( At_Rx_Buff , "OK\r\n" ))
        {
            // ESP8266已切换为STA模式
            R_SCI_UART_Write(&g_uart9_esp8266_ctrl, ESP8266_Init[7], strlen((char*)ESP8266_Init[7]));
            Clear_Buff();      //清除缓冲区数据
        }
    }
}


/*设置ESP8266为 AP 模式*/
void ESP8266_AP(void)
{
    ESP8266_AT_Send("AT+CWMODE=2\n");

    /*等待设置完成*/
    while(!Uart9_Receive_Flag)
    {
        if(strstr(At_Rx_Buff, "OK\n"))
        {
            // ESP8266已切换为AP模式
            R_SCI_UART_Write(&g_uart9_esp8266_ctrl, ESP8266_Init[5], strlen((char*)ESP8266_Init[5]));
            Clear_Buff();
        }
    }
}


/*设置ESP8266为 STA + AP 模式*/
void ESP8266_STA_AP(void)
{
    ESP8266_AT_Send("AT+CWMODE=3\n");

    /*等待设置完成*/
    while(!Uart9_Receive_Flag)
    {
        if(strstr(At_Rx_Buff, "OK\n"))
        {
            // ESP8266已切换为STA+AP模式
            R_SCI_UART_Write(&g_uart9_esp8266_ctrl, ESP8266_Init[6], strlen((char*)ESP8266_Init[6]));
            Clear_Buff();
        }
    }
}


uint8_t STR_ESP8266_STA_JoinAP[6][150] = {"The WIFI connection was successful\n",
                                          "The WIFI connection has timed out. Please check if all configurations are correct\n",
                                          "The WIFI password is incorrect. Please check if the Wifi password is correct\n",
                                          "The target WIFI cannot be found. Please check if the Wifi is turned on or if the WIFI name is correct\n",
                                          "The WIFI connection failed. Please check if all configurations are correct\n",
                                          "If the WIFI connection lasts longer than expected, please check if all configurations are correct\n"};

/*ESP8266连接WiFi函数*/
// timeout 期望连接时的时间，单位为秒
void ESP8266_STA_JoinAP(char* id, char* password, uint8_t timeout)
{
    char JoinAP_AT[256];
    uint8_t i;
    sprintf( JoinAP_AT , "AT+CWJAP=\"%s\",\"%s\"\r\n" , id , password);
    ESP8266_AT_Send(JoinAP_AT);

    /*判断WIFI连接是否成功*/
    for(i = 0; i <= timeout; i++)
    {
        if(strstr(At_Rx_Buff, "OK\n"))
        {
            R_SCI_UART_Write(&g_uart9_esp8266_ctrl, STR_ESP8266_STA_JoinAP[0], strlen((char*)STR_ESP8266_STA_JoinAP[0]));
            Clear_Buff();
            break;
        }
        if(strstr(At_Rx_Buff, "ERROR\n"))
        {
            if(strstr(At_Rx_Buff,"+CWJAP:1\n"))
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, STR_ESP8266_STA_JoinAP[1], strlen((char*)STR_ESP8266_STA_JoinAP[1]));
            }

            if(strstr(At_Rx_Buff, "+CWJAP:2\n"))
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, STR_ESP8266_STA_JoinAP[2], strlen((char*)STR_ESP8266_STA_JoinAP[2]));
            }

            if(strstr(At_Rx_Buff, "+CWJAP:3\n"))
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, STR_ESP8266_STA_JoinAP[3], strlen((char*)STR_ESP8266_STA_JoinAP[3]));
            }

            if(strstr(At_Rx_Buff, "+CWJAP:4\n"))
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, STR_ESP8266_STA_JoinAP[4], strlen((char*)STR_ESP8266_STA_JoinAP[4]));
            }

            while(1)
            {
                ESP8266_ERROR_Alarm();
            }
        }
        if(i == timeout)
        {
            R_SCI_UART_Write(&g_uart9_esp8266_ctrl, STR_ESP8266_STA_JoinAP[5], strlen((char*)STR_ESP8266_STA_JoinAP[5]));
            while(1)
            {
                ESP8266_ERROR_Alarm();
            }
        }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_SECONDS);
    }
}

uint8_t str_Link_Mode[2][150] = {"The ESP8266 has been switched to multi-connection mode\n",
                                 "The ESP8266 has been switched to single connection mode\n"};

void Link_Mode(uint8_t mode)
{
    switch(mode)
    {
        case 0:
            ESP8266_AT_Send("AT+CIPMUX=0\n");
            break;
        case 1:
            ESP8266_AT_Send("AT+CIPMUX=1\n");
            break;
    }

    while(!Uart9_Receive_Flag)
    {
        if(strstr(At_Rx_Buff, "OK\n"))
        {
            if(mode)
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, str_Link_Mode[0], strlen((char*)str_Link_Mode[0]));
            }
            else
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, str_Link_Mode[1], strlen((char*)str_Link_Mode[1]));
            }
            Clear_Buff();
        }
    }
}


uint8_t str_ESP8266_STA_JoinServer[3][150] = {"The server connection was successful.\n",
                                              "The server connection fails, please check if the server is open and the parameter is correct\n",
                                              "The server connection has exceeded the expected time. Please check whether all configurations are correct\n"};

/*ESP8266连接服务器函数*/
void ESP8266_STA_JoinServer(char* server_id, char* port, uint8_t timeout) // timeout：期望连接时间，单位为秒
{
    char JoinServer_AT[256];
    uint8_t i;
    sprintf(JoinServer_AT, "AT+CIPSTART=\"TCP\",\"%s\",%s\n", server_id, port);
    ESP8266_AT_Send(JoinServer_AT);

    /*判断服务器连接是否设置成功*/
    while(!Uart9_Receive_Flag)
    {
        /*超时判断，timeout：期望最长等待时间*/
        for(i = 0; i <= timeout; i++)
        {
            if(strstr(At_Rx_Buff, "OK\n"))
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, str_ESP8266_STA_JoinServer[0], strlen((char*)str_ESP8266_STA_JoinServer[0]));
                Clear_Buff();
                break;
            }

            if(strstr(At_Rx_Buff , "ERROR\n"))
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, str_ESP8266_STA_JoinServer[1], strlen((char*)str_ESP8266_STA_JoinServer[1]));
                while(1)
                {
                    ESP8266_ERROR_Alarm();//LED灯警告错误，红灯闪烁
                }
            }

            if(i == timeout)
            {
                R_SCI_UART_Write(&g_uart9_esp8266_ctrl, str_ESP8266_STA_JoinServer[2], strlen((char*)str_ESP8266_STA_JoinServer[2]));
                while(1)
                {
                    ESP8266_ERROR_Alarm();
                }
            }

            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_SECONDS);
        }
    }
}

uint8_t STR_ESP8266_Transmission[2][100] = {"The ESP8266 has been switched to transparent mode\n",
                                            "The ESP8266 has entered the transparent transmission mode\n"};

/*设置ESP8266为透传模式*/
void ESP8266_STA_Transmission(void)
{
    ESP8266_AT_Send("AT+CIPMODE=1\n");

    /*等待设置完成*/
    while(!Uart9_Receive_Flag)
    {
        if(strstr(At_Rx_Buff, "OK\n"))
        {
            R_SCI_UART_Write(&g_uart9_esp8266_ctrl, STR_ESP8266_Transmission[0], strlen((char*)STR_ESP8266_Transmission[0]));
            Clear_Buff();
        }
    }
}


/*设置ESP8266为发送数据模式*/
void ESP8266_Send_Data(void)
{
    ESP8266_AT_Send("AT+CIPSEND\n");

    /*等待设置完成*/
    while(!Uart9_Receive_Flag)
    {
        if (strstr(At_Rx_Buff , "OK\r\n"))
        {
            R_SCI_UART_Write(&g_uart9_esp8266_ctrl, STR_ESP8266_Transmission[1], strlen((char*)STR_ESP8266_Transmission[1]));
            Uart9_Show_flag = true;
            Clear_Buff();      //清除缓冲区数据
        }
}


// void esp8266_uart9_callback(uart_callback_args_t *p_args)
// {
//     switch (p_args->event)
//     {
//         case UART_EVENT_RX_CHAR:
//         {
//             At_Rx_Buff[Uart9_Num++] = (char)p_args->data;  //将UART9收到的数据放到Buff缓冲区中
//             /*进入透传模式后打开串口调试助手收发数据显示*/
//             if (Uart9_Show_flag)
//                 R_SCI_UART_Write (&g_uart0_ctrl, (uint8_t*) &(p_args->data), 1);
//             break;
//         }
//         case UART_EVENT_TX_COMPLETE:
//         {
//             Uart9_Receive_Flag = true;      //ESP8266回应完成标志
//             break;
//         }
//         default:
//             break;
//     }
// }




