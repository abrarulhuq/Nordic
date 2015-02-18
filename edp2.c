
#include "edp2.h"
#include "nrf_gpio.h"
#include "image_data_EPD200.h"
#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include "spi.h"
#include <string.h>

static volatile uint32_t EPD_Counter;

EPD_error EPD_status;
EPD_size sizeofdisplay;
// Default values for 1.44 inch
uint16_t lines_per_display = 96;
uint16_t dots_per_line = 128;
uint16_t bytes_per_line = 128 / 8;
uint16_t bytes_per_scan = 96 / 4;
uint8_t voltage_level = 0x03;
	
	struct compensation_type
	{
		uint16_t stage1_repeat;
		uint16_t stage1_step;
		uint16_t stage1_block;
		uint16_t stage2_repeat;
		uint16_t stage2_t1;
		uint16_t stage2_t2;
		uint16_t stage3_repeat;
		uint16_t stage3_step;
		uint16_t stage3_block;
	};
	
struct compensation_type *compensation;
	
	// stage1: repeat, step, block
	// stage2: repeat, t1, t2
	// stage3: repeat, step, block

	const struct compensation_type compensation_144[3] = 
	{
		{ 2, 6, 42,   4, 392, 392,   2, 6, 42 },  //  0 ... 10 Celcius
		{ 4, 2, 16,   4, 155, 155,   4, 2, 16 },  // 10 ... 40 Celcius
		{ 4, 2, 16,   4, 155, 155,   4, 2, 16 }   // 40 ... 50 Celcius
	};

	const struct compensation_type compensation_200[3] = 
	{
		{ 2, 6, 42,   4, 392, 392,   2, 6, 42 },  //  0 ... 10 Celcius
		{ 2, 2, 48,   4, 196, 196,   2, 2, 48 },  // 10 ... 40 Celcius
		{ 4, 2, 48,   4, 196, 196,   4, 2, 48 }   // 40 ... 50 Celcius
	};

	const struct compensation_type compensation_270[3] = 
	{
		{ 2, 8, 64,   4, 392, 392,   2, 8, 64 },  //  0 ... 10 Celcius
		{ 2, 4, 32,   4, 196, 196,   2, 4, 32 },  // 10 ... 40 Celcius
		{ 4, 8, 64,   4, 196, 196,   4, 8, 64 }   // 40 ... 50 Celcius
	};


uint8_t *channel_select;
uint16_t channel_select_length;

uint16_t temperature_offset;	
	
uint8_t state = 2;
uint8_t flag = 1;	
	
/*	
	EPD_1_44,        // 128 x 96
	EPD_2_0,         // 200 x 96
	EPD_2_7          // 264 x 176
*/
static const int pixel_width = 200;  // must be a multiple of 8
static const int pixel_height = 96;

uint8_t image_array[pixel_width * pixel_height / 8];
uint8_t old_image_array[pixel_width * pixel_height / 8];

extern uint8_t t1complete , t2complete , t3complete , t4complete ;	
	
