#include <stdio.h>
#include <unistd.h>//进程控制

#include "ohos_init.h"//系统启动
#include "cmsis_os2.h"//与任务管理相关的函数


#include "bsp_wifi.h"
#include "bsp_uart.h"
#include "bsp_led.h"
#include "bsp_mqtt.h"


#include "lwip/netifapi.h"
#include "lwip/sockets.h"
#include "lwip/api_shell.h"

#include "cJSON.h"

//任务优先级--数值越小优先级越高
#define BLUETOOTH_PRIORITY   osPriorityNormal
#define MQTT_PRIORITY    osPriorityNormal
#define PUB_SUB_PRIORITY   osPriorityNormal

//===============================================================  mqtt  ============================================
#define WIFI_SSID "Redmi"
#define WIFI_PAWD "xwysgxts"

#define SERVER_IP_ADDR "183.230.40.96"    //onenet
#define SERVER_IP_PORT 1883
#define TOPIC_SUB "$sys/Z4lEG4QWS0/myDevice/thing/property/set"//接收数据主题
#define TOPIC_PUB "$sys/Z4lEG4QWS0/myDevice/thing/property/post"//发送数据主题
#define SUB_REPLY "$sys/Z4lEG4QWS0/myDevice/thing/property/set_reply"


#define TASK_INIT_TIME 1 // s
#define MQTT_RECV_TASK_TIME (200 * 1000) // us


#define CLIENT_ID "myDevice"
#define DEVICE_NAME "Z4lEG4QWS0"
#define PASSWORD "version=2018-10-31&res=products%2FZ4lEG4QWS0%2Fdevices%2FmyDevice&et=1775115868&method=md5&sign=%2FnbMCb96Lx4l5MPJV3IM4A%3D%3D"

//系统
extern uint8_t mqtt_reconnect;

char* ID;
// uint8_t length_id=0;
uint8_t device=0;
//1
uint8_t AutoControl=0;
int Brightness=0;
uint8_t Lock=0;
uint8_t Status=0;
uint8_t Lightcolor=0;
//2
float temperature;
float humidity;
//3

uint8_t Lockopen=0;
uint8_t cardID[4];
uint8_t pattern=0;
//4
uint8_t closeDoor=0;

//控制任务
osThreadId_t mqtt_send_task_id;//mqtt
osThreadId_t bluetooth_task_id; //蓝牙
osThreadId_t Light_task_id;//灯
osThreadId_t Lock_nfc_task_id;//nfc
osThreadId_t Temhum_task_id;//温湿度
         
osThreadId_t light_recv_task_id;   // mqtt订阅数据任务


void Parsing_json_data(const char *payload){
    cJSON *root = NULL, *params = NULL, *id = NULL,*json_value = NULL;
    root = cJSON_Parse((const char *)payload);

    if(id = cJSON_GetObjectItem(root, "id")){
        // unsigned char reply[100];
        ID=id->valuestring;
        // length_id=strlen(ID);
        printf(" ID:%s\r\n",ID);
        
    }
    
    if (root){
        params = cJSON_GetObjectItem(root, "params");
        if(params){
            if(json_value=cJSON_GetObjectItem(params, "AutoControl")){
                AutoControl=json_value->valueint;
                printf("AutoControl:%d\r\n",AutoControl);
            }
            if(json_value=cJSON_GetObjectItem(params, "Brightness")){
                Brightness=json_value->valueint;
                printf("Brightness:%d\r\n",Brightness);
            }
            if(json_value=cJSON_GetObjectItem(params, "Lightcolor")){
                Lightcolor=json_value->valueint;
                printf("Lightcolor:%d\r\n",Lightcolor);
            }
            if(json_value=cJSON_GetObjectItem(params, "Lock")){
                Lock=json_value->valueint;
                printf("Lock:%d\r\n",Lock);
            }
            if(json_value=cJSON_GetObjectItem(params, "Status")){
                Status=json_value->valueint;
                printf("Status:%d\r\n",Status); 
            }
            if(json_value=cJSON_GetObjectItem(params, "device")){
                device=json_value->valueint;
                printf("device:%d\r\n",device);
            }
            
        }
    }
}

int8_t mqtt_sub_payload_callback(unsigned char *topic, unsigned char *payload)
{
    unsigned char str[17];
    unsigned char reply[100];
    // sprintf(reply,"{\"id\": \"%s\",\"code\": 200,\"msg\": \"success\"}",ID?ID:"2");
    printf("[info] topic:[%s] \r\n   recv<== %s\r\n", topic, payload);
    //解析JSON数据
    Parsing_json_data(payload);
    sprintf(reply,"{\"id\": \"%s\",\"code\": 200,\"msg\": \"success\"}",ID);
        printf("%s",reply);
    // if(device==1){
        str[0]='1';
        str[1]=AutoControl+'0';
        for(int i=0;i<3;i++){
            str[4-i]=(Brightness%10)+'0';
            Brightness/=10;
        }
        str[5]=Lightcolor+'0';
        str[6]=Lock+'0';
        str[7]=Status+'0';
        str[8]='\0';
        uart2_send_data(str,8);
        MQTTClient_pub(SUB_REPLY,reply, strlen(reply));
    // }
    // if(device==2){
        
    // }
}

