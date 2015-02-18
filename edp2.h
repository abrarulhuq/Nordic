
//#define TIMER_DELAY_MS  (1UL)  /**< Timer Delay in milli-seconds. */
#define TIMER_DELAY_MS  (1000UL)  /**< Timer Delay in milli-seconds. */

typedef enum
{
    TIMER0 = 0,  /**< Timer 0 module, base address at 0x40008000. */
    TIMER1,      /**< Timer 1 module, base address at 0x40009000. */
    TIMER2       /**< Timer 2 module, base address at 0x4000A000. */
} timer_t;



#include "nrf_delay.h"
#include <stdbool.h>
#include <stdint.h>

#define Mode0 				SPI_MODE0
#define FreqQ1Mbps  	SPI_OPERATING_FREQUENCY
#define MsbFirst 			0

#define		SPICLK_PIN	 SPI_PSELSCK0
#define		SPIMOSI_PIN  SPI_PSELMOSI0
#define		SPIMISO_PIN  SPI_PSELMISO0
#define 	EPD_CS_PIN    SPI_PSELSS0 //Ain4
//#define 	EPD_CS_PIN    27 //Ain4

#define 	EPD_RST_PIN  			 6  //Ain7
#define 	EPD_PANELON_PIN  	 5  //Ain6  
#define		EPD_DISCHARGE_PIN  4  //Ain5
#define 	EPD_BUSY_PIN 			26  //Ain 0
#define 	EPD_BORDER_PIN 		27	//Ain 1
//#define PWM_PIN 27 	  //Ain 1

// values for border byte
#define BORDER_BYTE_BLACK 0xff
#define BORDER_BYTE_WHITE 0xaa
#define BORDER_BYTE_NULL  0x00

#define	EPD_GFX_BLACK  1
#define EPD_GFX_WHITE  0
// 100 difference just to simplify things, so 1 pixel = 1 degree
#define T_MIN (-10)
#define T_MAX 80

typedef enum {
	EPD_1_44,        // 128 x 96
	EPD_2_0,         // 200 x 96
	EPD_2_7          // 264 x 176
} EPD_size;

typedef enum {           // Image pixel -> Display pixel
	EPD_inverse,     // B -> W, W -> B (New Image)
	EPD_normal       // B -> B, W -> W (New Image)
} EPD_stage;

typedef enum {           // error codes
	EPD_OK,
	EPD_UNSUPPORTED_COG,
	EPD_PANEL_BROKEN,
	EPD_DC_FAILED
} EPD_error;

typedef void EPD_reader(void *buffer, uint32_t address, uint16_t length);


//	const uint8_t EPD_Pin_PANEL_ON;
//	const uint8_t EPD_Pin_BORDER;
//	const uint8_t EPD_Pin_DISCHARGE;
//	const uint8_t EPD_Pin_RESET;
//	const uint8_t EPD_Pin_BUSY;
//	const uint8_t EPD_CS_PIN;

	void clear(void) ;
	void image(uint8_t *image_data) ;
	void image_sram(uint8_t *image_data);


	uint32_t get_current_time_tick(void);
		
	void power_off(void);
	void nothing_frame(void);
	void dummy_line(void);
	void border_dummy_line(void);



void start_EPD_timer(void);
void stop_EPD_timer(void) ;
void Timer_Init(timer_t timer, uint_fast16_t volatile number_of_ms);

	// power up and power down the EPD panel
void EPD_begin(void); 
void EPD_end(void) ;
	void EPD_setFactor(int temperature);


	// Low level API calls
	// ===================

	// single frame refresh
	void frame_fixed_timed(uint8_t fixed_value, uint32_t stage_time);

	// the B/W toggle stage
	void frame_stage2(void);

	// stages 1/3 functions
	void frame_fixed_13(uint8_t fixed_value, EPD_stage stage);
	void frame_data_13(uint8_t *image_data, EPD_stage stage, bool read_progmem);
	void frame_cb_13(uint32_t address, EPD_reader *reader, EPD_stage stage);

	// single line display - very low-level
	// also has to handle AVR progmem
	void EPD_line(uint16_t line, uint8_t *data, uint8_t fixed_value, bool read_progmem, EPD_stage stage, uint8_t border_byte, bool set_voltage_limit);

	// inline static void attachInterrupt();
	// inline static void detachInterrupt();

	void EPD_Init(EPD_size displaysize); 
	void Start_Show(void) ;
	void Start_Temp_Show(void);
	void Start_Show2(uint8_t *image_data) ;
	void Partial_Update(uint8_t Xpos, uint8_t Ypos, uint8_t rect_width, uint8_t rect_height, char *c);
	
	void EPD_GFX_display(void);
	void EPD_GFX_begin(void) ;
	void drawPixel(int16_t x, int16_t y, uint16_t colour) ;
	void Adafruit_GFX_Init(int16_t w, int16_t h);
	int16_t height(void);
  int16_t width(void);
	void drawStringXY(uint8_t Xpos, uint8_t Ypos, char *c, uint16_t color, uint16_t bgcolor, uint8_t fontsize);

	//===========gpio===================
	
	void EPD_GPIO_Config(void);
	
	bool EPD_IsBusy(void);
		
	void EPD_pwm_high(void);
	void EPD_pwm_low(void);
		
	void EPD_cs_high(void)	;
	void EPD_cs_low(void);
	
	void Flash_cs_high(void);
	void Flash_cs_low(void);
	
	void EPD_rst_high(void);
	void EPD_rst_low(void);
	
	void EPD_discharge_high(void);
	void EPD_discharge_low(void);
	
	void EPD_border_high(void);
	void EPD_border_low(void);
	
	void EPD_Vcc_turn_off (void);
	void EPD_Vcc_turn_on(void);
	
	void EPD_SPI_send(const uint8_t *buffer, uint16_t length);
	uint8_t epd_spisend(uint8_t header, uint8_t index);
	
