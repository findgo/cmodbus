# README #

This README would normally document whatever steps are necessary to get your application up and running.

### What is this repository for? ###

* Quick summary
* Version
* [Learn Markdown](https://bitbucket.org/tutorials/markdowndemo)

### How do I get set up? ###

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

本工程实现multiple modbus demo

本工程实现双主或双从modbus rtu or ascii 
可以通过mbconfig.h的宏进行控制切换

硬件接口
MBCOM0
PA8  --- TX
PA9  --- RX
定时器 TIM3
MBCOM1
PA2 --- TX
PA3 --- RX
定时器 TIM4