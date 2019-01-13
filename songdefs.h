/* 
 * File:   songdefs.h
 * Author: GroovyKatz
 *
 * Created on November 7, 2018, 5:37 PM
 */

#ifndef SONGDEFS_H
#define	SONGDEFS_H

#define NONE (char)0
#define LEFT (char)1
#define UP (char)2
#define RIGHT (char)3
#define DOWN (char)4
#define MAX_MOVEZ 30

typedef struct movez{
    char port;
    char star;
    int time;
    int post_move_delay;
}movez;

typedef struct song{
    char *name;
    movez *m;
    const unsigned char *song_data; 
}song;

#endif	/* SONGDEFS_H */