/*==================================================================

==================================================================*/
void Start_Task_Show(uint8_t y,char *c) 
{
		uint8_t displaying_count;

		Adafruit_GFX_Init(pixel_width, pixel_height);
	// set up graphics EPD library and clear the screen
		EPD_GFX_begin();	
		
	displaying_count = y;
	if (displaying_count == 0)
	{
			memset(image_array,0,sizeof(image_array)); //making image all white
			drawStringXY(18,10,c,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
	}
	if (displaying_count == 1)
	{
			memset(image_array,0,sizeof(image_array)); //making image all white
			drawStringXY(18,10,c+13,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
	}
	else if (displaying_count == 2)
	{
			memset(image_array,0,sizeof(image_array)); //making image all white
			drawStringXY(18,10,c+13,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
			drawStringXY(18,30,c+26,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
	}
	else if (displaying_count == 3)
	{
			memset(image_array,0,sizeof(image_array)); //making image all white
			drawStringXY(18,10,c+13,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
			drawStringXY(18,30,c+26,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
			drawStringXY(18,50,c+39,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
	}
	else if (displaying_count == 4)
	{
			memset(image_array,0,sizeof(image_array)); //making image all white
			drawStringXY(18,10,c+13,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
			drawStringXY(18,30,c+26,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
			drawStringXY(18,50,c+39,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
			drawStringXY(18,70,c+52,EPD_GFX_BLACK,EPD_GFX_WHITE,2);
	}
	
	//for drawing tick symbol using 2 triangles
	if (t1complete == 1)
	{	
		fillTriangle(2,12, 5,22, 5,17, EPD_GFX_BLACK);
		fillTriangle(5,22, 5,17, 16,8, EPD_GFX_BLACK);
  }
	if (t2complete == 1)
	{
		fillTriangle(2,32, 5,42, 5,37, EPD_GFX_BLACK);
		fillTriangle(5,42, 5,37, 16,28, EPD_GFX_BLACK);
  }
	if (t3complete == 1)
	{
		fillTriangle(2,52, 5,62, 5,57, EPD_GFX_BLACK);
		fillTriangle(5,62, 5,57, 16,48, EPD_GFX_BLACK);
  }
	if (t4complete == 1)
	{
		fillTriangle(2,72, 5,82, 5,77, EPD_GFX_BLACK);
		fillTriangle(5,82, 5,77, 16,68, EPD_GFX_BLACK);
	}
	// update the display
	 EPD_GFX_display();

}	

/*==================================================================
Note: the drawing is done in "image_array"
void  fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) for making
white square to write text within the square
==================================================================*/
void Partial_Update(uint8_t Xpos, uint8_t Ypos, uint8_t rect_width, uint8_t rect_height, char *c)
{
		//Paste the recently displayed image in image_array
		memcpy(image_array,old_image_array,sizeof(image_array));
		 
		Adafruit_GFX_Init(pixel_width, pixel_height);
		EPD_GFX_begin();	// set up graphics EPD library and clear the screen

		fillRect(Xpos, Ypos, rect_width, rect_height, EPD_GFX_WHITE);
		drawStringXY(Xpos+5,Ypos+5,c,EPD_GFX_BLACK,EPD_GFX_WHITE,1); //write inside box
		drawStringXY(Xpos+5,Ypos+15,"Text line 2",EPD_GFX_BLACK,EPD_GFX_WHITE,1); //write inside box
		drawStringXY(Xpos+5,Ypos+25,"Here line 3",EPD_GFX_BLACK,EPD_GFX_WHITE,1); //write inside box
		// update the display
		 EPD_GFX_display();

}	

/*==================================================================
Note: the drawing is done in "image_array"
==================================================================*/
void Start_Temp_Show(void)
{
		Adafruit_GFX_Init(pixel_width, pixel_height);
		// set up graphics EPD library and clear the screen
		EPD_GFX_begin();
		
		drawStringXY(10,10,"ARKA IMS",EPD_GFX_BLACK,EPD_GFX_WHITE,2);
		drawStringXY(10,30,"Text Display",EPD_GFX_BLACK,EPD_GFX_WHITE,2);
//	fillRect(100, 50, 50, 40, EPD_GFX_WHITE);
		drawRect(100, 50, 98, 40, EPD_GFX_BLACK) ;
		drawStringXY(105,55,"Custom",EPD_GFX_BLACK,EPD_GFX_WHITE,1); //write inside box
		drawStringXY(105,65,"Text",EPD_GFX_BLACK,EPD_GFX_WHITE,1); //write inside box
		drawStringXY(105,75,"Here",EPD_GFX_BLACK,EPD_GFX_WHITE,1); //write inside box
		// update the display
		 EPD_GFX_display();

}

/*==================================================================

==================================================================*/

void Start_Show2(uint8_t *image_data) 
{

	EPD_Init(EPD_2_0); // Select the panel size	
	EPD_begin(); // power up the EPD panel
	EPD_setFactor(28); // temperature taking normal room temp
	image_sram(image_data);
	EPD_end();   // power down the EPD panel
}


/*==================================================================

==================================================================*/
void Start_Show(void) 
{

	EPD_Init(EPD_2_0); // Select the panel size
		
	EPD_begin(); // power up the EPD panel

	EPD_setFactor(28); // temperature taking normal room temp

	switch(state) 
	{
		
			case 0:         // clear the screen
			clear();
			state = 1;
			
			break;

		case 1:         // first image
//			image_sram(image_array_3);
			state = 4;  //run once
			break;

		case 2:         // second image
	//		image_sram(image_array_3);
			state = 4;  // run once
			break;
		
		default: 
				break;
	}
	
	EPD_end();   // power down the EPD panel
}


/*==================================================================

==================================================================*/
void EPD_GFX_display(void) 
{
	// erase old, display new
	EPD_begin();
//	clear();
	EPD_setFactor(28);  			// temperature taking normal room temp 28
	image_sram(image_array);
	EPD_end();
}

/*==================================================================

==================================================================*/
void EPD_GFX_begin(void) 
{

	EPD_Init(EPD_2_0); // Select the panel size
	EPD_begin();			 // power up the EPD panel
//	clear();
	EPD_end();   			 // power down the EPD panel

		
//	memset(image_array, 255, sizeof(image_array));  
//	memset(image_array, 0, sizeof(image_array));  // clear buffers to white
	
	

}
/*==================================================================
	Note: The pixel drawing is done in "image_array"
==================================================================*/
// set a single pixel in new_image
void drawPixel(int16_t x, int16_t y, uint16_t colour) 
{
		int bit = x & 0x07;
		int byte = x / 8 + y * (pixel_width / 8);
		int mask = 0x01 << bit;
		if (EPD_GFX_BLACK == colour) 
		{
			image_array[byte] |= mask;
		} 
		else 
		{
			image_array[byte] &= ~mask;
		}
}

/*==================================================================

==================================================================*/
	EPD_error error(void) 
	{
		return  EPD_status;
	}

/*==================================================================

==================================================================*/
	// clear display (anything -> white)
	void clear(void) 
	{
			frame_fixed_13(0xff, EPD_inverse); 	//for complete black screen
//  	 frame_fixed_13(0xaa, EPD_inverse);	  //for complete white screen
//		 frame_stage2();
//		 frame_fixed_13(0xaa, EPD_normal);
	}

/*==================================================================
	// output an image (PROGMEM data)
==================================================================*/

	void image(uint8_t *image_data) 
	{
		 frame_data_13(image_data, EPD_inverse,true);
		 frame_stage2();
		 frame_data_13(image_data, EPD_normal,true);
	}


/*==================================================================
// change from old image to new image (SRAM version)
==================================================================*/
	
	void image_sram(uint8_t *image_data)
	{
		// copying the image data as backup
		memcpy(old_image_array,image_data,sizeof(old_image_array)); 
		
		frame_data_13(image_data, EPD_inverse, false);
		frame_stage2();
		frame_data_13(image_data, EPD_normal, false);
	}

/*==================================================================
//EPD_2_0
==================================================================*/	

void EPD_Init(EPD_size displaysize)
{
	 sizeofdisplay = displaysize;
	 EPD_setFactor(28); //default

	// display size dependant items
	{
		static uint8_t cs[] = {0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00};
		 channel_select = cs;
		 channel_select_length = sizeof(cs);
	}

	// set up size structure
	switch (sizeofdisplay) 
	{
			default:
			case EPD_1_44:  // default so no change
				break;

			case EPD_2_0: 
			{
				 lines_per_display = 96;
				 dots_per_line = 200;
				 bytes_per_line = 200 / 8;
				 bytes_per_scan = 96 / 4;
				static uint8_t cs[] = {0x72, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00};
				 channel_select = cs;
				 channel_select_length = sizeof(cs);
				 voltage_level = 0x03;
				break;
			}

			case EPD_2_7: 
			{
				 lines_per_display = 176;
				 dots_per_line = 264;
				 bytes_per_line = 264 / 8;
				 bytes_per_scan = 176 / 4;
				static uint8_t cs[] = {0x72, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfe, 0x00, 0x00};
				 channel_select = cs;
				 channel_select_length = sizeof(cs);
				 voltage_level = 0x00;
				break;
			}
	}
	EPD_GPIO_Config();
}

/*==================================================================

==================================================================*/
void EPD_setFactor(int temperature) 
{
	if (temperature < 10) 
	{
		 temperature_offset = 0;
	} 
	else if (temperature > 40) 
	{
		  temperature_offset = 2;
  } 
	else 
	{
		 temperature_offset = 1;
	}
	switch (sizeofdisplay) 
	{
			default:
			case EPD_1_44:
				// compensation = &compensation_144[temperature_offset];
				compensation = (struct compensation_type *) &compensation_144[temperature_offset];
				break;

			case EPD_2_0: 
			{
				 //compensation = &compensation_200[temperature_offset];
				compensation = (struct compensation_type *) &compensation_200[temperature_offset];
				break;
			}

			case EPD_2_7: 
			{
				 //compensation = &compensation_270[temperature_offset];
				compensation = (struct compensation_type *) &compensation_270[temperature_offset];
				break;
			}
	}
}

/*==================================================================

==================================================================*/
void EPD_begin(void) 
{

	// assume ok
	 EPD_status = EPD_OK;

	EPD_rst_low()	;
	EPD_Vcc_turn_off() ;
	EPD_discharge_low();
	EPD_border_low();
	EPD_cs_low();

	SPI_on(Mode0, FreqQ1Mbps, MsbFirst) ; //MODE0, MSBFIRST, 1Mbps();(Mode0, FreqQ1Mbps, MsbFirst) ; //MODE0, MSBFIRST, 1Mbps();

	nrf_delay_ms(5);
	EPD_Vcc_turn_on();
	nrf_delay_ms(10);

	EPD_rst_high();
	EPD_border_high();
	EPD_cs_high();
	nrf_delay_ms(5);

	EPD_rst_low();
	nrf_delay_ms(5);

	EPD_rst_high();
	nrf_delay_ms(5);

	// wait for COG to become ready
	while (1  == EPD_IsBusy()) 
	{
		nrf_delay_us(10);
	}

	// read the COG ID
	uint8_t cog_id = epd_spisend(0x71, 0x00);
	cog_id = epd_spisend(0x71, 0x00);

	if (0x02 != (0x0f & cog_id)) 
	{
		 EPD_status = EPD_UNSUPPORTED_COG;
		 power_off();
		return;
	}

	// Disable OE
	epd_spisend(0x70, 0x02);
	epd_spisend(0x72, 0x40);

	// check breakage
	epd_spisend(0x70, 0x0f);
	uint8_t broken_panel = epd_spisend(0x73, 0x00); 
	if (0x00 == (0x80 & broken_panel)) 
	{
		 EPD_status = EPD_PANEL_BROKEN;
		 power_off();
		return;
	}

	// power saving mode
	epd_spisend(0x70, 0x0b);
	epd_spisend(0x72, 0x02);

	// channel select
	epd_spisend(0x70, 0x01);
	EPD_SPI_send(channel_select, channel_select_length);

	// high power mode osc
	epd_spisend(0x70, 0x07);
	epd_spisend(0x72, 0xd1);

	// power setting
	epd_spisend(0x70, 0x08);
	epd_spisend(0x72, 0x02);

	// Vcom level
	epd_spisend(0x70, 0x09);
	epd_spisend(0x72, 0xc2);

	// power setting
	epd_spisend(0x70, 0x04);
	epd_spisend(0x72, 0x03);

	// driver latch on
	epd_spisend(0x70, 0x03);
	epd_spisend(0x72, 0x01);

	// driver latch off
	epd_spisend(0x70, 0x03);
	epd_spisend(0x72, 0x00);

	
	nrf_delay_ms(5);

	bool dc_ok = false;

	for (int i = 0; i < 4; ++i) 
	{
			// charge pump positive voltage on - VGH/VDL on
			epd_spisend(0x70, 0x05);
			epd_spisend(0x72, 0x01);

			nrf_delay_ms(240);

			// charge pump negative voltage on - VGL/VDL on
			epd_spisend(0x70, 0x05);
			epd_spisend(0x72, 0x03);

			nrf_delay_ms(40);

			// charge pump Vcom on - Vcom driver on
			epd_spisend(0x70, 0x05);
			epd_spisend(0x72, 0x0f);

			nrf_delay_ms(40);

			// check DC/DC
			epd_spisend(0x70, 0x0f);
			uint8_t dc_state = epd_spisend(0x73, 0x00);
			if (0x40 == (0x40 & dc_state)) 
			{
				dc_ok = true;
				break;
			}
	}
	if (!dc_ok) 
	{
		// output enable to disable
		epd_spisend(0x70, 0x02);
		epd_spisend(0x72, 0x40);

		 EPD_status = EPD_DC_FAILED;
		 power_off();
		return;
	}

	SPI_off();
}

/*==================================================================

==================================================================*/
void EPD_end(void) 
{

	 nothing_frame();

	if (EPD_1_44 ==  sizeofdisplay || EPD_2_0 ==  sizeofdisplay) 
	{
		 border_dummy_line();
	}

	 dummy_line();

	if (EPD_2_7 ==  sizeofdisplay) 
	{
		// only pulse border pin for 2.70" EPD
		EPD_border_low() ;
		nrf_delay_ms(200);
		EPD_border_high() ;
	}

	SPI_on(Mode0, FreqQ1Mbps, MsbFirst) ; //MODE0, MSBFIRST, 1Mbps();(MODE0, FREQ1MBPS, MSBFIRST) ;

	// check DC/DC
	epd_spisend(0x70, 0x0f);
	int dc_state = epd_spisend(0x73, 0x00);  
	if (0x40 != (0x40 & dc_state)) 
	{
		 EPD_status = EPD_DC_FAILED;
		 power_off();
		return;
	}

	// latch reset turn on
	epd_spisend(0x70, 0x03);
	epd_spisend(0x72, 0x01);

	// output enable off
	epd_spisend(0x70, 0x02);
	epd_spisend(0x72, 0x05);

	// power off charge pump Vcom
	epd_spisend(0x70, 0x05);
	epd_spisend(0x72, 0x03);

	// power off charge pump neg voltage
	epd_spisend(0x70, 0x05);
	epd_spisend(0x72, 0x01);

	nrf_delay_ms(240);

	// power off all charge pumps
	epd_spisend(0x70, 0x05);
	epd_spisend(0x72, 0x00);

	// turn of osc
	epd_spisend(0x70, 0x07);
	epd_spisend(0x72, 0x01);

	// discharge internal on
	epd_spisend(0x70, 0x04);
	epd_spisend(0x72, 0x83);

	nrf_delay_ms(30);

	// discharge internal off
	//epd_spisend(0x70, 0x04);
	//epd_spisend(0x72, 0x00);

	power_off();
}

/*==================================================================

==================================================================*/
void power_off(void) 
{

	// turn of power and all signals
	EPD_rst_low();
	EPD_Vcc_turn_off();
	EPD_border_low();

	// ensure SPI MOSI and CLOCK are Low before CS Low
	SPI_off();
	EPD_cs_low() ;

	// pulse discharge pin
	EPD_discharge_high();
	nrf_delay_ms(150);
	EPD_discharge_low();
}



/*==================================================================
// One frame of data is the number of lines * rows. For example:
// The 1.44” frame of data is 96 lines * 128 dots.
// The 2” frame of data is 96 lines * 200 dots.
// The 2.7” frame of data is 176 lines * 264 dots.

// the image is arranged by line which matches the display size
// so smallest would have 96 * 32 bytes  
==================================================================*/
void frame_fixed_timed(uint8_t fixed_value, uint32_t stage_time) 
{
	 
//	start_EPD_timer();
//	do 
//	{
//			uint32_t t_start = get_current_time_tick();
			for (uint8_t line = 0; line <  lines_per_display ; ++line) 
			{
				 EPD_line(lines_per_display - line - 1, 0, fixed_value, false, EPD_normal,BORDER_BYTE_NULL,false);
			}
//			uint32_t t_end = get_current_time_tick(); 
//			if (t_end > t_start) 
//			{
//				stage_time = t_end - t_start;
//			} 
//			else 
//			{																				 
//				stage_time -= t_start - t_end + 1 + (0xFFFFFFFF);//ULONG_MAX = 0xFFFFFFFFUL
//			}
//	} while (get_current_time_tick()< 200);
	
//	stop_EPD_timer();
}

/*==================================================================

==================================================================*/
void frame_fixed_13(uint8_t value, EPD_stage stage) 
{

	uint16_t repeat;
	uint16_t step;
	uint16_t block;
	uint16_t total_lines;
	int block_begin;
	int block_end ;
	int line, n;
	
	if (EPD_inverse == stage)   //=======================compensation values set here==============================
	{  // stage 1
			repeat = compensation->stage1_repeat;
			step =   compensation->stage1_step;
			block =  compensation->stage1_block;
	} 
	else 
	{                     // stage 3
			repeat = compensation->stage3_repeat;
			step =   compensation->stage3_step;
			block =  compensation->stage3_block;
	}

	total_lines =  lines_per_display;

	for (n = 0; n < repeat; ++n) 
	{

		block_begin = 0;
		block_end = 0;

		while (block_begin < total_lines) 
		{

				block_end += step;
				block_begin = block_end - block;
				if (block_begin < 0) 
				{
					block_begin = 0;
				} 
				else if (block_begin >= total_lines) 
				{
					break;
				}

				bool full_block = (block_end - block_begin == block);

				for (line = block_begin; line < block_end; ++line) 
				{
						if (line >= total_lines) 
						{
							break;
						}
						if (full_block && (line < (block_begin + step))) 
						{
							 EPD_line(line, 0, 0x00, false, EPD_normal,BORDER_BYTE_NULL,false); 
						} 
						else 
						{
							 EPD_line(line, 0, value, false, EPD_normal,BORDER_BYTE_NULL,false);
						}
				}
		}
	}
}


/*==================================================================

==================================================================*/
void frame_data_13(uint8_t *image, EPD_stage stage, bool read_progmem) 
{

	uint16_t repeat;
	uint16_t step;
	uint16_t block;
	uint16_t total_lines;
	int block_begin ;
	int block_end ;
	int line, n ;

	if (stage == EPD_inverse) 
	{  // stage 1
		repeat =  compensation->stage1_repeat;
		step =  	compensation->stage1_step;
		block =  	compensation->stage1_block;
	} 
	else 
	{                     // stage 3
		repeat =  compensation->stage3_repeat;
		step =  	compensation->stage3_step;
		block =   compensation->stage3_block;
	}

	 total_lines =  lines_per_display;

	for (n = 0; n < repeat; ++n) 
	{

		 block_begin = 0;
		 block_end = 0;

		while (block_begin < total_lines) 
		{

				block_end += step;
				block_begin = block_end - block;
				if (block_begin < 0) 
				{
					block_begin = 0;
				} 
				else if (block_begin >= total_lines) 
				{
					break;
				}

				bool full_block = (block_end - block_begin == block);

				for (line = block_begin; line < block_end; ++line) 
				{
						if (line >= total_lines) 
						{
							break;
						}
						if (full_block && (line < (block_begin + step))) 
						{
							 EPD_line(line, 0, 0x00, false, EPD_normal,BORDER_BYTE_NULL,false);
						} 
						else 
						{
							 EPD_line(line, &image[line *  bytes_per_line], 0x00, read_progmem, stage, BORDER_BYTE_NULL,false);
						}
				}
		}
	}
}

/*==================================================================

==================================================================*/
void EPD_frame_cb_13(uint32_t address, EPD_reader *reader, EPD_stage stage) 
{
	uint8_t buffer[264 / 8]; // allows for 2.70" panel
	uint16_t repeat;
	uint16_t step;
	uint16_t block;
	uint16_t total_lines;
	int block_begin;
	int block_end;
	int line, n;
	
	if (stage == EPD_inverse ) 
	{  // stage 1
		repeat =  compensation->stage1_repeat;
		step =  compensation->stage1_step;
		block =  compensation->stage1_block;
	} 
	else 
	{                     // stage 3
		repeat =  compensation->stage3_repeat;
		step =  compensation->stage3_step;
		block =  compensation->stage3_block;
	}

	total_lines =  lines_per_display;

	for (n = 0; n < repeat; ++n) 
	{

		block_begin = 0;
		block_end = 0;

		while (block_begin < total_lines) 
		{

			block_end += step;
			block_begin = block_end - block;
			if (block_begin < 0) 
			{
				block_begin = 0;
			} 
			else if (block_begin >= total_lines) 
			{
				break;
			}

			bool full_block = (block_end - block_begin == block);

			for (line = block_begin; line < block_end; ++line) 
			{
					if (line >= total_lines) 
					{
						break;
					}
					if (full_block && (line < (block_begin + step))) 
					{
						 EPD_line(line, 0, 0x00, false, EPD_normal,BORDER_BYTE_NULL,false);
					} 
					else 
					{
						reader(buffer, address + line *  bytes_per_line,  bytes_per_line);
						EPD_line(line, buffer, 0, false, stage,BORDER_BYTE_NULL,false);
					}
			}
		}
	}
}


/*==================================================================

==================================================================*/
void frame_stage2(void) 
{
	uint16_t i; 
	for (i = 0; i < (compensation->stage2_repeat); ++i) 
	{
		 frame_fixed_timed(0xff,  compensation->stage2_t1);
		 frame_fixed_timed(0xaa,  compensation->stage2_t2);
	}
}

/*==================================================================

==================================================================*/
void nothing_frame(void) 
{
	uint16_t line;
	for ( line = 0; line <  lines_per_display; ++line) 
	{
		 EPD_line(line, 0, 0x00, false, EPD_normal, BORDER_BYTE_NULL, true);
	}
}

/*==================================================================

==================================================================*/
void dummy_line(void) 
{
	 EPD_line(0x7fffu, 0, 0x00, false, EPD_normal, BORDER_BYTE_NULL, true);
}

/*==================================================================

==================================================================*/
//	void line(uint16_t line, const uint8_t *data, uint8_t fixed_value, bool read_progmem, EPD_stage stage = EPD_normal, uint8_t border_byte = BORDER_BYTE_NULL, bool set_voltage_limit = false);
void border_dummy_line(void) 
{
		EPD_line(0x7fffu, 0, 0x00, false, EPD_normal, BORDER_BYTE_BLACK,false);
		nrf_delay_ms(40);
		EPD_line(0x7fffu, 0, 0x00, false, EPD_normal, BORDER_BYTE_WHITE,false);
		nrf_delay_ms(200);
}

/*==================================================================

==================================================================*/

void EPD_line(uint16_t line, uint8_t *data, uint8_t fixed_value, bool read_progmem, EPD_stage stage, uint8_t border_byte, bool set_voltage_limit) 
{
	uint16_t b ;
	uint8_t pixels ; 
	int scan_pos, scan_shift ;
	
       	SPI_on(Mode0, FreqQ1Mbps, MsbFirst) ; //MODE0, MSBFIRST, 1Mbps

       if (set_voltage_limit) 
			 {
	       // charge pump voltage level reduce voltage shift
	       epd_spisend(0x70, 0x04);
	       epd_spisend(0x72,  voltage_level);
       }

       // send data
       epd_spisend(0x70, 0x0a);

       nrf_delay_us(10);

       // CS low
			 EPD_cs_low();
       SPI_transfer(0x72);

       // border byte
       SPI_transfer(border_byte);

       // odd pixels
     for (b =  bytes_per_line; b > 0; --b) 
	   {
				 if (0 != data) 
				 {
						 pixels = data[b - 1];
						 switch(stage) 
						 {
								case EPD_inverse:      // B -> W, W -> B
									pixels ^= 0xff;
									break;
								case EPD_normal:       // B -> B, W -> W
									break;	
						 }
						 pixels = 0xaa | pixels;
						 SPI_transfer(pixels);
				 } 
				 else 
				 {
						 SPI_transfer(fixed_value);
				 }
     }

       // scan line
     scan_pos = ( lines_per_display - line - 1) >> 2;
     scan_shift = (line & 0x03) << 1;
     for (b = 0; b <  bytes_per_scan; ++b) 
	   {
	       if (scan_pos == (int) b) 
				 {
		       SPI_transfer(0x03 << scan_shift);
	       } 
				 else 
				 {
		       SPI_transfer(0x00);
	       }
     }

       // even pixels
     for (b = 0; b <  bytes_per_line; ++b) 
	   {
				 if (0 != data) 
				 {
						 pixels = data[b];

						 switch(stage) 
						 {
								 case EPD_inverse:      // B -> W, W -> B (Current Image)
											pixels ^= 0xff;
											break;
								 case EPD_normal:       // B -> B, W -> W (New Image)
											break;
						 }
						 pixels >>= 1;
						 pixels |= 0xaa;

						 pixels = ((pixels & 0xc0) >> 6)
							 | ((pixels & 0x30) >> 2)
							 | ((pixels & 0x0c) << 2)
							 | ((pixels & 0x03) << 6);
						 SPI_transfer(pixels);
				 } 
				 else 
				 {
						SPI_transfer(fixed_value);
				 }
     }

       // CS high
       EPD_cs_high();

       // output data to panel
       epd_spisend(0x70, 0x02);
       epd_spisend(0x72, 0x07);
				
				
			 
       SPI_off();
}

//============================SPI==========================================================

/*==================================================================

==================================================================*/
//Used to transfer multiple bytes 
void EPD_SPI_send(const uint8_t *buffer, uint16_t length)
{

	EPD_cs_low ();
	
	// send all data
	for (uint16_t i = 0; i < length; ++i) 
	{
		SPI_transfer(*buffer++);
	}

	// CS high
	EPD_cs_high ();
}

/*==================================================================

==================================================================*/
uint8_t epd_spisend(uint8_t header, uint8_t index)
{
		uint8_t val; 
	 	EPD_cs_low ();
		SPI_transfer (header); // header of Register Index
		val = SPI_transfer (index);
		EPD_cs_high ();	
		nrf_delay_us(10);

		return val;
}



/*==================================================================

==================================================================*/
//epd GPIO function
bool EPD_IsBusy(void)
{
	nrf_delay_us(2);
	return 0;
	//return (bool)nrf_gpio_pin_read(EPD_BUSY_PIN);
}
	
//void EPD_pwm_high()    
//{
//	nrf_gpio_pin_set(EPD_Pin_PWM);
//}
//void EPD_pwm_low()     
//{
//	nrf_gpio_pin_clear(EPD_Pin_PWM);
//}
	
void EPD_cs_high()     
{
	nrf_gpio_pin_set(EPD_CS_PIN);
}
void EPD_cs_low()      
{
	nrf_gpio_pin_clear(EPD_CS_PIN);
}

//void Flash_cs_high()   
//{
//	nrf_gpio_pin_set(EPD_Pin_FLASH_CS);
//}
//void Flash_cs_low()    
//{
//	nrf_gpio_pin_clear(EPD_Pin_FLASH_CS);
//}

void EPD_rst_high()    
{
	nrf_gpio_pin_set(EPD_RST_PIN);
}
void EPD_rst_low()     
{
	nrf_gpio_pin_clear(EPD_RST_PIN);
}

void EPD_discharge_high() 
{
	nrf_gpio_pin_set(EPD_DISCHARGE_PIN);
}
void EPD_discharge_low()  
{
	nrf_gpio_pin_clear(EPD_DISCHARGE_PIN);
}

void EPD_border_high()    
{
	//nrf_gpio_pin_set(EPD_BORDER_PIN);
}
void EPD_border_low()     
{
	//nrf_gpio_pin_clear(EPD_BORDER_PIN);
}

void EPD_Vcc_turn_off() 
{
	nrf_gpio_pin_clear(EPD_PANELON_PIN);
}
void EPD_Vcc_turn_on()  
{
	nrf_gpio_pin_set(EPD_PANELON_PIN);
}

/*==================================================================

==================================================================*/
void EPD_GPIO_Config(void)
{
		nrf_gpio_cfg_output(EPD_PANELON_PIN);
	//	nrf_gpio_cfg_output(EPD_BORDER_PIN);
		nrf_gpio_cfg_output(EPD_DISCHARGE_PIN);
		nrf_gpio_cfg_output(EPD_RST_PIN);
		nrf_gpio_cfg_output(EPD_CS_PIN);
//	nrf_gpio_cfg_output(PWM_PIN);
//	nrf_gpio_cfg_input(EPD_BUSY_PIN,NRF_GPIO_PIN_PULLUP);

	  EPD_Vcc_turn_off();		
		EPD_cs_low();
//	EPD_pwm_low();
		EPD_rst_low();
		EPD_discharge_low();
		EPD_border_low();
}




