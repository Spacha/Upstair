# Upstair
Health monitoring system for Texas Instruments Real Time Operating System (TI-RTOS).

## Main features
The device in use: http://www.ti.com/tool/CC2650STK

The goal was to make a software that encourages the user to make healthier choices; using stairs over elevator. Technically the device has accelometer and gyroscope, from which I read data and interpret it using some simple statistics (NOTE: I hadn't got any statistical math courses at the time). So, based on the calculations (basically comparing the variance of the data to a mean of last couple of seconds) the device could tell if the user was:

1. Going up/down stairs
2. Using elevator to go up/down
3. Doing neither

The gyro was used (jointly with the accelomter) to take the orientation of the device into account before interpreting the accelometer data.

If the user was using stairs, a broadcast was sent to all surrounding devices telling how sporty the user was. It could also receive messages from similar devices and store them for later reading. It had also many other simple features including graphical menu (Nokia 3310 style) and simple scoring system.

Register descriptions were provided, based on which I could read the relevant data from each sensor's register using I2C

## Relevant files
- `main.c`
- `upstair.h`
- Everything under `bitmaps/`
- Everything under `libs/`

Most of the other files were provided by Texas Instruments or by my university. To get my application working, I had to update and fix some of these files. I made some changes to some files under `wireless`. The device had to be able to communicate with other similar devices (send & receive messages).
Also, I updated code in some files under `sensors`. The device had to be able to read data from sensors including temperature, accelometer and gyroscope.

Also, I updated code in some files under `sensors`. The device had to be able to read data from sensors including temperature, accelometer and gyroscope.

Miika ⛱
