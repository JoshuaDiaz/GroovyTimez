/**
/*
 * File:        Groovytimes
 * Author:      The Groovy Katz
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config.h"
// threading library
#include "pt_cornell_1_2_1.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include "songdefs.h"
#include "song_test.h"
#include "sounddefs.h"
#include <math.h>

#define DAC_config_chan_A 0b0011000000000000
#define DAC_config_chan_B 0b1011000000000000
#define EnablePullDownA(bits) CNPUACLR=bits; CNPDASET=bits;
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;
#define dmaChn 0


///// Thread Definitions /////
static struct pt pt_menu, pt_time, pt_calibrate, pt_select, pt_gameplay, pt_input, pt_sound;

///// Game State ///// 
#define menu (char)1
#define calibrate (char)2
#define select (char)3
#define play (char)4
#define end (char)5
//tatic enum state{menu, calibrate, select, play, end};
volatile unsigned char game_state;

///// Inputs /////
int menu_button;
char song_select, song_slide;
char reset;
char port_state;
char star_state;

///// Song Stuff /////
#define MAX_SONGS 5
song playing_song;
song *songs;
static int score_flag=0, miss_flag=0, clap_flag=0, boop_flag=0;

///// String Buffer /////
char buffer[60];
char buffer2[60];

#define sine_table_size_1 256
#define sine_table_size_2 128
#define sine_table_size_3 64
#define sine_table_size_4 32
static short sin_table_1[sine_table_size_1];
static short sin_table_2[sine_table_size_2];
static short sin_table_3[sine_table_size_3];
static short sin_table_4[sine_table_size_4];

///// HELPER FUNCTIONS /////
/** map a move to text **/
inline void move2screenPORT(char move, unsigned short color){
    tft_setTextSize(3);
    tft_setCursor(150,110);
    tft_setTextColor(color);
    switch(move){
        case LEFT: 
            tft_writeString("<-");
            break;
        case UP: 
            tft_writeString("^");
            tft_setCursor(150,130);
            tft_writeString("|");
            break;
        case RIGHT: 
            tft_writeString("->");
            break;
        case DOWN: 
            tft_writeString("|");
            tft_setCursor(150,130);
            tft_writeString("v");
            break;
    }
    return;
}
inline void move2screenSTAR(char move, unsigned short color){
    tft_setTextSize(3);
    tft_setCursor(200,110);
    tft_setTextColor(color);
    switch(move){
        case LEFT: 
            tft_writeString("<-");
            break;
        case UP: 
            tft_writeString("^");
            tft_setCursor(200,140);
            tft_writeString("|");
            break;
        case RIGHT: 
            tft_writeString("->");
            break;
        case DOWN: 
            tft_writeString("|");
            tft_setCursor(200,140);
            tft_writeString("v");
            break;
    }
    return;
}

///**returns 1 if port move scored**/
//inline int didMoveScorePORT(char move){
//    int ret;
//    switch(move){
//        case LEFT: 
//            if(port_state==LEFT) ret = 1;
//            break;
//        case UP: 
//            if(port_state==UP) ret = 1;
//            break;
//        case RIGHT: 
//            if(port_state==RIGHT) ret = 1;
//            break;
//        case DOWN: 
//            if(port_state==DOWN) ret = 1;
//            break;
//    }
//    return ret;
//}
///**returns 1 if starboard move scored**/
//inline int didMoveScoreSTAR(char move){
//    int ret;
//    switch(move){
//        case LEFT: 
//            if(star_state==LEFT) ret = 1;
//            break;
//        case UP: 
//            if(star_state==UP) ret = 1;
//            break;
//        case RIGHT: 
//            if(star_state==RIGHT) ret = 1;
//            break;
//        case DOWN: 
//            if(star_state==DOWN) ret = 1;
//            break;
//    }
//    return ret;
//}
/** Reset all input signals for gameplay to 0 **/
//inline void resetGameplayInput(){
//    up_port=0, down_port=0, left_port=0, right_port=0;
//    up_star=0, down_star=0, left_star=0, right_star=0;
//    return;
//}

