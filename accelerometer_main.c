/*
 * File:        TFT_test_BRL4.c
 * Author:      Bruce Land
 * For use with Sean Carroll's Big Board
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config.h"
// threading library
#include "pt_cornell_1_2_1.h"

////////////////////////////////////
//#include "i2c_helper.h"

#define EnablePullDownA(bits) CNPUACLR=bits; CNPDASET=bits;
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;


/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480
  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// string buffer
char buffer[100];


// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_i2c, pt_direction, pt_calibrate, pt_wait;

// system 1 second interval tick
int sys_time_seconds ;

static signed int xAccelPort, yAccelPort, zAccelPort;
static signed int xAccelStar, yAccelStar, zAccelStar;
static char xAccelMSB, yAccelMSB, zAccelMSB;
//static char xAccelLSB, yAccelLSB, zAccelLSB;

unsigned char getDataPort(unsigned char axis) {
    unsigned char ddata;
    StartI2C1(); //Start
    IdleI2C1();
    MasterWriteI2C1(0x3A); //Send Device Address (Write)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    MasterWriteI2C1(axis); //Send Register Address
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    RestartI2C1(); //Restart
    IdleI2C1();
    MasterWriteI2C1(0x3B); //Send Device Address (Read)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    ddata = MasterReadI2C1();
    IdleI2C1();
    StopI2C1();
    IdleI2C1();
    return ddata;
}

unsigned char getDataStar(unsigned char axis) {
    unsigned char ddata;
    StartI2C1(); //Start
    IdleI2C1();
    MasterWriteI2C1(0x38); //Send Device Address (Write)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    MasterWriteI2C1(axis); //Send Register Address
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    RestartI2C1(); //Restart
    IdleI2C1();
    MasterWriteI2C1(0x39); //Send Device Address (Read)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    ddata = MasterReadI2C1();
    IdleI2C1();
    StopI2C1();
    IdleI2C1();
    return ddata;
}

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_i2c(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        //float values[3];
        //readImuValues(values);
        
        xAccelMSB = (getDataPort(0x01));
        //xAccelLSB = (getData(0x02));
        yAccelMSB = (getDataPort(0x03));
        //yAccelLSB = (getData(0x04));
        zAccelMSB = (getDataPort(0x05));
        //zAccelLSB = (getData(0x06));
        
        xAccelPort = (signed int)((xAccelMSB));
        yAccelPort = (signed int)((yAccelMSB));
        zAccelPort = (signed int)((zAccelMSB));
        
        xAccelMSB = (getDataStar(0x01));
        //xAccelLSB = (getData(0x02));
        yAccelMSB = (getDataStar(0x03));
        //yAccelLSB = (getData(0x04));
        zAccelMSB = (getDataStar(0x05));
        //zAccelLSB = (getData(0x06));
        
        xAccelStar = (signed int)((xAccelMSB));
        yAccelStar = (signed int)((yAccelMSB));
        zAccelStar = (signed int)((zAccelMSB));
        
        PT_YIELD_TIME_msec(100);
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

static int x_up, x_down, x_left, x_right;
static int y_up, y_down, y_left, y_right;
static int z_up, z_down, z_left, z_right;
static int direction_port, direction_star;
static enum direction{UP,DOWN,LEFT,RIGHT,NONE};

static int flag = 0;
static int i;
static int flag2, time;

static enum state{release, maybe_push, push, maybe_release};
static enum state keypad_state = release;
static enum state flag_state = release;
static int next_flag;

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_direction(struct pt *pt))
{
    PT_BEGIN(pt);
    x_up = 0;
    y_up = -64;
    z_up = 0;
    
    x_down = 0;
    y_down = 64;
    z_down = 0;
    
    x_left = 60;
    y_left = 0;
    z_left = 0;
    
    x_right = -65;
    y_right = 5;
    z_right = -5;
    
    int thresh = 30;
    
    int x_up_cond_port, y_up_cond_port, z_up_cond_port;
    int x_down_cond_port, y_down_cond_port, z_down_cond_port;
    int x_left_cond_port, y_left_cond_port, z_left_cond_port;
    int x_right_cond_port, y_right_cond_port, z_right_cond_port;
    
    int x_up_cond_star, y_up_cond_star, z_up_cond_star;
    int x_down_cond_star, y_down_cond_star, z_down_cond_star;
    int x_left_cond_star, y_left_cond_star, z_left_cond_star;
    int x_right_cond_star, y_right_cond_star, z_right_cond_star;
    mPORTBClearBits(BIT_0 | BIT_1 | BIT_2); //000
    
      while(1) {
          
        
        
        //UP conditionals
        x_up_cond_port = (xAccelPort > (x_up-thresh))&(xAccelPort < (x_up+thresh));
        y_up_cond_port = (yAccelPort > (y_up-thresh))&(yAccelPort < (y_up+thresh));
        z_up_cond_port = (zAccelPort > (z_up-thresh))&(zAccelPort < (z_up+thresh));
        
        //DOWN conditionals
        x_down_cond_port = (xAccelPort > (x_down-thresh))&(xAccelPort < (x_down+thresh));
        y_down_cond_port = (yAccelPort > (y_down-thresh))&(yAccelPort < (y_down+thresh));
        z_down_cond_port = (zAccelPort > (z_down-thresh))&(zAccelPort < (z_down+thresh));
        
        //LEFT conditionals
        x_left_cond_port = (xAccelPort > (x_left-thresh))&(xAccelPort < (x_left+thresh));
        y_left_cond_port = (yAccelPort > (y_left-thresh))&(yAccelPort < (y_left+thresh));
        z_left_cond_port = (zAccelPort > (z_left-thresh))&(zAccelPort < (z_left+thresh));
        
        //RIGHT conditionals
        x_right_cond_port = (xAccelPort > (x_right-thresh))&(xAccelPort < (x_right+thresh));
        y_right_cond_port = (yAccelPort > (y_right-thresh))&(yAccelPort < (y_right+thresh));
        z_right_cond_port = (zAccelPort > (z_right-thresh))&(zAccelPort < (z_right+thresh));
        
        //UP conditionals
        x_up_cond_star = (xAccelStar > (x_up-thresh))&(xAccelStar < (x_up+thresh));
        y_up_cond_star = (yAccelStar > (y_up-thresh))&(yAccelStar < (y_up+thresh));
        z_up_cond_star = (zAccelStar > (z_up-thresh))&(zAccelStar < (z_up+thresh));
        
        //DOWN conditionals
        x_down_cond_star = (xAccelStar > (x_down-thresh))&(xAccelStar < (x_down+thresh));
        y_down_cond_star = (yAccelStar > (y_down-thresh))&(yAccelStar < (y_down+thresh));
        z_down_cond_star = (zAccelStar > (z_down-thresh))&(zAccelStar < (z_down+thresh));
        
        //LEFT conditionals
        x_left_cond_star = (xAccelStar > (x_left-thresh))&(xAccelStar < (x_left+thresh));
        y_left_cond_star = (yAccelStar > (y_left-thresh))&(yAccelStar < (y_left+thresh));
        z_left_cond_star = (zAccelStar > (z_left-thresh))&(zAccelStar < (z_left+thresh));
        
        //RIGHT conditionals
        x_right_cond_star = (xAccelStar > (x_right-thresh))&(xAccelStar < (x_right+thresh));
        y_right_cond_star = (yAccelStar > (y_right-thresh))&(yAccelStar < (y_right+thresh));
        z_right_cond_star = (zAccelStar > (z_right-thresh))&(zAccelStar < (z_right+thresh));
        
        
        if(x_up_cond_port & y_up_cond_port & z_up_cond_port){
            direction_port = UP; 
            mPORTAClearBits(BIT_2 | BIT_3 | BIT_4); //000
            //mPORTBClearBits(BIT_0 | BIT_1 | BIT_2); //000
        }
        else if(x_down_cond_port & y_down_cond_port & z_down_cond_port){
            direction_port = DOWN; 
            mPORTASetBits(BIT_2);
            mPORTAClearBits(BIT_3 | BIT_4);//001
            //mPORTBSetBits(BIT_0);
            //mPORTBClearBits(BIT_1 | BIT_2);//001
        }
        else if(x_left_cond_port & y_left_cond_port & z_left_cond_port){
            direction_port = LEFT; 
            mPORTASetBits(BIT_3);
            mPORTAClearBits(BIT_2 | BIT_4); //010
            //mPORTBSetBits(BIT_1);
            //mPORTBClearBits(BIT_0 | BIT_2); //010
        }
        else if(x_right_cond_port & y_right_cond_port & z_right_cond_port){
            direction_port = RIGHT;
            mPORTASetBits(BIT_2 | BIT_3);
            mPORTAClearBits(BIT_4); //011
            //mPORTBSetBits(BIT_0 | BIT_1);
            //mPORTBClearBits(BIT_2); //011
        }
        else {
            direction_port = NONE;
            mPORTASetBits(BIT_4); 
            mPORTAClearBits(BIT_2 | BIT_3); //100
            //mPORTBSetBits(BIT_2); 
            //mPORTBClearBits(BIT_0 | BIT_1); //100
        }
        
        if(x_up_cond_star & y_up_cond_star & z_up_cond_star){
            direction_star = UP; 
            mPORTBClearBits(BIT_0 | BIT_1 | BIT_2); //000
        }
        else if(x_down_cond_star & y_down_cond_star & z_down_cond_star){
            direction_star = DOWN; 
            mPORTBSetBits(BIT_0);
            mPORTBClearBits(BIT_1 | BIT_2);//001
        }
        else if(x_left_cond_star & y_left_cond_star & z_left_cond_star){
            direction_star = LEFT; 
            mPORTBSetBits(BIT_1);
            mPORTBClearBits(BIT_0 | BIT_2); //010
        }
        else if(x_right_cond_star & y_right_cond_star & z_right_cond_star){
            direction_star = RIGHT;
            mPORTBSetBits(BIT_0 | BIT_1);
            mPORTBClearBits(BIT_2); //011
        }
        else {
            direction_star = NONE;
            mPORTBSetBits(BIT_2); 
            mPORTBClearBits(BIT_0 | BIT_1); //100
        }
        
        
        next_flag = mPORTBReadBits(BIT_10);
        switch(flag_state){
            case release:
                if (next_flag == 0){
                    flag_state = release;
                    flag = 0;
                }
                else{
                    flag_state = maybe_push;
                    flag = 0;
                }
               
                break;
                
            case maybe_push:
                if (next_flag == 0){
                    flag_state = release;
                    flag = 0;
                }
                else{
                    flag_state = push;
                    flag = 1;
                }
                break;
                
            case push:
                //put in play tone logic
                if (next_flag == 0){
                    flag_state = maybe_release;
                    flag = 0;
                }
                else{
                    flag_state = push;
                    flag = 0;
                }
                break;
                
            case maybe_release:
                if (next_flag == 0){
                    flag_state = release;  
                    flag = 0;
                }
                else{
                    flag_state = push;
                    flag = 0;
                }
                break;
        }
        
        
        i = 0; //very important for resetting calibrate
        
        PT_YIELD_TIME_msec(50);
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
}

static PT_THREAD (protothread_calibrate(struct pt *pt))
{
    PT_BEGIN(pt);
    flag = 0;
    int next;
    
      while(1) {

        next = 0;
        PT_YIELD_TIME_msec(50);
        next = mPORTBReadBits(BIT_10);
        switch(keypad_state){
            case release:
                if (next == 0){
                    keypad_state = release;
                }
                else{
                    keypad_state = maybe_push;
                }
               
                break;
                
            case maybe_push:
                if (next == 0){
                    keypad_state = release;    
                }
                else{
                    keypad_state = push;
                    i+=1;
                }
                break;
                
            case push:
                //put in play tone logic
                if (next == 0){
                    keypad_state = maybe_release;
                }
                else{
                    keypad_state = push;
                }
                break;
                
            case maybe_release:
                if (next == 0){
                    keypad_state = release;                    
                }
                else{
                    keypad_state = push;
                }
                break;
        }
        if(i==0){
            i=1;
        }
        else if(i==1){
            mPORTASetBits(BIT_2 | BIT_3 | BIT_4); //111
            x_up = xAccelPort;
            y_up = yAccelPort;
            z_up = zAccelPort;
        }
        else if(i==2){
            mPORTASetBits(BIT_3 | BIT_4); //110
            mPORTAClearBits(BIT_2); //000
            x_down = xAccelPort;
            y_down = yAccelPort;
            z_down = zAccelPort;
        }
        else if(i==3){
            mPORTASetBits(BIT_2 | BIT_4); //101
            mPORTAClearBits(BIT_3); //000
            x_left = xAccelPort;
            y_left = yAccelPort;
            z_left = zAccelPort;
        }
        else if(i==4){
            mPORTASetBits(BIT_4); //100
            mPORTAClearBits(BIT_2 | BIT_3); //000
            x_right = xAccelPort;
            y_right = yAccelPort;
            z_right = zAccelPort;  
        }
        else{
            flag = 0;
        }
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

    mPORTBSetPinsDigitalIn(BIT_10);
    EnablePullDownB(BIT_10);
    
    mPORTASetPinsDigitalOut(BIT_2 | BIT_3 | BIT_4);
    mPORTBSetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2);
    
  OpenI2C1(I2C_ON, 0x0C2);
  IdleI2C1();
  
  StartI2C1(); //Start
  IdleI2C1();
  
  MasterWriteI2C1(0x3A); //Send Device Address (Write)
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  
  MasterWriteI2C1(0x2A); //Send Register Address
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  
  MasterWriteI2C1(0x03); //Send Write Data
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  StopI2C1();
  
  MasterWriteI2C1(0x38); //Send Device Address (Write)
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  
  MasterWriteI2C1(0x2A); //Send Register Address
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  
  MasterWriteI2C1(0x03); //Send Write Data
  IdleI2C1();
  while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
  StopI2C1();
    
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_i2c);
  PT_INIT(&pt_direction);
  PT_INIT(&pt_calibrate);

  
  // round-robin scheduler for threads
  while (1){
        if(flag==0){
            PT_SCHEDULE(protothread_direction(&pt_direction));
            
        }
        else{
            PT_SCHEDULE(protothread_calibrate(&pt_calibrate));
        }
        
        
        PT_SCHEDULE(protothread_i2c(&pt_i2c));
  } // main
}
// === end  ======================================================
