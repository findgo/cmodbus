# README #
未说明,multiplemodbus将发布测试过后的正常版本
分支multiplemodbus_BETA为开发升级用

本工程实现multiple modbus demo

本工程实现多主或多从modbus rtu or ascii 
可以通过mbconfig.h的宏进行控制切换
可同时支持主从机,但未测试


硬件接口
MBCOM0
PA8  --- TX
PA9  --- RX
定时器 TIM3
MBCOM1
PA2 --- TX
PA3 --- RX
定时器 TIM4