/***** Start Menu Thread *****/
static PT_THREAD (protothread_menu(struct pt *pt))
{
    PT_BEGIN(pt);
    static char startBlink = 1;
    
    while(1) {
        startBlink = !startBlink;
        
        tft_fillRoundRect(0,100, 320, 140, 1, ILI9340_BLACK);// x,y,w,h,radius,color

        tft_setTextSize(3);
        tft_setCursor(57,50);
        tft_setTextColor(ILI9340_YELLOW);
        tft_writeString("Groovy Times");
        
        if(startBlink){
            tft_setTextSize(2);
            tft_setCursor(100, 125);
            tft_setTextColor(ILI9340_YELLOW);
            sprintf(buffer,"Press START");
            tft_writeString(buffer); 
        }        
        
        if(menu_button){
            boop_flag = 1;
            tft_fillRoundRect(0, 0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            game_state = calibrate;
            break;
        }
        
        PT_YIELD_TIME_msec(700);   
        
      } // END WHILE(1)
  PT_END(pt);
} // end menu thread

/***** Calibration Screen Thread *****/
static PT_THREAD (protothread_calibrate(struct pt *pt))
{
    PT_BEGIN(pt);
    static int calibration_lvl;
    while(1) {
        
        calibration_lvl++;
        
        tft_fillRoundRect(0, 80, 200, 140, 1, ILI9340_BLACK);// x,y,w,h,radius,color

        if(calibration_lvl < 10){
            tft_setTextSize(2);
            tft_setCursor(75,50);
            tft_setTextColor(ILI9340_YELLOW);
            tft_writeString("CALIBRATING.");
        }
        else if(calibration_lvl < 20){
            tft_setTextSize(2);
            tft_setCursor(75,50);
            tft_setTextColor(ILI9340_YELLOW);
            tft_writeString("CALIBRATING..");
        }
        else if(calibration_lvl < 30){
            tft_setTextSize(2);
            tft_setCursor(75,50);
            tft_setTextColor(ILI9340_YELLOW);
            tft_writeString("CALIBRATING...");
        }
        else if(calibration_lvl >= 40){
            tft_fillRoundRect(0, 0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            tft_setTextSize(2);
            tft_setCursor(55,50);
            tft_setTextColor(ILI9340_YELLOW);
            tft_writeString("TIME TO GROOVE!!!!!");
        }
        if(calibration_lvl >= 44){
            tft_fillRoundRect(0, 0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            calibration_lvl = 0;
            game_state = select;
            break;
        }
        
        PT_YIELD_TIME_msec(50);   
        
      } // END WHILE(1)
  PT_END(pt);
} // end calibrate thread

/***** Song Selection Screen Thread *****/
static PT_THREAD (protothread_select(struct pt *pt))
{
    static char songslide_last = -1;
    #define TRANSITION_TO_GAME_TIME_MSEC 2000
    PT_BEGIN(pt);
    while(1) {
        
        if(song_slide != songslide_last){
            tft_fillRoundRect(0, 0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            boop_flag = 1;
        }
        
        if(song_slide >= 2){
            tft_setTextSize(1);
            tft_setCursor(30,50);
            tft_setTextColor(ILI9340_YELLOW);
            sprintf(buffer, songs[song_slide-2].name);
            tft_writeString(buffer);
        }
        if(song_slide >= 1){
            tft_setTextSize(2);
            tft_setCursor(30,70);
            tft_setTextColor(ILI9340_YELLOW);
            sprintf(buffer, songs[song_slide-1].name);
            tft_writeString(buffer);
        }
        
        tft_setTextSize(3);
        tft_setCursor(30,110);
        tft_setTextColor(ILI9340_YELLOW);
        sprintf(buffer, songs[song_slide].name);
        tft_writeString(buffer);  
        
        if(song_slide < MAX_SONGS-1){
            tft_setTextSize(2);
            tft_setCursor(30,160);
            tft_setTextColor(ILI9340_YELLOW);
            sprintf(buffer, songs[song_slide+1].name);
            tft_writeString(buffer);
        }
        if(song_slide < MAX_SONGS-2){
            tft_setTextSize(1);
            tft_setCursor(30,180);
            tft_setTextColor(ILI9340_YELLOW);
            sprintf(buffer, songs[song_slide+2].name);
            tft_writeString(buffer);
        };
        songslide_last = song_slide;
        
        if(song_select){
            playing_song = songs[song_slide];
            //display transition message
            tft_fillRoundRect(0, 0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            tft_setTextSize(2);
            tft_setCursor(20,40);
            tft_setTextColor(ILI9340_YELLOW);
            tft_writeString("GET READY TO GROOVE TO:");
            tft_setTextSize(3);
            tft_setCursor(50,110);
            tft_setTextColor(ILI9340_YELLOW);
            sprintf(buffer, songs[song_slide].name);
            tft_writeString(buffer);
            
            //reset stuff
            song_select = 0;
            songslide_last = -1;
            
            //wait for transition
            PT_YIELD_TIME_msec(TRANSITION_TO_GAME_TIME_MSEC);
            tft_fillRoundRect(0, 0, 320, 240, 1, ILI9340_BLACK);
            game_state = play;
            break;
        }
        PT_YIELD_TIME_msec(250);   
        
    } // END WHILE(1)
  PT_END(pt);
} // end select thread


static char song_endflag_check;
static int moves_iter;
static char song_port_move, song_star_move;
static int delay;
static int post_move_delay;
static int score;
/***** Gameplay Thread *****/
static PT_THREAD (protothread_gameplay(struct pt *pt))
{

    PT_BEGIN(pt);
    while(1) {
        //display song title
        tft_setTextColor(ILI9340_YELLOW);
        tft_setTextSize(1);
        tft_setCursor(120,10);
        sprintf(buffer, playing_song.name);
        tft_writeString(buffer);
        
        //check if user scored
        if(moves_iter>0){
            //deprecated
//            if(didMoveScorePORT(song_port_move) && didMoveScoreSTAR(song_star_move))
//                score++;
//            resetGameplayInput();       
            if(song_port_move==port_state || song_star_move==star_state){
                score++; 
                score_flag = 1;
                tft_drawCircle(160, 120, 49, ILI9340_GREEN);
                tft_drawCircle(160, 120, 50, ILI9340_GREEN);
                tft_drawCircle(160, 120, 51, ILI9340_GREEN);
                tft_drawCircle(160, 120, 52, ILI9340_GREEN);
                tft_drawCircle(160, 120, 53, ILI9340_GREEN);
            }
            else{
                miss_flag = 1;
                tft_drawLine(209, 167, 111, 69, ILI9340_RED);
                tft_drawLine(111, 167, 209, 69, ILI9340_RED);
                tft_drawLine(209, 168, 111, 70, ILI9340_RED);
                tft_drawLine(111, 168, 209, 70, ILI9340_RED);
                tft_drawLine(209, 169, 111, 71, ILI9340_RED);
                tft_drawLine(111, 169, 209, 71, ILI9340_RED);
                tft_drawLine(209, 170, 111, 72, ILI9340_RED);
                tft_drawLine(111, 170, 209, 72, ILI9340_RED);
                tft_drawLine(209, 171, 111, 73, ILI9340_RED);
                tft_drawLine(111, 171, 209, 73, ILI9340_RED);
            }
        }
        
        //pause for how long the song demands
        PT_YIELD_TIME_msec(post_move_delay);
        
        //clear screen
        tft_fillRoundRect(40, 100, 300, 100, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_drawCircle(160, 120, 49, ILI9340_BLACK);
        tft_drawCircle(160, 120, 50, ILI9340_BLACK);
        tft_drawCircle(160, 120, 51, ILI9340_BLACK);
        tft_drawCircle(160, 120, 52, ILI9340_BLACK);
        tft_drawCircle(160, 120, 53, ILI9340_BLACK);
        tft_drawLine(209, 167, 111, 69, ILI9340_BLACK);
        tft_drawLine(111, 167, 209, 69, ILI9340_BLACK);
        tft_drawLine(209, 168, 111, 70, ILI9340_BLACK);
        tft_drawLine(111, 168, 209, 70, ILI9340_BLACK);
        tft_drawLine(209, 169, 111, 71, ILI9340_BLACK);
        tft_drawLine(111, 169, 209, 71, ILI9340_BLACK);
        tft_drawLine(209, 170, 111, 72, ILI9340_BLACK);
        tft_drawLine(111, 170, 209, 72, ILI9340_BLACK);
        tft_drawLine(209, 171, 111, 73, ILI9340_BLACK);
        tft_drawLine(111, 171, 209, 73, ILI9340_BLACK);

        //if reached end of song flag, do shit
        if(song_endflag_check == -1){  
            //clear screen
            //tft_fillRoundRect(0, 0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            //display score
            tft_setTextSize(3);
            tft_setCursor(95,40);
            tft_setTextColor(ILI9340_YELLOW);
            tft_writeString("Score:");
            tft_setCursor(120,110);
            sprintf(buffer, "%d / %d", score, moves_iter-1);
            tft_writeString(buffer);         
            //reset stuff
            song_endflag_check = 0;
            moves_iter = 0;
            delay = 0;
            score = 0;       
            reset = 0;
            // block till button pressed
            PT_YIELD_UNTIL(pt, reset);
            tft_fillRoundRect(0, 0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            game_state = select;
            PT_RESTART(pt);
            //break;
        }
        
        //DISPLAY SCORE
//        tft_setCursor(20,20);
//        tft_setTextSize(1);
//        tft_setTextColor(ILI9340_YELLOW);
//        sprintf(buffer, "Score: %d", score);
//        tft_writeString(buffer);
        
        
        //set current song data to locals 
        song_port_move = playing_song.m[moves_iter].port;
        song_star_move = playing_song.m[moves_iter].star;
        delay = playing_song.m[moves_iter].time;
        post_move_delay = playing_song.m[moves_iter].post_move_delay;
        song_endflag_check = song_port_move;
        
        //set sound flag for clap
        clap_flag = 1;
        //print moves on screen
        //port
        move2screenPORT(song_port_move, ILI9340_YELLOW);
        //starboard
        //move2screenSTAR(song_star_move, ILI9340_YELLOW);
        
        //increment moves counter
        moves_iter++;
        
        //wait for move to finish
        PT_YIELD_TIME_msec(delay);   
        PT_END(pt);
    } // END WHILE(1)
  
} // end gameplay thread

static int buttstate;
/***** Input Controller Thread *****/
static PT_THREAD (protothread_input(struct pt *pt))
{
    PT_BEGIN(pt);
    static int adc_val;
    static int game_input; //input is ordered as RB3, RB4, RA4, RB9, RB8
    
    //arduino pins, adc, select button
    mPORTASetPinsDigitalIn(BIT_4);
    EnablePullDownA(BIT_4);
    mPORTBSetPinsDigitalIn(BIT_3 | BIT_4 | BIT_9 | BIT_8 | BIT_13 | BIT_7);
    EnablePullDownB(BIT_3 | BIT_4 | BIT_9 | BIT_8 | BIT_7);
    
    adc_val = ReadADC10(0);   // read the result of channel 9 conversion from the idle buffer
    AcquireADC10();
    
    while(1) {
        switch(game_state){
         
            case(menu): 
                menu_button = mPORTBReadBits(BIT_7);
                break;
                
            case(calibrate):
                menu_button = 0;
                break;
                
            case(select):
                adc_val = ReadADC10(0);
                AcquireADC10();
//                tft_fillRoundRect(0, 0, 320,10, 1, ILI9340_BLACK);// x,y,w,h,radius,color
//                tft_setTextSize(1);
//                tft_setCursor(50,20);
//                tft_setTextColor(ILI9340_YELLOW);
//                tft_writeString(buffer);
                song_slide = (char)((float)(MAX_SONGS*adc_val)/1024.0);
                song_select = (char)mPORTBReadBits(BIT_7);

                break;
                
            case(play):
                if(buttstate && !mPORTBReadBits(BIT_7)){
                    buttstate = 0;
                    reset =1;
                }
                else if(mPORTBReadBits(BIT_7)) buttstate =1;
                
                game_input = 0; //input is ordered as RB3, RB4, RA4, RB9, RB8
                if(mPORTBReadBits(BIT_3)) game_input |= 0b10000;
                if(mPORTBReadBits(BIT_4)) game_input |= 0b01000;
                if(mPORTAReadBits(BIT_4)) game_input |= 0b00100;
                if(mPORTBReadBits(BIT_9)) game_input |= 0b00010;
                if(mPORTBReadBits(BIT_8)) game_input |= 0b00001;
                switch(game_input){
                    case(0): 
                        port_state = NONE; star_state=NONE;
                        sprintf(buffer2, "input: X ");
                        break;
                    case(1): 
                        port_state = LEFT; star_state=NONE;
                        sprintf(buffer2, "input: < ");
                        break;
                    case(2): 
                        port_state = UP; star_state=NONE;
                        sprintf(buffer2, "input: ^ ");
                        break;
                    case(3): 
                        port_state = RIGHT; star_state=NONE;
                        sprintf(buffer2, "input: > ");
                        break;
                    case(4): 
                        port_state = DOWN; star_state=NONE;
                        sprintf(buffer2, "input: V ");
                        break;    
                    case(5): 
                        port_state = NONE; star_state=LEFT;
                        sprintf(buffer2, "input: 5");
                        break;
                    case(6): 
                        port_state = LEFT; star_state=LEFT;
                        sprintf(buffer2, "input: <  <");
                        break;
                    case(7): 
                        port_state = UP; star_state=LEFT;
                        sprintf(buffer2, "input: ^  <");
                        break;
//                    case(8): 
//                        port_state = RIGHT; star_state=LEFT;
//                        sprintf(buffer, "input: >  <");
//                        break;
//                    case(9): 
//                        port_state = DOWN; star_state=LEFT;
//                        sprintf(buffer, "input: v  <");
//                        break;
//                    case(10): 
//                        port_state = NONE; star_state=UP;
//                        sprintf(buffer, "input: X  ^");
//                        break;
//                    case(11): 
//                        port_state = LEFT; star_state=UP;
//                        sprintf(buffer, "input: <  ^");
//                        break;
//                    case(12): 
//                        port_state = UP; star_state=UP;
//                        sprintf(buffer, "input: ^  ^");
//                        break;
//                    case(13): 
//                        port_state = RIGHT; star_state=UP;
//                        sprintf(buffer, "input: >  ^");
//                        break;
//                    case(14): 
//                        port_state = DOWN; star_state=UP;
//                        sprintf(buffer, "input: v  ^");
//                        break;
//                    case(15): 
//                        port_state = NONE; star_state=RIGHT;
//                        sprintf(buffer, "input: X  >");
//                        break;
//                    case(16): 
//                        port_state = LEFT; star_state=RIGHT;
//                        sprintf(buffer, "input: <  >");
//                        break;
//                    case(17): 
//                        port_state = UP; star_state=RIGHT;
//                        sprintf(buffer, "input: ^  >");
//                        break;
//                    case(18): 
//                        port_state = RIGHT; star_state=RIGHT;
//                        sprintf(buffer, "input: >  >");
//                        break;
//                    case(19): 
//                        port_state = DOWN; star_state=RIGHT;
//                        sprintf(buffer, "input: v  >");
//                        break;
//                    case(20): 
//                        port_state = NONE; star_state=DOWN;
//                        sprintf(buffer, "input: X  V");
//                        break;
//                    case(21): 
//                        port_state = LEFT; star_state=DOWN;
//                        sprintf(buffer, "input: <  V");
//                        break;
//                    case(22): 
//                        port_state = UP; star_state=DOWN;
//                        sprintf(buffer, "input: ^  V");
//                        break;
//                    case(23): 
//                        port_state = RIGHT; star_state=DOWN;
//                        sprintf(buffer, "input: >  V");
//                        break;
//                    case(24): 
//                        port_state = DOWN; star_state=DOWN;
//                        sprintf(buffer, "input: V  V");
//                        break;                                    
                }
                tft_fillRoundRect(180, 170, 50, 50, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                tft_setCursor(120,170);
                tft_setTextSize(2);
                tft_setTextColor(ILI9340_GREEN);
                tft_writeString(buffer2);
//                if(!left_port)left_port = mPORTAReadBits(BIT_1);
//                if(!up_port)up_port = mPORTAReadBits(BIT_2);
//                if(!down_port)down_port = mPORTAReadBits(BIT_3);
//                if(!right_port)right_port = mPORTAReadBits(BIT_4);
//                if(!left_star)left_star = mPORTBReadBits(BIT_10);
//                if(!up_star)up_star = mPORTBReadBits(BIT_9);
//                if(!down_star)down_port = mPORTBReadBits(BIT_8);
//                if(!right_star)right_star = mPORTBReadBits(BIT_7);
                break;               
            case(end):
                break;
            default: break;
        }
        
        PT_YIELD_TIME_msec(50);   
        
      } // END WHILE(1)
  PT_END(pt);
} // end input thread

int iter;
/** SOUND THREAD **/
static PT_THREAD (protothread_sound(struct pt *pt))
{
   PT_BEGIN(pt);
   static unsigned int score_timer;
   PT_YIELD_UNTIL(pt, (score_flag || miss_flag || clap_flag || boop_flag));
    while(1) {
        if(score_flag){
            DmaChnDisable(dmaChn);
            WriteTimer3(0x0);
            DmaChnSetTxfer(dmaChn, sin_table_1, (void*)&SPI2BUF, sine_table_size_1, 2, 2); // DMA sends data to SPI2
            PT_YIELD_TIME_msec(7);
            DmaChnEnable(dmaChn);
            score_flag = 0;
        }
        if(miss_flag){
            DmaChnDisable(dmaChn);
            WriteTimer3(0x0);
            DmaChnSetTxfer(dmaChn, sin_table_2, (void*)&SPI2BUF, sine_table_size_2, 2, 2); // DMA sends data to SPI2
            PT_YIELD_TIME_msec(7);
            DmaChnEnable(dmaChn);
            miss_flag = 0;
        }
        if(clap_flag){         
            DmaChnDisable(dmaChn);
            WriteTimer3(0x0);
            DmaChnSetTxfer(dmaChn, sin_table_3, (void*)&SPI2BUF, sine_table_size_3, 2, 2); // DMA sends data to SPI2
            PT_YIELD_TIME_msec(7);
            DmaChnEnable(dmaChn);
            clap_flag = 0;
        }
        if(boop_flag){         
            DmaChnDisable(dmaChn);
            WriteTimer3(0x0);
            DmaChnSetTxfer(dmaChn, sin_table_4, (void*)&SPI2BUF, sine_table_size_4, 2, 2); // DMA sends data to SPI2
            PT_YIELD_TIME_msec(7);
            DmaChnEnable(dmaChn);
            boop_flag = 0;
        }   
        score_timer = ReadTimer3();
        if(score_timer > 50000){ // Set pre-scaler = 64, 1Sec = 625000
             DmaChnDisable(dmaChn); // Stop the tone after 1 second
             PT_RESTART(pt);
        }
        PT_YIELD_TIME_msec(5); // prevent CPU hogging
        //DmaChnDisable(dmaChn);
        //break;

    }
       //if (end_flag==1)PT_END(pt);
       //else 
   PT_END(pt);
}// end sound thread

/***** System Clock *****/
//static PT_THREAD (protothread_time(struct pt *pt))
//{
//    PT_BEGIN(pt);
//    static int sys_time;
//    while(1) {
//        PT_YIELD_TIME_msec(1000);
//        
//        sys_time++;
//        
//        tft_fillRoundRect(0,12, 240, 12, 1, ILI9340_BLACK);
//        
//        tft_setTextSize(2);
//        tft_setCursor(5,5);
//        tft_setTextColor(ILI9340_YELLOW);
//        sprintf(buffer,"%d", sys_time);
//        tft_writeString(buffer);
//        
//        PT_YIELD_TIME_msec(1000) ;   
//        
//      } // END WHILE(1)
//  PT_END(pt);
//} // system clock

// === Main  ======================================================

/***** Main Method *****/
int main(void)
{
    ///// SONG STUFF /////
    songs = (song[MAX_SONGS]){song0, song1, song2, song3, song4};
    
    ///// Setup ADC on AN11 = RB13 /////
    ANSELA = 0; ANSELB = 0; 
    CloseADC10();	// ensure the ADC is off before setting the configuration

	// define setup parameters for OpenADC10
	// Turn module on | output in integer | trigger mode auto | enable autosample
        // ADC_CLK_AUTO -- Internal counter ends sampling and starts conversion (Auto convert)
        // ADC_AUTO_SAMPLING_ON -- Sampling begins immediately after last conversion completes; SAMP bit is automatically set
        // ADC_AUTO_SAMPLING_OFF -- Sampling begins with AcquireADC10();
        #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF //

        // ADC ref external  | disable offset test | disable scan mode | do 1 sample | use single buf | alternate mode off
        #define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF

        // use peripherial bus clock | set sample time | set ADC clock divider
        // ADC_CONV_CLK_Tcy2 means divide CLK_PB by 2 (max speed)
        // ADC_SAMPLE_TIME_5 seems to work with a source resistance < 1kohm
        #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_5 | ADC_CONV_CLK_Tcy2 //ADC_SAMPLE_TIME_15| ADC_CONV_CLK_Tcy2

        // set AN11 and  as analog inputs
        #define PARAM4	ENABLE_AN11_ANA // pin 24

        // do not assign channels to scan
        #define PARAM5	SKIP_SCAN_ALL

	// use ground as negative ref for A | use AN11 for input A     
	// configure to sample AN11 
	SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11 ); // configure to sample AN11 
	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above

    // Enable the ADC
	EnableADC10(); 
    
    
    //insantiate sine tables for sound effects
    int j;
    for (j = 0; j < sine_table_size_1; j++){
        sin_table_1[j] = (unsigned short)(1023*sin((float)j*6.283/(float)sine_table_size_1)+1024)|DAC_config_chan_A;
    }
    int i;
    for (i = 0; i < sine_table_size_2; i++){
        sin_table_2[i] = (unsigned short)(1023*sin((float)i*6.283/(float)sine_table_size_2)+1024)|DAC_config_chan_A;
    }
    for (i = 0; i < sine_table_size_3; i++){
        sin_table_3[i] = (unsigned short)(1023*sin((float)i*6.283/(float)sine_table_size_3)+1024)|DAC_config_chan_A;
    }
    for (i = 0; i < sine_table_size_4; i++){
        sin_table_4[i] = (unsigned short)(1023*sin((float)i*6.283/(float)sine_table_size_4)+1024)|DAC_config_chan_A;
    }
    
    //Setup SPI for DAC
    SpiChnOpen(SPI_CHANNEL2, 
    SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV | SPICON_FRMEN | SPICON_FRMPOL, 2);
    PPSOutput(2, RPB5, SDO2);
    PPSOutput(4, RPB10, SS2);
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 909);
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_64, 0xffff);
    
    // Control CS for DAC
    DmaChnOpen(dmaChn, 0, DMA_OPEN_AUTO); // Auto mode sets DMA to play indefinitely
    //DmaChnOpen(dmaChn, 0, DMA_OPEN_DEFAULT); // Auto mode sets DMA to play indefinitely
    DmaChnSetTxfer(dmaChn, sin_table_1, (void*)&SPI2BUF, sine_table_size_1*2, 2, 2); // DMA sends data to SPI2
    DmaChnSetEventControl(dmaChn, DMA_EV_START_IRQ(_TIMER_2_IRQ));
    
    
    ///// Setup TFT /////
    tft_init_hw();
    tft_begin();
    tft_setRotation(1);
    tft_fillScreen(ILI9340_BLACK); //erase screen

    ///// Setup ProtoThreads /////
    PT_setup();
    INTEnableSystemMultiVectoredInt();

    ///// Initialize and Schedule Threads
    PT_INIT(&pt_menu);
    PT_INIT(&pt_calibrate);
    PT_INIT(&pt_input);
    PT_INIT(&pt_select);
    PT_INIT(&pt_gameplay);
    PT_INIT(&pt_sound);
    //PT_INIT(&pt_time);

    game_state = menu;
    while(1) {
        PT_SCHEDULE(protothread_input(&pt_input));
        PT_SCHEDULE(protothread_sound(&pt_sound));
        if(game_state == menu)
            PT_SCHEDULE(protothread_menu(&pt_menu));
        else if(game_state == calibrate) 
            PT_SCHEDULE(protothread_calibrate(&pt_calibrate));
        else if(game_state == select) 
            PT_SCHEDULE(protothread_select(&pt_select));
        else if(game_state == play)
            PT_SCHEDULE(protothread_gameplay(&pt_gameplay));
        //PT_SCHEDULE(protothread_time(&pt_time));
  }
} // main


