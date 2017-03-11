/*  
    Decoding BTC4 data in file to ram
    file format has (see inside of blender export scripts) : 

    - u32 event_id, frame_id
    - u32 width, height
	- palette of 256 u16 
    - btc frame as 1 u32 for each 4x4 block (ie size=w*h/4)
    - u8 sound[1066]
    - padding up to next 512B

*/


#include <stdint.h>
#include "kernel/bitbox.h"
#include <string.h>
#include "fatfs/ff.h"

#ifndef EMULATOR
#include "stm32f4xx.h"
#endif

#define VIDEO_FILENAME "frames.ani"

//#define DEBUG
    
// Dual buffer ; should be sufficient
// x*y/4 + 4*2 (event) +4*2 (size) + 512 (palette) + 1066 audio 
#define BUFSIZE WIDTH*HEIGHT/4+512+8+8 + 1066
#define BUFSIZE_PADDED (512*((BUFSIZE+511)/512))
#define WIDTH 400
#define HEIGHT 300

enum VideoEvent {evt_nothing,evt_a,evt_up,evt_down,evt_left,evt_right,evt_always=100};

enum VideoEvent evt_name;
int evt_frame;

uint32_t buffer1[BUFSIZE_PADDED/4];
uint32_t buffer2[BUFSIZE_PADDED/4];

uint32_t *buffer_disp=buffer1;
uint32_t *buffer_load=buffer2;

FATFS fatfs;
FIL video_file;
FRESULT result;

uint16_t previous_gamepad;
#define PUSHED(key) (gamepad_buttons[0] & ~previous_gamepad & gamepad_##key) 

void game_init() 
{
	result = f_mount(&fatfs,"",1); //mount now
	if (result==FR_OK) result = f_open (&video_file,VIDEO_FILENAME,FA_READ); // open file	

	if (result==FR_OK) message ("ok, mounted & file opened ok\n");	
	// dummy, empty frame
	// XXX crash of not the exact size of the movie ?? load a little header
	
	message("size of buffer is %d, padded to %d\n",BUFSIZE, BUFSIZE_PADDED);
}

volatile int t_tot; // for all frames


void jumpto_frame(int frame)
{
	message("jumped to %d!\n",frame);
	f_lseek(&video_file, frame*BUFSIZE_PADDED);
}

void game_frame() 
{
	size_t bytes_read;
	unsigned int bytes_to_read;
	static uint32_t *dst;

	// every 4 frame (15fps fixed) 
	// we can't use vga_frame here since vga_frame is not SURE to be increased by one 
	// sometimes if we're too slow we can skip a vga frame  
	static int read_frame;
	
	if (read_frame%4==0) {

		// exchange frame buffers
		uint32_t *tmp = (uint32_t *)buffer_load; 
		buffer_load=buffer_disp; 
		buffer_disp=tmp;

		// setup new buffer for display
		evt_name = *buffer_disp;
		evt_frame = *(buffer_disp+1);


		// / 4 because of 2x video
		bytes_to_read = BUFSIZE_PADDED; // always a multiple of 512

		bytes_to_read -= 3*8192;
		dst = buffer_load;

	} else {
		bytes_to_read = 8192;
		// do not modify dst, it will continue from where it was
	} 

	// load next frame or second half
	if (result==FR_OK) { // sometimes KO but needs mounted
		// read all file, might not read all.
		#ifndef EMULATOR
		int t0 = DWT->CYCCNT;
		#endif
		
		result = f_read (&video_file,dst,bytes_to_read, &bytes_read); // errors because of data timeout  ... to be checkeds

		#ifdef DEBUG
		message("bytes read %d, toread %d\n",bytes_to_read,bytes_read);
		#endif 

		#ifndef EMULATOR
		t0 = DWT->CYCCNT-t0;
		t_tot += t0;
		#endif
		dst += bytes_read/4;
	}

	// allow jumping if we've read a whole frame, not in-between
	if (read_frame%4==3) { 

		// eof : loop it ? Stay here ?
		if (result==FR_OK && bytes_read<bytes_to_read) {
			jumpto_frame(0);
		}

		#ifdef DEBUG
		message("frame %d current event %d-%d\n",(vga_frame+1), evt_name, evt_frame);
		#endif 

		
		// now let's check events
		if ( 
			(evt_name == evt_always )||
			(evt_name == evt_a 		&& PUSHED(A)) ||
			(evt_name == evt_up 	&& PUSHED(up)) ||
			(evt_name == evt_down 	&& PUSHED(down)) ||
			(evt_name == evt_right 	&& PUSHED(right)) ||
			(evt_name == evt_left 	&& PUSHED(left))
			)
		{
			jumpto_frame(evt_frame);
		}

    	previous_gamepad = gamepad_buttons[0];
    	
	}

	read_frame+=1;

} 

