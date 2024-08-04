#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <icm20608.h>
#include <math.h>

#include <u8g2_port.h>
#include <rthw.h>

#include <drv_matrix_led.h>

#include <drv_gpio.h>
// #include <pin.h>

#define LOG_TAG     "icm_example"
#define LOG_LVL     LOG_LVL_DBG
#include <ulog.h>


//定义消息队列
rt_mq_t mq_pitch = RT_NULL;
rt_mq_t mq_roll = RT_NULL;

// 定义蜂鸣器引脚
#define BUZZER GET_PIN(B, 0)
//按键引脚
#define key_up        GET_PIN(C, 5)
#define key_down      GET_PIN(C, 1)



#define    ZERO       0

#define OLED_I2C_PIN_SCL                    22  // PB6
#define OLED_I2C_PIN_SDA                    23  // PB7




//定义一个PI
#define M_PI 3.14

// 定义灵敏度系数  
#define SENSITIVITY 70.0  
  
// 互补滤波系数  
#define ALPHA 0.45

// 角度状态变量  
static double accPitch = 0.0, accRoll = 0.0;  
static double gyroPitch = 0.0, gyroRoll = 0.0;  


float pitch = 0.0; // 俯仰角
float roll = 0.0;  // 横滚角

static rt_uint8_t KEY_FLAG = 0;



static int key_up_callback(void *args)
{
    uint8_t value = 0;
    value = rt_pin_read(key_up);
    if(!value)
    KEY_FLAG = 1;
}   
static int key_down_callback(void *args)
{
    uint8_t value = 0;
    value = rt_pin_read(key_down);
    if(!value)
    KEY_FLAG = 0;
        
}


// //按键驱动
// rt_pin_mode(key_up,    PIN_MODE_INPUT_PULLUP);
// rt_pin_mode(key_down,    PIN_MODE_INPUT_PULLUP);
// //选择上升沿触发
// rt_pin_attach_irq(key_up, PIN_IRQ_MODE_FALLING, key_up_callback, RT_NULL);
// rt_pin_attach_irq(key_down, PIN_IRQ_MODE_FALLING, key_down_callback, RT_NULL);
//  //使能中断
// rt_pin_irq_enable(key_up ,PIN_IRQ_ENABLE);
// rt_pin_irq_enable(key_down,PIN_IRQ_ENABLE);










// 更新角度的函数  
static void updateAngles(rt_int16_t AX, rt_int16_t AY, rt_int16_t AZ, rt_int16_t GX, rt_int16_t GY)
 {  

    // 计算加速度计推导的俯仰角和翻滚角  
    accPitch = atan2(AY, sqrt(AX * AX + AZ * AZ)) *(180.0 / M_PI);
    accRoll = atan2(-AX, AZ) * (180.0 / M_PI); 

    // 陀螺仪积分得到角速度
    float gyroPitch = pitch + (float)GY / SENSITIVITY; // 131 根据陀螺仪灵敏度调整
    float gyroRoll = roll + (float)GX / SENSITIVITY;
 
    // 综合加速度计和陀螺仪数据，使用互补滤波
    pitch = ALPHA * gyroPitch + (1.0 - ALPHA) * accPitch;
    roll = ALPHA * gyroRoll + (1.0 - ALPHA) * accRoll;

    // Pitch = pitch;
    // Roll = roll;

    // // 输出更新后的角度
    // rt_kprintf(" Pitch: %.2f\n", pitch);  
    // rt_kprintf(" Roll: %.2f \n", roll);  
}  

static void led_matrix()
{
    if(abs(pitch) < 30 && abs(roll) < 30)
    {
        led_matrix_fill_test(3);
        // rt_thread_mdelay(1000);
    }
    else if(50 > abs(pitch) > 20 || 50 > abs(roll) > 20)
    {
        led_matrix_fill_test(1);
    }
    else if(abs(pitch) > 50 || abs(roll) > 50 )
    {
        led_matrix_fill_test(0);
    }
    // led_matrix_rst();
}