void light_recv_task(void){
    while(1){
        MQTTClient_sub();
        usleep(MQTT_RECV_TASK_TIME);
    }
}
//订阅主题
void mqtt_subscribe(char *subTopic){
    // 订阅Topic
    if (MQTTClient_subscribe(subTopic) != 0) 
    {
        printf("[error] MQTTClient_subscribe\r\n");
    } 
    else 
    {
        printf("[success] MQTTClient_subscribe\r\n");
    }
    sleep(TASK_INIT_TIME); 
}

/**
 * 订阅主题-
*/
void mqtt_send_task(void)
{
    while(1){
        if(mqtt_reconnect==1){
                // uint8_t res=0;
            // 连接Wifi
            if (WiFi_connectHotspots(WIFI_SSID, WIFI_PAWD) != WIFI_SUCCESS) 
            {
                printf("[error] WiFi_connectHotspots\r\n");
            }

            // 连接MQTT服务器
            if (MQTTClient_connectServer(SERVER_IP_ADDR, SERVER_IP_PORT) != 0) 
            {
                printf("[error] MQTTClient_connectServer\r\n");
            } 
            else 
            {
                printf("[success] MQTTClient_connectServer\r\n");
            }
            sleep(TASK_INIT_TIME);

            if (MQTTClient_init(CLIENT_ID, DEVICE_NAME, PASSWORD) != 0) 
            {
                printf("[error] MQTTClient_init\r\n");
            } 
            else 
            {
                printf("[success] MQTTClient_init\r\n");
            }
            sleep(TASK_INIT_TIME);

            /*订阅主题*/
            mqtt_subscribe(TOPIC_PUB);
            mqtt_subscribe(TOPIC_SUB);
            mqtt_subscribe(SUB_REPLY);

            temperature_task_create();
            light_task_create();
            
            lockNFC_task_create();
            mqtt_reconnect=0;

            // light_receive();
        }
        watchdog_feed();  // 必须定期调用
        osDelay(100);
    }
    
}

void BlueTooth_task(void)
{
    static const char *data = "Hello,this is my work\r\n";
    static const char *read = "read:\r\n";

    uint8_t recbuf[17];
    uint8_t len=0;

    uart2_init(115200);
    uart2_send_data(data,strlen(data));

    // led_init();
    // LED(0);
    while (1) 
    {
        // uart2_send_data(read,strlen(read));

        len=uart2_read_data(recbuf,16);//每次只能读取14个有效字符

        // for(int i=0;i<len;i++){
        //     printf("%c",recbuf[i]);
        // }
        // printf("\r\n---------------\r\n");

        if(recbuf[0]=='1'){//智能照明数据--7位--device=1AutoControl=0Brightness=255Lock=0Status=1
            AutoControl=recbuf[1]-'0';
            Brightness=(recbuf[2]-'0')*100+(recbuf[3]-'0')*10+(recbuf[4]-'0');
            Lightcolor=recbuf[5]-'0';
            Lock=recbuf[6]-'0';
            Status=recbuf[7]-'0';
            printf("AutoControl:%d Brightness:%d Lightcolor:%d Lock:%d Status:%d\r\n",AutoControl,Brightness,Lightcolor,Lock,Status);
        }
        if(recbuf[0]=='2'){//温湿度数据
            temperature=(recbuf[1]-'0')*10+(recbuf[2]-'0')+0.1*(recbuf[3]-'0')+0.01*(recbuf[4]-'0');
            printf("temprature:%.2f\r\n",temperature);
            humidity=(recbuf[5]-'0')*10+(recbuf[6]-'0')+0.1*(recbuf[7]-'0')+0.01*(recbuf[8]-'0');
            printf("humidity:%.2f\r\n",humidity);
        }

        if(recbuf[0]=='3'){//NFC数据
            Lockopen=recbuf[1]-'0';
            for(int i=0,j=2;i<4;i++,j+=2){
                if(recbuf[j]<='9'&&recbuf[j]>='0'){
                    cardID[i]=(recbuf[j]-'0')*16;
                }else{
                    cardID[i]=(recbuf[j]-'A'+10)*16;
                }
                if(recbuf[j+1]<='9'&&recbuf[j+1]>='0'){
                    cardID[i]+=(recbuf[j+1]-'0');
                }else{
                    cardID[i]+=(recbuf[j+1]-'A'+10);
                }
            }
            pattern=recbuf[10]-'0';
            printf("Lockopen:%d cardID:%02X-%02X-%02X-%02X pattern:%d\r\n",Lockopen,cardID[0],cardID[1],cardID[2],cardID[3],pattern);
        }
        if(recbuf[0]=='4'){
            closeDoor=recbuf[1]-'0';//1开门，0关门
            printf("closeDoor:%d\r\n",closeDoor);
        }

        // if(len>0)uart2_send_data(recbuf,len);

        // sleep(2);
    }
}

