#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <icm20608.h>
#include <math.h>

#define LOG_TAG     "icm.app"
#define LOG_LVL     LOG_LVL_DBG
#include <ulog.h>


//定义一个PI
#define M_PI 3.14

// 定义灵敏度系数  
#define SENSITIVITY 50.0  
  
// 互补滤波系数  
#define ALPHA 0.98

// 角度状态变量  
static double accPitch = 0.0, accRoll = 0.0;  
static double gyroPitch = 0.0, gyroRoll = 0.0;  


float pitch = 0.0; // 俯仰角
float roll = 0.0;  // 横滚角

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

    // 输出更新后的角度（可选）  
    rt_kprintf(" Pitch: %d \n", (int)pitch);  
    rt_kprintf(" Roll: %d \n", (int)roll);  
}  

static void icm_thread_entry(void *parameter)
{
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

    while (count++ < 100)
    {
        rt_int16_t accel_x, accel_y, accel_z;
        rt_int16_t gyros_x, gyros_y, gyros_z;

        /* 读取三轴加速度 */
        result = icm20608_get_accel(dev, &accel_x, &accel_y, &accel_z);
        if (result == RT_EOK)
        {
            LOG_D("current accelerometer: accel_x%6d, accel_y%6d, accel_z%6d", accel_x, accel_y, accel_z);
        }
        else
        {
            LOG_E("The sensor does not work");
        }

        /* 读取三轴陀螺仪 */
        result = icm20608_get_gyro(dev, &gyros_x, &gyros_y, &gyros_z);
        if (result == RT_EOK)
        {
            LOG_D("current gyroscope    : gyros_x%6d, gyros_y%6d, gyros_z%6d", gyros_x, gyros_y, gyros_z);
        }
        else
        {
            LOG_E("The sensor does not work");
            break;
        }

        /* 获取俯仰角和翻滚角 */
        updateAngles(accel_x, accel_y, accel_z,gyros_x, gyros_y);

        rt_thread_mdelay(1000);
    }
}

static int icm_app(void)
{
    rt_thread_t res = rt_thread_create("icm", icm_thread_entry, RT_NULL, 1024, 20, 50);
    if(res == RT_NULL)
    {
        return -RT_ERROR;
    }

    rt_thread_startup(res);
    
    return RT_EOK;
}
MSH_CMD_EXPORT(icm_app, icm_app);