#include <board.h>
#include <rtthread.h>
#include <drv_gpio.h>
#include <rtdevice.h>
#include "aht10.h"

// #include<drv_led.h>

#define LOG_TAG              "AHT10"
#define LOG_LVL              LOG_LVL_DBG
#include <ulog.h>

static float humidity, temperature;
static aht10_device_t dev;
static int count;



/* 总线名称 */
static const char *i2c_bus_name = "i2c3";

static rt_thread_t thread1 = RT_NULL;
rt_mq_t mq_hum = RT_NULL;
rt_mq_t mq_tem = RT_NULL;




static void thread1_entry(void *p)
{


    
    while (1)
    {
        /* 读取湿度 */
        humidity = aht10_read_humidity(dev);
        
       
        // LOG_I("humidity:%.2f",humidity);


        rt_mq_send(mq_hum, &humidity, sizeof(humidity));

        /* 读取温度 */
        temperature = aht10_read_temperature(dev);
       

        // LOG_I("temperature:%.2f",temperature);
        rt_mq_send(mq_tem, &temperature, sizeof(temperature));

        // rt_sprintf(buffer1, "temp: %d ℃", abs(temperature));
        // rt_sprintf(buffer2, "hum:  %d", abs(humidity));

        // u8g2_DrawStr(&u8g2,5, 10, buffer1); 
        // u8g2_DrawStr(&u8g2,70, 10, buffer2); 



        rt_thread_mdelay(100);
    }
}
static int AHT10_example(void *p)
{
    /* 初始化 aht10 */
    dev = aht10_init(i2c_bus_name);
    if (dev == RT_NULL)
    {
        LOG_E(" The sensor initializes failure");
        return 0;
    }
   
    mq_hum = rt_mq_create("mq_hum", 10, sizeof(humidity), RT_IPC_FLAG_FIFO);
    mq_tem = rt_mq_create("mq_tem", 10, sizeof(temperature), RT_IPC_FLAG_FIFO);
 
    thread1 = rt_thread_create("thread1", thread1_entry, RT_NULL, 1024, 25, 10);
    if (thread1 != RT_NULL)
    {
        rt_thread_startup(thread1);
    }
    return RT_EOK;
}

MSH_CMD_EXPORT(AHT10_example, AHT10 example);