void Temhum_task(void){
        /*接收数据*/
        // osThreadAttr_t options;
        // options.name = "mqtt_recv_task";
        // options.attr_bits = 0;
        // options.cb_mem = NULL;
        // options.cb_size = 0;
        // options.stack_mem = NULL;
        // options.stack_size = 1024*5;
        // options.priority = PUB_SUB_PRIORITY;
    
        // mqtt_recv_task_id = osThreadNew((osThreadFunc_t)mqtt_recv_task, NULL, &options);
        // if (mqtt_recv_task_id != NULL) 
        // {
        //     printf("ID = %d, Create mqtt_recv_task_id is OK!\r\n", mqtt_recv_task_id);
        // }
   
    /*发送数据*/
    while (1) 
    {
        if(mqtt_reconnect==0){
            unsigned char str[200];
            sprintf(str,"{\"id\":\"1741697592945\",\"version\":\"1.0\",\"params\":{\"Humidity\":{\"value\":%.2f},\"Temperature\":{\"value\":%.2f}}}",humidity,temperature);
            MQTTClient_pub(TOPIC_PUB,str, strlen(str));//发布消息
        }
        
        sleep(4);

    }
}

void Light_task(void){

        /*接收数据*/
        osThreadAttr_t options;
        p_MQTTClient_sub_callback = &mqtt_sub_payload_callback;
        options.name = "light_recv_task";
        options.attr_bits = 0;
        options.cb_mem = NULL;
        options.cb_size = 0;
        options.stack_mem = NULL;
        options.stack_size = 1024*7;
        options.priority = PUB_SUB_PRIORITY;
    
        light_recv_task_id = osThreadNew((osThreadFunc_t)light_recv_task, NULL, &options);
        if (light_recv_task_id != NULL) 
        {
            printf("ID = %d, Create light_recv_task_id is OK!\r\n", light_recv_task_id);
        }
    
    /*发送数据*/
    while (1) {
        if(AutoControl==1&&mqtt_reconnect==0){
            unsigned char str[200];
            sprintf(str,"{\"id\":\"1743411867977\",\"version\":\"1.0\",\"params\":{\"AutoControl\":{\"value\":%d},\"Brightness\":{\"value\":%d},\"Lock\":{\"value\":%d},\"Status\":{\"value\":%d}}}",AutoControl,Brightness,Lock,Status);
            MQTTClient_pub(TOPIC_PUB,str, strlen(str));//发布消息
        }
        
        sleep(10);
    }
}

void Lock_nfc_task(void){
    while (1) 
        {
            if(mqtt_reconnect==0){
                if(Lockopen==1){
                    unsigned char str[200];
                    sprintf(str,"{\"id\":\"1743836920243\",\"version\":\"1.0\",\"params\":{\"cardID\":{\"value\":[%d,%d,%d,%d]},\"pattern\":{\"value\":%d}}}",cardID[0],cardID[1],cardID[2],cardID[3],pattern);
                    MQTTClient_pub(TOPIC_PUB,str, strlen(str));//发布消息
                    Lockopen=0;
                }
                    unsigned char str[200];
                    sprintf(str,"{\"id\":\"1744359751915\",\"version\":\"1.0\",\"params\":{\"closeDoor\":{\"value\":%d}}}",closeDoor);
                    MQTTClient_pub(TOPIC_PUB,str, strlen(str));//发布消息
            }
            sleep(4);
        }
}


