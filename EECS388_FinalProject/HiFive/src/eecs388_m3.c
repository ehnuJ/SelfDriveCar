#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "eecs388_lib.h"
#include "metal/i2c.h"

struct metal_i2c *i2c;
uint8_t bufWrite[5];
uint8_t bufRead[1];
volatile int g_angle;
volatile int g_speed;

//The entire setup sequence
void set_up_I2C()
{
    uint8_t oldMode;
    uint8_t newMode;
    _Bool success;

    bufWrite[0] = PCA9685_MODE1;
    bufWrite[1] = MODE1_RESTART;
    printf("%d\n",bufWrite[0]);
    
    i2c = metal_i2c_get_device(0);

    if(i2c == NULL){
        printf("Connection Unsuccessful\n");
    }
    else{
        printf("Connection Successful\n");
    }
    
    //Setup Sequence
    metal_i2c_init(i2c,I2C_BAUDRATE,METAL_I2C_MASTER);
    success = metal_i2c_write(i2c,PCA9685_I2C_ADDRESS,2,bufWrite,METAL_I2C_STOP_DISABLE);//reset
    delay(100);
    printf("resetting PCA9685 control 1\n");

    //Initial Read of control 1
    bufWrite[0] = PCA9685_MODE1;//Address
    success = metal_i2c_transfer(i2c,PCA9685_I2C_ADDRESS,bufWrite,1,bufRead,1);//initial read
    printf("Read success: %d and control value is: %d\n", success, bufWrite[0]);
    
    //Configuring Control 1
    oldMode = bufRead[0];
    newMode = (oldMode & ~MODE1_RESTART) | MODE1_SLEEP;
    printf("sleep setting is %d\n", newMode);
    bufWrite[0] = PCA9685_MODE1;//address
    bufWrite[1] = newMode;//writing to register
    success = metal_i2c_write(i2c,PCA9685_I2C_ADDRESS,2,bufWrite,METAL_I2C_STOP_DISABLE);//sleep
    bufWrite[0] = PCA9685_PRESCALE;//Setting PWM prescale
    bufWrite[1] = 0x79;
    success = metal_i2c_write(i2c,PCA9685_I2C_ADDRESS,2,bufWrite,METAL_I2C_STOP_DISABLE);//sets prescale
    bufWrite[0] = PCA9685_MODE1;
    bufWrite[1] = 0x01 | MODE1_AI | MODE1_RESTART;
    printf("on setting is %d\n", bufWrite[1]);
    success = metal_i2c_write(i2c,PCA9685_I2C_ADDRESS,2,bufWrite,METAL_I2C_STOP_DISABLE);//awake
    delay(100);
    printf("Setting the control register\n");
    bufWrite[0] = PCA9685_MODE1;
    success = metal_i2c_transfer(i2c,PCA9685_I2C_ADDRESS,bufWrite,1,bufRead,1);//initial read
    printf("Set register is %d\n",bufRead[0]);
} 

void breakup(int bigNum, uint8_t* low, uint8_t* high)
{
    *low = bigNum & 0xFF;
    *high = (bigNum >> 8) & 0xFF;
}

void steering(int angle)
{
    int ServoCycle = getServoCycle(angle);
    breakup(ServoCycle , &bufWrite[3], &bufWrite[4]);

    bufWrite[0] = PCA9685_LED0_ON_L;
    bufWrite[1] = 0;
    bufWrite[2] = 0;

    metal_i2c_transfer(i2c,PCA9685_I2C_ADDRESS,bufWrite,5,bufRead,1);


}

void stopMotor()
{
    int MotorCycle = 280;
    breakup(MotorCycle , &bufWrite[3], &bufWrite[4]);

    bufWrite[0] = PCA9685_LED1_ON_L;
    bufWrite[1] = 0;
    bufWrite[2] = 0;
    metal_i2c_transfer(i2c,PCA9685_I2C_ADDRESS,bufWrite,5,bufRead,1);

}

void driveForward(uint8_t speedFlag)
{
    int MotorCycle;
    if (speedFlag == 1) {MotorCycle = 313;}
    if (speedFlag == 2) {MotorCycle = 315;}
    if (speedFlag == 3) {MotorCycle = 317;}
     
    breakup(MotorCycle , &bufWrite[3], &bufWrite[4]);

    bufWrite[0] = PCA9685_LED1_ON_L;
    bufWrite[1] = 0;
    bufWrite[2] = 0;
    metal_i2c_transfer(i2c,PCA9685_I2C_ADDRESS,bufWrite,5,bufRead,1);

}

void driveReverse(uint8_t speedFlag)
{
    int MotorCycle;
    if (speedFlag == 1) {MotorCycle = 267;}
    if (speedFlag == 2) {MotorCycle = 265;}
    if (speedFlag == 3) {MotorCycle = 263;}
    breakup(MotorCycle , &bufWrite[3], &bufWrite[4]);

    bufWrite[0] = PCA9685_LED1_ON_L;
    bufWrite[1] = 0;
    bufWrite[2] = 0;
    metal_i2c_transfer(i2c,PCA9685_I2C_ADDRESS,bufWrite,5,bufRead,1);

}

void raspberrypi_int_handler(int devid)
{
    char * str = malloc(40 * sizeof(char));
    char * test_string = "Speed: ";
    
    ser_readline(devid, 20 , str);
    
    int similar = strncmp(test_string , str , 6) ;
    if (similar == 0){
        sscanf(str+16 , "%d" , &g_angle);
        sscanf(str+7 , "%d" , &g_speed);
        printf(str);
    };

    
    // g_angle => readline from UART0
    free(str);
}


int main()
{
    // Initialize I2C
    set_up_I2C();
    delay(2000);

    // Calibrate Motor
    printf("Calibrate Motor.\n");
    stopMotor();
    delay(2000);

    // initialize UART channels
    ser_setup(1); // uart0 (receive from raspberry pi)
    ser_setup(0); // uart0 (send to raspberry pi)

    printf("Setup completed.\n");
    printf("Begin the main loop.\n");
    
    // Initialize global angle
    g_angle = 0;
    g_speed = 0;
    // Drive loop
    while (1) {
        if (ser_isready(1)){

            raspberrypi_int_handler(1);

            steering(g_angle);
            if (g_speed == 0){stopMotor(); ser_printline(0, "STOP");}
            else if (g_speed == 1) { driveForward(1); ser_printline(0, "FORWARD");}  
            else if (g_speed == 2) { driveReverse(1); ser_printline(0, "REVERSE");}
        }
    }
    return 0;
}
