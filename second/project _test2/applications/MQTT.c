#include <board.h>
#include <rtthread.h>
#include <drv_gpio.h>
#ifndef RT_USING_NANO
#include <rtdevice.h>
#endif /* RT_USING_NANO */

#include "rtthread.h"
#include "dev_sign_api.h"
#include "mqtt_api.h"

#define GPIO_LED_B    GET_PIN(F, 11)

#define LOG_TAG              "app_mqtt"
#define LOG_LVL              LOG_LVL_DBG
#include <ulog.h>

//置于全局变量
extern rt_mq_t mq_hum;
extern rt_mq_t mq_tem;
extern rt_mq_t mq_pitch;
extern rt_mq_t mq_roll;

static char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
static char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
static char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};

void *HAL_Malloc(uint32_t size);
void HAL_Free(void *ptr);
void HAL_Printf(const char *fmt, ...);
int HAL_GetProductKey(char product_key[IOTX_PRODUCT_KEY_LEN + 1]);
int HAL_GetDeviceName(char device_name[IOTX_DEVICE_NAME_LEN + 1]);
int HAL_GetDeviceSecret(char device_secret[IOTX_DEVICE_SECRET_LEN]);
uint64_t HAL_UptimeMs(void);
int HAL_Snprintf(char *str, const int len, const char *fmt, ...);

#define EXAMPLE_TRACE(fmt, ...)  \
    do { \
        HAL_Printf("%s|%03d :: ", __func__, __LINE__); \
        HAL_Printf(fmt, ##__VA_ARGS__); \
        HAL_Printf("%s", "\r\n"); \
    } while(0)



//处理MQTT消息到达的事件,回调
static void example_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;
    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            /* print topic name and topic message */
            EXAMPLE_TRACE("Message Arrived:");
            EXAMPLE_TRACE("Topic  : %.*s", topic_info->topic_len, topic_info->ptopic);
            EXAMPLE_TRACE("Payload: %.*s", topic_info->payload_len, topic_info->payload);
            EXAMPLE_TRACE("\n");
            
            break;
        default:
            break;
    }
}

static int example_subscribe(void *handle)
{
    int res = 0;
    const char *fmt = "/%s/%s/user/get";
    char *topic = NULL;
    int topic_len = 0;
    topic_len = strlen(fmt) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len);
    if (topic == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return -1;
    }
    memset(topic, 0, topic_len);
    HAL_Snprintf(topic, topic_len, fmt, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);
    res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, example_message_arrive, NULL);
    if (res < 0) {
        EXAMPLE_TRACE("subscribe failed");
        HAL_Free(topic);
        return -1;
    }
    HAL_Free(topic);
    return 0;
}




// 该函数用来从消息队列中读取温湿度数据组合成payload数据格式并传入example_publish函数
static void mqtt_tranfer_sensor(void *pclient)
{
    float humidity;//湿度
    float temperature;//温度
    float pitch;//俯仰角
    float roll;//横滚角

    char *payload = NULL;
    int payload_len = 0,res = 0;

    const char     *fmt = "/sys/%s/%s/thing/event/property/post";

    char           *topic = NULL;
    int topic_len = 0;
    topic_len = strlen(fmt) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;

    topic = HAL_Malloc(topic_len);
    if (topic == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return;
    }

    memset(topic, 0, topic_len);
    HAL_Snprintf(topic, topic_len, fmt, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);
    
    //接受队列
    rt_mq_recv(mq_hum, &humidity, sizeof(humidity), RT_WAITING_NO);
    rt_mq_recv(mq_pitch, &pitch, sizeof(pitch), RT_WAITING_NO);
    rt_mq_recv(mq_roll, &roll, sizeof(roll), RT_WAITING_NO);
    rt_mq_recv(mq_tem, &temperature, sizeof(temperature), RT_WAITING_NO);
  

    //计算payload所需长度，温度湿度数据保留一位小数上传，"params":{"CurrentTemperature":16.5,"CurrentHumidity":56.3}
    payload_len = strlen("{\"params\":{\"CurrentTemperature\":14.3,\"CurrentHumidity\":77.3,\"pitch\": -11.2,\"roll\": -34.3}}") + 3;
    // payload_len = strlen("{\"params\":{\"pitch\":11.2,\"roll\":34.3}}") + 3;



    payload = HAL_Malloc(payload_len);//开辟缓存区
    if (payload == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return;
    }

    memset(payload, 0, payload_len);
    HAL_Snprintf(payload, payload_len, "{\"params\":{\"CurrentTemperature\":%.1f, \"CurrentHumidity\":%.1f, \"pitch\":%.1f, \"roll\":%.1f}}", temperature, humidity,pitch,roll);

    res = IOT_MQTT_Publish_Simple(0, topic, IOTX_MQTT_QOS0, payload, strlen(payload));

    if (res < 0) {
        EXAMPLE_TRACE("publish failed, res = %d", res);
        HAL_Free(topic);
        HAL_Free(payload);
        return;
    }

    HAL_Free(topic);
    HAL_Free(payload);
}





static void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    EXAMPLE_TRACE("msg->event_type : %d", msg->event_type);
}





static void mqtt_example_main(void *p)
{
    void                   *pclient = NULL;
    int                     res = 0;
    int                     loop_cnt = 0;
    iotx_mqtt_param_t       mqtt_params;

    HAL_GetProductKey(DEMO_PRODUCT_KEY);
    HAL_GetDeviceName(DEMO_DEVICE_NAME);
    HAL_GetDeviceSecret(DEMO_DEVICE_SECRET);

    EXAMPLE_TRACE("mqtt example");

    /* Initialize MQTT parameter */
    memset(&mqtt_params, 0x0, sizeof(mqtt_params));
    mqtt_params.handle_event.h_fp = example_event_handle;
    pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == pclient) {
        EXAMPLE_TRACE("MQTT construct failed");
        return;
    }
    res = example_subscribe(pclient);
    if (res < 0) {
        IOT_MQTT_Destroy(&pclient);
        return;
    }

    while (1) {
        if (0 == loop_cnt % 20) {
            // example_publish(pclient);
            mqtt_tranfer_sensor(pclient);
        }
        IOT_MQTT_Yield(pclient, 200);
        loop_cnt += 1;
    }
    
    return;
}

static char mqtt_thread_stack[4096];
static struct rt_thread mqtt_thread;
static int mqtt_example()
{
    rt_thread_init(&mqtt_thread, "mqtt_thread", mqtt_example_main, RT_NULL, mqtt_thread_stack, sizeof(mqtt_thread_stack), 15, 10);
    rt_thread_startup(&mqtt_thread);
    return 0;
}
MSH_CMD_EXPORT(mqtt_example, app_ali_mqtt_sample);