static void icm_thread_entry(void *parameter)
{
    u8g2_t u8g2;
    // Initialization
    u8g2_Setup_ssd1306_i2c_128x64_noname_f( &u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_gpio_and_delay_rtthread);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_I2C_CLOCK, OLED_I2C_PIN_SCL);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_I2C_DATA, OLED_I2C_PIN_SDA);    

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

     char buffer1[32];
     char buffer2[32];

     char buffer3[32];
     char buffer4[32];
     char buffer5[32];
     char buffer6[32];


    icm20608_device_t dev = RT_NULL;
    const char *i2c_bus_name = "i2c2";
    int count = 0;
    rt_err_t result;

    /* 初始化 icm20608 传感器 */
    dev = icm20608_init(i2c_bus_name);
    if (dev == RT_NULL)
    {
        LOG_E("The sensor initializes failure");
    }
    else
    {
        LOG_D("The sensor initializes success");
    }

    /* 对 icm20608 进行零值校准：采样 10 次，求取平均值作为零值 */
    result = icm20608_calib_level(dev, 10);
    if (result == RT_EOK)
    {
        LOG_D("The sensor calibrates success");
        LOG_D("accel_offset: X%6d  Y%6d  Z%6d", dev->accel_offset.x, dev->accel_offset.y, dev->accel_offset.z);
        LOG_D("gyro_offset : X%6d  Y%6d  Z%6d", dev->gyro_offset.x, dev->gyro_offset.y, dev->gyro_offset.z);
    }
    else
    {
        LOG_E("The sensor calibrates failure");
        icm20608_deinit(dev);
    }

    

    // while (count++ < 100)
    while(1)
    {
        rt_int16_t accel_x, accel_y, accel_z;
        rt_int16_t gyros_x, gyros_y, gyros_z;

        /* 读取三轴加速度 */
        result = icm20608_get_accel(dev, &accel_x, &accel_y, &accel_z);
        if (result == RT_EOK)
        {
            // LOG_D("current accelerometer: accel_x%6d, accel_y%6d, accel_z%6d", accel_x, accel_y, accel_z);
        }
        else
        {
            LOG_E("The sensor does not work");
        }

        /* 读取三轴陀螺仪 */
        result = icm20608_get_gyro(dev, &gyros_x, &gyros_y, &gyros_z);
        if (result == RT_EOK)
        {
            // LOG_D("current gyroscope    : gyros_x%6d, gyros_y%6d, gyros_z%6d", gyros_x, gyros_y, gyros_z);
        }
        else
        {
            LOG_E("The sensor does not work");
            break;
        }

        /* 获取俯仰角和翻滚角 */
        updateAngles(accel_x, accel_y, accel_z,gyros_x, gyros_y);

        led_matrix();

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2,5, 10, "###############");
    
    if(abs(pitch) > 30 || abs(roll) > 30)
    {
    
        rt_sprintf(buffer1, "!! Tumble");
        u8g2_DrawStr(&u8g2,40, 25, buffer1);    
        rt_thread_delay(100);
    
    }
    else if(abs(pitch) < 30  && abs(roll) < 30)
    {    
        rt_sprintf(buffer1, "Healthy");
        u8g2_DrawStr(&u8g2,40, 25, buffer1);       
        rt_thread_delay(100);
       
    }
    // u8g2_SendBuffer(&u8g2);



    


    if(pitch >10)
    {
        rt_sprintf(buffer3, "front:%d °", abs(pitch));
        rt_sprintf(buffer4, "back: %d °", ZERO);
        
        // u8g2_ClearBuffer(&u8g2);
    }
    else if(pitch<(-10))
    {
        rt_sprintf(buffer3, "front:%d °", ZERO);
        rt_sprintf(buffer4, "back: %d °", abs(pitch));
        
        // u8g2_ClearBuffer(&u8g2);
    }
    else
    {
        rt_sprintf(buffer3, "front:%d °", ZERO);
        rt_sprintf(buffer4, "back: %d °", ZERO);
        
        // u8g2_ClearBuffer(&u8g2);
    }
    
    if(roll >10)
    {
        rt_sprintf(buffer5, "left: %d °",ZERO );
        rt_sprintf(buffer6, "right:%d °", abs(roll));
      
        // u8g2_ClearBuffer(&u8g2);
    }
    else if(roll<(-10))
    {
        rt_sprintf(buffer5, "left: %d °", abs(roll));
        rt_sprintf(buffer6, "right:%d °", ZERO);
        
        // u8g2_ClearBuffer(&u8g2);
    }
    else
    {       
        rt_sprintf(buffer5, "left: %d °", ZERO);
        rt_sprintf(buffer6, "right:%d °", ZERO);
        
        // u8g2_ClearBuffer(&u8g2);
    }


   u8g2_DrawStr(&u8g2,5, 40, buffer3);
   u8g2_DrawStr(&u8g2,5, 55, buffer4);
   u8g2_DrawStr(&u8g2,70, 40, buffer5);
   u8g2_DrawStr(&u8g2,70, 55, buffer6);

   u8g2_SendBuffer(&u8g2);












        //发送队列
        rt_mq_send(mq_pitch, &pitch, sizeof(pitch));
        rt_mq_send(mq_roll, &roll, sizeof(roll));

        rt_thread_mdelay(100);
    }
}

static int icm_example(void)
{
    //创建队列
    mq_pitch = rt_mq_create("mq_pitch", 10, sizeof(pitch), RT_IPC_FLAG_FIFO);
    mq_roll = rt_mq_create("mq_roll", 10, sizeof(roll), RT_IPC_FLAG_FIFO);

    rt_thread_t res = rt_thread_create("icm", icm_thread_entry, RT_NULL, 4096, 20, 50);
    if(res == RT_NULL)
    {
        return -RT_ERROR;
    }

    rt_thread_startup(res);
    
    return RT_EOK;
}
MSH_CMD_EXPORT(icm_example, icm example);