void mqtt_task_create(void)
{
    osThreadAttr_t taskOptions;
    taskOptions.name = "mqttTask";       // 任务的名字
    taskOptions.attr_bits = 0;               // 属性位
    taskOptions.cb_mem = NULL;               // 堆空间地址
    taskOptions.cb_size = 0;                 // 堆空间大小
    taskOptions.stack_mem = NULL;            // 栈空间地址
    taskOptions.stack_size = 1024*7;           // 栈空间大小 单位:字节
    taskOptions.priority = MQTT_PRIORITY; // 任务的优先级

    mqtt_send_task_id = osThreadNew((osThreadFunc_t)mqtt_send_task, NULL, &taskOptions); // 创建任务
    if (mqtt_send_task_id != NULL)
    {
        printf("ID = %d, mqtt_send_task_id Create OK!\n", mqtt_send_task_id);
    }
}
/*LUBETOOTH----蓝牙接收设备数据-----------------------------------*/
void Linked_Bluetooth(void)
{
    osThreadAttr_t taskOptions;
    taskOptions.name = "mqttTask1";       // 任务的名字
    taskOptions.attr_bits = 0;               // 属性位
    taskOptions.cb_mem = NULL;               // 堆空间地址
    taskOptions.cb_size = 0;                 // 堆空间大小
    taskOptions.stack_mem = NULL;            // 栈空间地址
    taskOptions.stack_size = 1024*4;           // 栈空间大小 单位:字节
    taskOptions.priority = BLUETOOTH_PRIORITY; // 任务的优先级

    bluetooth_task_id = osThreadNew((osThreadFunc_t)BlueTooth_task, NULL, &taskOptions); // 创建任务
    if (bluetooth_task_id != NULL)
    {
        printf("ID = %d, bluetooth_task_id Create OK!\n", bluetooth_task_id);
    }
}
// -----------------------------温度---------------------------
void temperature_task_create(void){
    osThreadAttr_t taskOptions;
    p_MQTTClient_sub_callback = &mqtt_sub_payload_callback;
    taskOptions.name = "mqttTask2";       // 任务的名字
    taskOptions.attr_bits = 0;               // 属性位
    taskOptions.cb_mem = NULL;               // 堆空间地址
    taskOptions.cb_size = 0;                 // 堆空间大小
    taskOptions.stack_mem = NULL;            // 栈空间地址
    taskOptions.stack_size = 1024*3;           // 栈空间大小 单位:字节
    taskOptions.priority = PUB_SUB_PRIORITY; // 任务的优先级

    Temhum_task_id = osThreadNew((osThreadFunc_t)Temhum_task, NULL, &taskOptions); // 创建任务
    if (Temhum_task_id != NULL)
    {
        printf("ID = %d, Temhum_task_id Create OK!\n", Temhum_task_id);
    }
}

// -----------------------------照明--------------------------------
// void light_receive(void){
//     /*接收数据*/
//     osThreadAttr_t options;
//     p_MQTTClient_sub_callback = &mqtt_sub_payload_callback;
//     options.name = "light_recv_task";
//     options.attr_bits = 0;
//     options.cb_mem = NULL;
//     options.cb_size = 0;
//     options.stack_mem = NULL;
//     options.stack_size = 1024*7;
//     options.priority = PUB_SUB_PRIORITY;

//     light_recv_task_id = osThreadNew((osThreadFunc_t)light_recv_task, NULL, &options);
//     if (light_recv_task_id != NULL) 
//     {
//         printf("ID = %d, Create light_recv_task_id is OK!\r\n", light_recv_task_id);
//     }
// }
void light_task_create(void){
    osThreadAttr_t taskOptions;
    taskOptions.name = "mqttTask3";       // 任务的名字
    taskOptions.attr_bits = 0;               // 属性位
    taskOptions.cb_mem = NULL;               // 堆空间地址
    taskOptions.cb_size = 0;                 // 堆空间大小
    taskOptions.stack_mem = NULL;            // 栈空间地址
    taskOptions.stack_size = 1024*3;           // 栈空间大小 单位:字节
    taskOptions.priority = PUB_SUB_PRIORITY; // 任务的优先级

    Light_task_id = osThreadNew((osThreadFunc_t)Light_task, NULL, &taskOptions); // 创建任务
    if (Light_task_id != NULL)
    {
        printf("ID = %d, Light_task_id Create OK!\n", Light_task_id);
    }
}

//--------------------------门锁------------------
void lockNFC_task_create(void){
    osThreadAttr_t taskOptions;
    p_MQTTClient_sub_callback = &mqtt_sub_payload_callback; 
    taskOptions.name = "mqttTask4";       // 任务的名字
    taskOptions.attr_bits = 0;               // 属性位
    taskOptions.cb_mem = NULL;               // 堆空间地址
    taskOptions.cb_size = 0;                 // 堆空间大小
    taskOptions.stack_mem = NULL;            // 栈空间地址
    taskOptions.stack_size = 1024*3;           // 栈空间大小 单位:字节
    taskOptions.priority = PUB_SUB_PRIORITY; // 任务的优先级

    Lock_nfc_task_id = osThreadNew((osThreadFunc_t)Lock_nfc_task, NULL, &taskOptions); // 创建任务
    if (Lock_nfc_task_id != NULL)
    {
        printf("ID = %d, Lock_nfc_task_id Create OK!\n", Lock_nfc_task_id);
    }
}
/**
 * @description: 初始化并创建任务
 * @param {*}
 * @return {*}
 */
static void template_demo(void)
{

    //蓝牙，lock,temperature,light
    mqtt_task_create();//mqtt连接onenet,temperature humidity
    Linked_Bluetooth();

}
SYS_RUN(template_demo);
