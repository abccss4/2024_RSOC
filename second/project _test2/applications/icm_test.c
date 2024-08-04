#include <rtthread.h>
#include <rtdevice.h>

// //定义一个PI
// #define M_PI 3.14

// // 定义灵敏度系数  
// #define SENSITIVITY 131.0  
  
// // 互补滤波系数  
// #define ALPHA 0.98

// // 角度状态变量  
// static double accPitch = 0.0, accRoll = 0.0;  
// static double gyroPitch = 0.0, gyroRoll = 0.0;  


// float pitch = 0.0; // 俯仰角
// float roll = 0.0;  // 横滚角

// // 更新角度的函数  
// static void updateAngles(rt_int16_t AX, rt_int16_t AY, rt_int16_t AZ, rt_int16_t GX, rt_int16_t GY)
//  {  

//     // 计算加速度计推导的俯仰角和翻滚角  
//     accPitch = atan2(AY, sqrt(AX * AX + AZ * AZ)) *(180.0 / M_PI);
//     accRoll = atan2(-AX, AZ) * (180.0 / M_PI); 

//     // 陀螺仪积分得到角速度
//     float gyroPitch = pitch + (float)GY / SENSITIVITY; // 131 根据陀螺仪灵敏度调整
//     float gyroRoll = roll + (float)GX / SENSITIVITY;
 
//     // 综合加速度计和陀螺仪数据，使用互补滤波
//     pitch = ALPHA * gyroPitch + (1.0 - ALPHA) * accPitch;
//     roll = ALPHA * gyroRoll + (1.0 - ALPHA) * accRoll;

//     // 输出更新后的角度（可选）  
//     rt_kprintf(" Pitch: %.2f \n", pitch);  
//     rt_kprintf(" Roll: %.2f \n", roll);  
// }  