#define SND_OFFSET (8+8+400*300/4+512)
void game_snd_buffer(uint16_t *buffer, int len) {
	uint8_t *src = (uint8_t*)buffer_disp;
	src += SND_OFFSET;

	for (int i=0;i<len;i++){
		*buffer++ = *src<<8 | *src;
		src++;
	}
}

// redo renderer & kernel by rendering 4 lines by 4 lines 1/4 width at a time
// will need 4+4=8 lines buffer ie 8*400 ~ 3.2 / 4k 
// case for overloading normal buffers ? completely remove all logic ? OSEF ici !

// uint16_t extra_buffers[512*4*2]; // 4k ram = 2*4 lines of 512 (400, padded ) pixels each - start at 1k boundary ?

void graph_frame( void ) {}

void graph_line ( void )  
{
    uint16_t *palette = (uint16_t*)buffer_disp+8;

    // palette is 256 u16 so 128 u32 after start (no padding)
    // data advances width/4 words per block (and a block is line/4)
    // also make 1/2 screen on even and 1/2 on odd frames since we're on a double mode
    uint32_t *src =  buffer_disp+4 + 128 + (WIDTH/4)*(vga_line / 4) + (vga_odd ? WIDTH/8 : 0); 
    uint32_t *dst = (uint32_t *)draw_buffer + (vga_odd ? WIDTH/4 : 0); 

    for (int i=0;i<WIDTH/4/2;i++) 
    {
        uint32_t word_block = *src++; 

        uint16_t c1=palette[ word_block >>24]; 
        uint16_t c2=palette[(word_block >>16) & 0xff]; 

        switch(word_block>>((vga_line&3)*4) & 0xf)
         {
            case   0 : *dst++ = (c2<<16)|c2; *dst++ = (c2<<16)|c2; break;
            case   1 : *dst++ = (c2<<16)|c1; *dst++ = (c2<<16)|c2; break;
            case   2 : *dst++ = (c1<<16)|c2; *dst++ = (c2<<16)|c2; break;
            case   3 : *dst++ = (c1<<16)|c1; *dst++ = (c2<<16)|c2; break;
            case   4 : *dst++ = (c2<<16)|c2; *dst++ = (c2<<16)|c1; break;
            case   5 : *dst++ = (c2<<16)|c1; *dst++ = (c2<<16)|c1; break;
            case   6 : *dst++ = (c1<<16)|c2; *dst++ = (c2<<16)|c1; break;
            case   7 : *dst++ = (c1<<16)|c1; *dst++ = (c2<<16)|c1; break;
            case   8 : *dst++ = (c2<<16)|c2; *dst++ = (c1<<16)|c2; break;
            case   9 : *dst++ = (c2<<16)|c1; *dst++ = (c1<<16)|c2; break;
            case  10 : *dst++ = (c1<<16)|c2; *dst++ = (c1<<16)|c2; break;
            case  11 : *dst++ = (c1<<16)|c1; *dst++ = (c1<<16)|c2; break;
            case  12 : *dst++ = (c2<<16)|c2; *dst++ = (c1<<16)|c1; break;
            case  13 : *dst++ = (c2<<16)|c1; *dst++ = (c1<<16)|c1; break;
            case  14 : *dst++ = (c1<<16)|c2; *dst++ = (c1<<16)|c1; break;
            case  15 : *dst++ = (c1<<16)|c1; *dst++ = (c1<<16)|c1; break;
        }
    }
}
