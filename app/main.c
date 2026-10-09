#include <K1921VG015.h>
#include <system_k1921vg015.h>
#include <retarget.h>
#include <memasm.h>
#include <string.h>



/*============Дефайн скорости UART==================*/
#define UART2_BAUD  115200


/*============Дефайн словаря==================*/
#define DICT_SIZE (sizeof(my_dict) / sizeof(FontMap))


/*============Дефайны для парсера==================*/
#define DMA_PING_PONG_SIZE 10
#define PARSER_BUF_SIZE 256
#define MARKER_START 0xA
#define QUEUE_SIZE 4



/*============Переменные для пинг-понг==================*/
uint8_t DMA_PING[DMA_PING_PONG_SIZE];
uint8_t DMA_PONG[DMA_PING_PONG_SIZE];

DMA_CtrlData_TypeDef DMA_CONFIGDATA __attribute__((aligned(1024)));



/*============Переменные для парсера==================*/
uint8_t parser_buf[PARSER_BUF_SIZE];
uint8_t checkParser[10];

uint8_t parser_state = 0;
uint8_t sum_calc = 0;
uint8_t expected_len_data = 0;
uint8_t real_len_data = 0;

volatile uint8_t start_for_DMA = 0;
volatile uint8_t start_for_parser = 0;

volatile uint8_t start_write_func = 0;
volatile uint8_t start_exe_func = 0;


typedef enum
{
	LED_CONTROL = 0x11,
	DISPLAY_CONTROL = 0x12,
	UART_2 = 0x13
} DeviceAddr; //Возможные адреса протокола

const uint8_t validAddr[] =
{
		LED_CONTROL, DISPLAY_CONTROL, UART_2
};


typedef enum
{
	REDRAW_WINDOW1 = 0x21,
	LED_OFF = 0x22,
	DISPLAY_FULL = 0x31,
	DISPLAY_CLEAR = 0x32,
	UART_2_TX = 0x41
} Commands;//Возможный список команд

const uint8_t validCMD[] =
{
		REDRAW_WINDOW1, LED_OFF, DISPLAY_FULL, DISPLAY_CLEAR, UART_2_TX
};

const int8_t cmd_data_lens[] = //соответствие каждой команде\функции своей длины данных
{
		8, 5, 16, 3, 24 //случайное количество байт для каждой из команд
};


typedef struct
{
	uint8_t addr; uint8_t cmd; uint8_t data[24]; uint8_t data_len;
} Packet;

Packet packet[QUEUE_SIZE];



/*============Переменные для настройки PLL==================*/
uint32_t timeout_counter;



/*============Переменные для работы TMR32==================*/
volatile uint32_t SysTimer_ms = 0;
volatile uint32_t Delay_counter_ms = 0;



/*============Переменные для меню==================*/
typedef enum
{
	WINDOW_1 = 1,
	SETTINGS_1 = 2,
	SETTINGS_2 = 3,
} Windows;


volatile uint8_t current_window = WINDOW_1;
volatile uint8_t current_window1_1 = 1;
volatile uint8_t current_window1_2 = 4;

volatile uint32_t gpio_btn1 = 0;
volatile uint32_t gpio_btn2 = 0;
volatile uint32_t gpio_btn3 = 0;
volatile uint32_t gpio_btn4 = 0;

volatile uint8_t isEditing = 0;
volatile uint8_t need_window1 = 0;

volatile uint8_t cursor_y = 29;
volatile uint8_t cursor_x = 0;


char window_1[] = "Б01  Е-10  dP-14Д02  Е-09  dP-15";
char window_2;
char window_3;

char window1_12[] = "АБВГД";


/*============Переменные для рисования==================*/
typedef struct
{
    char key;           // Буква (например, 'Т')
    uint32_t bitmap[28];  // Массив пикселей
} FontMap;


const FontMap my_dict[] =
{
		{'А', {0x8, 0x1C, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7F, 0x7F, 0x7F, 0x7F, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63}},
		{'Б', {0x7F, 0x7F, 0x7F, 0x7F, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7C, 0x7C, 0x7E, 0x7E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x7E, 0x7C, 0x7C}},
		{'В', {0x7C, 0x7C, 0x7E, 0x7E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x7C, 0x7C, 0x7E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x7E, 0x7C, 0x7C}},
		{'Г', {0x7F, 0x7F, 0x7F, 0x7F, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60}},
		{'Д', {0x7, 0x7, 0xF, 0xF, 0x1B, 0x1B, 0x1B, 0x1B, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x7F, 0x7F, 0x7F, 0x7F, 0x63, 0x63, 0x63, 0x63}},
		{'Е', {0x7F, 0x7F, 0x7F, 0x7F, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7C, 0x7C, 0x7C, 0x7C, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7F, 0x7F, 0x7F, 0x7F}},
		{'Ж', {0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x3E, 0x3E, 0x3E, 0x3E, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B}},
		{'З', {0x1C, 0x1C, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x3, 0x3, 0x3, 0x3, 0x1E, 0x1C, 0x1C, 0x1E, 0x3, 0x3, 0x3, 0x3, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x1C, 0x1C}},
		{'И', {0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x67, 0x67, 0x67, 0x67, 0x6B, 0x6B, 0x6B, 0x6B, 0x73, 0x73, 0x73, 0x73, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63}},
		{'Й', {0x6B, 0x6B, 0x6B, 0x6B, 0x63, 0x63, 0x63, 0x63, 0x67, 0x67, 0x67, 0x67, 0x6B, 0x6B, 0x6B, 0x6B, 0x73, 0x73, 0x73, 0x73, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63}},
		{'К', {0x63, 0x63, 0x63, 0x63, 0x66, 0x66, 0x66, 0x66, 0x6C, 0x6C, 0x6C, 0x6C, 0x78, 0x78, 0x78, 0x78, 0x6C, 0x6C, 0x6C, 0x6C, 0x66, 0x66, 0x66, 0x66, 0x63, 0x63, 0x63, 0x63}},
		{'Л', {0xF, 0xF, 0x1F, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x73, 0x73, 0x73, 0x73}},
		{'М', {0x41, 0x41, 0x63, 0x63, 0x77, 0x77, 0x77, 0x77, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63}},
		{'Н', {0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7F, 0x7F, 0x7F, 0x7F, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63}},
		{'О', {0x1C, 0x1C, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x1C, 0x1C}},
		{'П', {0x7F, 0x7F, 0x7F, 0x7F, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63}},
		{'P', {0x7C, 0x7C, 0x7E, 0x7E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x7E, 0x7C, 0x7C, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60}},
		{'C', {0x1C, 0x1C, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x1C, 0x1C}},
		{'Т', {0x7F, 0x7F, 0x7F, 0x7F, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C}},
		{'У', {0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3F, 0x3F, 0x1F, 0x1F, 0x3, 0x3, 0x3, 0x3, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x1C, 0x1C}},
		{'Ф', {0x1C, 0x1C, 0x3E, 0x3E, 0x6B, 0x6B, 0x6B, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x6B, 0x6B, 0x6B, 0x3E, 0x3E, 0x3E, 0x3E, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C}},
		{'Х', {0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x36, 0x36, 0x36, 0x36, 0x1C, 0x1C, 0x1C, 0x1C, 0x36, 0x36, 0x36, 0x36, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63}},
		{'Ц', {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7E, 0x7E, 0x3E, 0x3E, 0x3, 0x3, 0x3, 0x3}},
		{'Ч', {0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3F, 0x3F, 0x1F, 0x1F, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3}},
		{'Ш', {0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x7F, 0x7F, 0x7F, 0x7F}},
		{'Щ', {0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x6B, 0x7F, 0x7F, 0x7F, 0x7F, 0x3, 0x3, 0x3, 0x3}},
		{'Ъ', {0x70, 0x70, 0x70, 0x70, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3E, 0x3E, 0x3E, 0x3E, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3E, 0x3E, 0x3E, 0x3E}},
		{'Ы', {0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x73, 0x73, 0x7B, 0x7B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x7B, 0x7B, 0x73, 0x73}},
		{'Ь', {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7C, 0x7C, 0x7E, 0x7E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x7E, 0x7C, 0x7C}},
		{'Э', {0x3E, 0x3E, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x3, 0x3, 0x3, 0x3, 0xF, 0xF, 0xF, 0xF, 0x3, 0x3, 0x3, 0x3, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x3E, 0x3E}},
		{'Ю', {0x66, 0x66, 0x66, 0x66, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x79, 0x79, 0x79, 0x79, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x66, 0x66, 0x66, 0x66}},
		{'Я', {0x1F, 0x1F, 0x3F, 0x3F, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3F, 0x3F, 0x1F, 0x1F, 0xF, 0xF, 0x1B, 0x1B, 0x33, 0x33, 0x33, 0x33, 0x63, 0x63, 0x63, 0x63}},
		{'d', {0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x1F, 0x1F, 0x3F, 0x3F, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3F, 0x3F, 0x1F, 0x1F}},
		{'0', {0x1C, 0x1C, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x1C, 0x1C}},
		{'1', {0xC, 0xC, 0x1C, 0x1C, 0x7C, 0x7C, 0x7C, 0x7C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x7F, 0x7F, 0x7F, 0x7F}},
		{'2', {0x1C, 0x1C, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x3, 0x3, 0x3, 0x3, 0x6, 0x6, 0xE, 0xE, 0x1C, 0x1C, 0x18, 0x18, 0x30, 0x30, 0x30, 0x30, 0x7F, 0x7F, 0x7F, 0x7F}},
		{'3', {0x7F, 0x7F, 0x7F, 0x7F, 0xC, 0xC, 0xC, 0xC, 0x18, 0x18, 0x18, 0x18, 0xC, 0xC, 0xE, 0xE, 0x7, 0x7, 0x3, 0x3, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x3E, 0x3E}},
		{'4', {0xE, 0xE, 0xE, 0xE, 0x1E, 0x1E, 0x1E, 0x1E, 0x36, 0x36, 0x36, 0x36, 0x66, 0x66, 0x66, 0x66, 0x7F, 0x7F, 0x7F, 0x7F, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6}},
		{'5', {0x7F, 0x7F, 0x7F, 0x7F, 0x60, 0x60, 0x60, 0x60, 0x7C, 0x7C, 0x3E, 0x3E, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x3E, 0x3E}},
		{'6', {0x1C, 0x1C, 0x1C, 0x1C, 0x30, 0x30, 0x30, 0x30, 0x60, 0x60, 0x60, 0x60, 0x7C, 0x7C, 0x7E, 0x7E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x3E, 0x3E}},
		{'7', {0x7F, 0x7F, 0x7F, 0x7F, 0x3, 0x3, 0x7, 0x7, 0xE, 0xE, 0xC, 0xC, 0x18, 0x18, 0x18, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30}},
		{'8', {0x3E, 0x3E, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x3E, 0x3E, 0x3E}},
		{'9', {0x1C, 0x1C, 0x3E, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3F, 0x3F, 0x1F, 0x1F, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x63, 0x63, 0x3E, 0x3E, 0x1C, 0x1C}},
		{'-', {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7F, 0x7F, 0x7F, 0x7F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}},
		{' ', {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}
};


const unsigned char picture_tab[]={};



/*============Функция задержки мс==================*/
void Delay_ms(uint32_t Milliseconds);



/*============Настройка TMR32, TMR1==================*/
void TMR32_init(uint32_t *SystemCoreClock);
void TMR1_init();



/*============Настройка обработчика прерываний TMR32, TMR1==================*/
void TMR32_IRQHandler();
void TMR1_IRQHandler();



/*============Настройка GPIO==================*/
void GPIO_init();



/*============Настройка обработчика прерывания GPIO==================*/
void GPIO_IRQHandler();

/*Нажали "конфигурация" = прерывание отключает возможность листать влево\вправо; while с флагом, чтобы переменная мигала; можно листать вверх\вниз, подтверждать выбор (через прерывания этих кнопок)
 * Нажали "вверх\вниз" = включены только после нажатия кнопки "конфигурация"; сменяют переменную
 * Нажали "подтвердить" = отключается листание вверх\вниз, переменная перестает мигать (остается только курсор)*/


/*============Настройка UART2==================*/
void UART2_init();



/*============Настройка обработчика прерывания по приему UART2==================*/
void UART2_IRQHandler();



/*============Настройка SPI0==================*/
void SPI0_init();



/*============Глобальная настройка DMA SPI0==================*/
void DMA_GLOBAL_init();



/*============Настройка обработчика прерывания DMA по приему\передаче UART2==================*/
void DMA_CH_13_IRQHandler();
void DMA_CH_10_IRQHandler();



/*============Блок парсера==================*/
uint8_t valid_addres(uint8_t addr);


uint8_t get_cmd_len(uint8_t cmd);


void parserByte (uint8_t byte);



/*============Функции парсера==================*/
void redrawWindow1 (uint8_t* data, char* str, uint8_t size);


void ledOFF (uint8_t* data, char* str, uint8_t size);


void displayFull (uint8_t* data, char* str, uint8_t size);


void displayClear (uint8_t* data, char* str, uint8_t size);


void UART2_TX (uint8_t* data, char* str, uint8_t size);


typedef void (*functions)(uint8_t* data, char* str, uint8_t size);

functions table[] =
{
		[0x21] = redrawWindow1,
		[0x22] = ledOFF,
		[0x31] = displayFull,
		[0x32] = displayClear,
		[0x41] = UART2_TX
};



/*============Функция передачи SPI0==================*/
void SPI0_send(uint8_t data);



/*============Блок работы с дисплеем==================*/
void OLED_send_command(uint8_t cmd);


void OLED_send_data(uint8_t cmd);


void OLED_Init();


const uint32_t* FindBitmap(char c);


void OLED_full();


void OLED_clear();


void OLED_SetCursor(uint8_t column, uint8_t row);


void Picture_display(const unsigned char *ptr_pic);


void DMA_Print_Row(uint8_t *data, uint16_t size);


void DMA_Collect_Row(uint8_t x, uint8_t y, const char* str);


void Draw_Cursor(uint8_t x, uint8_t y, uint8_t state);


void Inversion_Square(uint8_t state);


uint8_t getIndex(uint8_t x, uint8_t y);

void FLASH_Window_init(char* str, uint8_t size, char* str1, uint8_t size1);



int main(void)
{
	SystemInit();

	SystemCoreClockUpdate();


	GPIO_init();

	TMR32_init(&SystemCoreClock);

	TMR1_init();

	UART2_init();

    SPI0_init();

    DMA_GLOBAL_init();


	/*============Разрешение прерываний (!разобраться!)==================*/
    InterruptEnable();


    /*============Запуск экранчика==================*/
    OLED_Init();
    OLED_clear();
    Picture_display(picture_tab);


    /*============Берем данные из памяти для актуализации==================*/
    FLASH_Window_init(window_1, sizeof(window_1), window1_12, sizeof(window1_12));



	Delay_ms(1500);
    OLED_clear();
    DMA_Collect_Row(0, 0, window_1);


    while(1)
    {
    	while (start_write_func != start_exe_func)
    	{
    		table[packet[start_exe_func].cmd](packet[start_exe_func].data, window_1, sizeof(window_1));

    		start_exe_func = (start_exe_func + 1) % QUEUE_SIZE;
    	}


    	Draw_Cursor(cursor_x, cursor_y, 1);
    	Delay_ms(200);
    	Draw_Cursor(cursor_x, cursor_y, 0);
    	Delay_ms(200);


    	while (isEditing)
    	{
    		Inversion_Square(0);
    		Delay_ms(200);
    		Inversion_Square(1);
    		Delay_ms(200);
    	}


    	if(need_window1)
    	{
			uint32_t data_FLASH;


		    FLASH->ADDR = MEM_FLASH_BASE + MEM_FLASH_PAGE_SIZE * (MEM_FLASH_PAGE_TOTAL - 1);
		    FLASH->CMD = (0xC0DE << 16) | (1 << 2);//ключ + чистка области
		    while (FLASH->STAT & 1);//ожидание конца


		    FLASH->ADDR = MEM_FLASH_BASE + MEM_FLASH_PAGE_SIZE * (MEM_FLASH_PAGE_TOTAL - 1);//2 страница флеш-памяти
		    FLASH->DATA[0].DATA = (uint8_t)window1_12[current_window1_1] | ((uint8_t)window1_12[current_window1_2] << 8);
		    FLASH->CMD = (0xC0DE << 16) | (1 << 1);//ключ + запись в область
		    while (FLASH->STAT & 1);//ожидание конца

			need_window1 = 0;
			GPIOC->DATAOUTTGL = 1;
    	}
    };
    return 0;
}



/*============Функция задержки мс==================*/
void Delay_ms(uint32_t Milliseconds)
{
	Delay_counter_ms = Milliseconds;
	while (Delay_counter_ms != 0);
}



/*============Настройка TMR32==================*/
void TMR32_init(uint32_t *SystemCoreClock)
{
	RCU->CGCFGAPB |= 1;
	RCU->RSTDISAPB |= 1;

	TMR32->CAPCOM[0].VAL = (*SystemCoreClock / 1000) - 1;//Период отсчета
	TMR32->COUNT = (TMR32->COUNT & ~0xFFFFFFFF) | ((*SystemCoreClock / 1000) - 1);//Текущее значение таймера

	TMR32->CTRL = (TMR32->CTRL & ~(0b00 << 4)) | (0b01 << 4);//режим счета вверх
	TMR32->CTRL &= ~(0b1 << 8);//источник тактирования sysclk
	TMR32->CTRL &= ~(0b00 << 6);//деление на 1

	TMR32->IM |= 1 << 1;//Разрешаем прерывание по совпадению значения счетчика и CAPCOM[0]

	// Настраиваем обработчик прерывания для TMR32
	PLIC_SetIrqHandler (Plic_Mach_Target, IsrVect_IRQ_TMR32, TMR32_IRQHandler);
	PLIC_SetPriority   (IsrVect_IRQ_TMR32, 0x1);
	PLIC_IntEnable     (Plic_Mach_Target, IsrVect_IRQ_TMR32);
}



/*============Настройка обработчика прерываний TMR32==================*/
void TMR32_IRQHandler()
{
	SysTimer_ms++;

	if (Delay_counter_ms){
		Delay_counter_ms--;
	}
    //Сбрасываем флаг прерывания таймера
	TMR32->IC = 3;
}



/*============Настройка TMR1==================*/
void TMR1_init()
{
	RCU->CGCFGAPB |= 1 << 2;
	RCU->RSTDISAPB |= 1 << 2;

	TMR1->CTRL |= (TMR1->CTRL & ~(0b11 << 4)) | (0b01 << 4);//режим счета вверх
	TMR1->CTRL &= ~(0b1 << 8);//источник тактирования sysclk
	TMR1->CTRL &= ~(0b00 << 6);//деление на 1

	TMR1->IM |= 1 << 1;//Разрешаем прерывание по совпадению значения счетчика и CAPCOM[0]

	// Настраиваем обработчик прерывания для TMR1
	PLIC_SetIrqHandler (Plic_Mach_Target, IsrVect_IRQ_TMR1, TMR1_IRQHandler);
	PLIC_SetPriority   (IsrVect_IRQ_TMR1, 0x1);
	PLIC_IntEnable     (Plic_Mach_Target, IsrVect_IRQ_TMR1);
}



/*============Настройка обработчика прерываний TMR1==================*/
void TMR1_IRQHandler()
{
	TMR1->CAPCOM[0].VAL = 0;//Период отсчета

	UART2->DMACR &= ~1;//отключаем разрешение обслуживания буфера приемника\


	if (DMA->PRIALTSET & (1 << 13))//если активна альтернатива -> значит буфер PONG начал заполняться
	{
		uint32_t rem = DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG_bit.N_MINUS_1 + 1;
		uint32_t bytes_received = DMA_PING_PONG_SIZE - rem;

		if (bytes_received > 0)
		{
			memcpy(&parser_buf[start_for_DMA], DMA_PONG, bytes_received);
			start_for_DMA = (start_for_DMA + bytes_received) % PARSER_BUF_SIZE;
		}

		DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b011 << 29;//канал в режиме пинг-понг
		DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= (DMA_PING_PONG_SIZE-1) << 5;//1 передач
		DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b0000 << 1;//арбитраж после каждой
	}


	else//если активна основая -> значит буфер PING начал заполняться
	{
		uint32_t rem = DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG_bit.N_MINUS_1 + 1;
		uint32_t bytes_received = DMA_PING_PONG_SIZE - rem;

		if (bytes_received > 0)
		{
			memcpy(&parser_buf[start_for_DMA], DMA_PING, bytes_received);
			start_for_DMA = (start_for_DMA + bytes_received) % PARSER_BUF_SIZE;
		}

		DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b011 << 29;//канал в режиме пинг-понг
		DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= (DMA_PING_PONG_SIZE-1) << 5;//1 передач
		DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b0000 << 1;//арбитраж после каждой

	}


	while (start_for_parser != start_for_DMA)
	{
		parserByte(parser_buf[start_for_parser]);
		start_for_parser = (start_for_parser + 1) % PARSER_BUF_SIZE;
	}


	UART2->IMSC |= 1 << 4;//разрешаем именно прерывания UART

    //Сбрасываем флаг прерывания таймера
	TMR1->IC |= 3;
}



/*============Настройка GPIO==================*/
void GPIO_init()
{
	//Включение и тактирование было прописано в CLOCKOUT
	GPIOC->OUTENSET |= 1;//C0 - можем управлять выводом
	GPIOC->DATAOUTCLR = 1;//C0 - низкий уровень


	//настройка входа порта С1
	GPIOC->ALTFUNCCLR = 1 << 1;
	GPIOC->OUTENCLR = 1 << 1;
	GPIOC->OUTMODE &= ~(0b11 << 2);
	GPIOC->PULLMODE |= 1 << 1;


	//настройка входа порта С2
	GPIOC->ALTFUNCCLR = 1 << 2;
	GPIOC->OUTENCLR = 1 << 2;
	GPIOC->OUTMODE &= ~(0b11 << 4);
	GPIOC->PULLMODE |= 1 << 2;


	//настройка входа порта С3
	GPIOC->ALTFUNCCLR = 1 << 3;
	GPIOC->OUTENCLR = 1 << 3;
	GPIOC->OUTMODE &= ~(0b11 << 6);
	GPIOC->PULLMODE |= 1 << 3;


	//настройка входа порта С4
	GPIOC->ALTFUNCCLR = 1 << 4;
	GPIOC->OUTENCLR = 1 << 4;
	GPIOC->OUTMODE &= ~(0b11 << 8);
	GPIOC->PULLMODE |= 1 << 4;


	//Настройка прерывания для порта кнопки
	GPIOC->INTTYPESET |= 0b1111 << 1;//прерывание по фронту
	GPIOC->INTPOLCLR |= 0b1111 << 1;//по отрицательному фронту
	GPIOC->INTENSET |= 0b11 << 1;//включение прерывания

    PLIC_SetIrqHandler (Plic_Mach_Target, IsrVect_IRQ_GPIO, GPIO_IRQHandler);
    PLIC_SetPriority   (IsrVect_IRQ_GPIO, 0x1);
    PLIC_IntEnable     (Plic_Mach_Target, IsrVect_IRQ_GPIO);
}



/*============Настройка обработчика прерывания GPIO==================*/
void GPIO_IRQHandler()
{
	if (GPIOC->INTSTATUS & (1 << 1))//Переключение в режим ввода\Конфигурация
	{
		if ((SysTimer_ms - gpio_btn1) > 150)
		{
			isEditing = 1;

			GPIOC->INTENCLR |= 1 << 1;//Выключаем вход в Конфигурацию
			GPIOC->INTENCLR |= 1 << 2;//Выключаем перелистывания горизонта
			GPIOC->INTENSET |= 1 << 3;//Включаем перелистывание вертикали
			GPIOC->INTENSET |= 1 << 4;//Включаем подтверждение ввода

			gpio_btn1 = SysTimer_ms;
		}

		GPIOC->INTSTATUS = 1 << 1;
	}



	if (GPIOC->INTSTATUS & (1 << 2))//Пролистывание вправо
	{
		if ((SysTimer_ms - gpio_btn2) > 150)
		{
			Draw_Cursor(cursor_x, cursor_y, 0);

			switch (current_window)
			{
			case WINDOW_1:
				cursor_x = 0;
				cursor_y = cursor_y > 32 ? 29 : 61;
			}

			Draw_Cursor(cursor_x, cursor_y, 1);


			gpio_btn2 = SysTimer_ms;
		}

		GPIOC->INTSTATUS = 1 << 2;
	}



	if (GPIOC->INTSTATUS & (1 << 3))//Пролистывание вверх
	{
		if ((SysTimer_ms - gpio_btn3) > 150)
		{
			switch (current_window)
			{
			case WINDOW_1:

				if (cursor_y < 32)
				{
					current_window1_1 = (current_window1_1 + 1) % 5;
					window_1[getIndex(cursor_x, cursor_y)] = window1_12[current_window1_1];
					DMA_Collect_Row(cursor_x, cursor_y - 29, (char[]){window_1[getIndex(cursor_x, cursor_y)], '\0'});
				}
				else
				{
					current_window1_2 = (current_window1_2 + 1) % 5;
					window_1[getIndex(cursor_x, cursor_y)] = window1_12[current_window1_2];
					DMA_Collect_Row(cursor_x, cursor_y - 29, (char[]){window_1[getIndex(cursor_x, cursor_y)], '\0'});
				}

				break;
			}

			gpio_btn3 = SysTimer_ms;
		}

		GPIOC->INTSTATUS = 1 << 3;
	}



	if (GPIOC->INTSTATUS & (1 << 4))//Подтверждение ввода
	{
		if ((SysTimer_ms - gpio_btn4) > 150)
		{
			isEditing = 0;
			need_window1 = 1;


			GPIOC->INTENSET |= 1 << 1;//Включаем вход в Конфигурацию
			GPIOC->INTENSET |= 1 << 2;//Включаем перелистывания горизонта
			GPIOC->INTENCLR |= 1 << 3;//Выключаем перелистывание вертикали
			GPIOC->INTENCLR |= 1 << 4;//Выключаем подтверждение ввода


			gpio_btn4 = SysTimer_ms;
		}

		GPIOC->INTSTATUS = 1 << 4;
	}
}



/*============Настройка UART2==================*/
void UART2_init()
{
    uint32_t baud_icoef = 16000000 / (16 * UART2_BAUD);
    uint32_t baud_fcoef = ((16000000 / (16.0f * RETARGET_UART_BAUD) - baud_icoef) * 64 + 0.5f);


    // Настраиваем GPIO
    RCU->CGCFGAHB |= 1 << 8;//Разрешение тактирования порта А
    RCU->RSTDISAHB |= 1 << 8;//Включения порта А

    RCU->CGCFGAPB |= 1 << 8;//Разрешение тактирования блока UART2
    RCU->RSTDISAPB |= 1 << 8;//Включение блока UART2

    GPIOA->ALTFUNCNUM |= 0b11 << 24;//Выбор алтернативы для 14 порта - uart
    GPIOA->ALTFUNCNUM |= 0b11 << 26;//Выбор алтернативы для 15 порта - uart
    GPIOA->ALTFUNCSET |= 0b11 << 12;//Включение альтернативы для портов - uart


    // Настраиваем UART2
    RCU->UARTCLKCFG[2].UARTCLKCFG_bit.CLKSEL = RCU_UARTCLKCFG_CLKSEL_HSE;
    RCU->UARTCLKCFG[2].UARTCLKCFG_bit.DIVEN = 0;
    RCU->UARTCLKCFG[2].UARTCLKCFG_bit.RSTDIS = 1;
    RCU->UARTCLKCFG[2].UARTCLKCFG_bit.CLKEN = 1;

    UART2->IBRD = baud_icoef;//целая часть делителя скорости
    UART2->FBRD = baud_fcoef;//дробная часть делителя скорости
    UART2->LCRH |= (0b11 << 5);//8 информационных бит
    UART2->LCRH &= ~(0b1 << 4);//включение режима FIFO буфера приема\передачи
    UART2->IFLS &= ~(0b111111);//порог прерывания на 1\8 буфера

    UART2->IMSC |= 1 << 4;

    UART2->CR |= 0b11 << 8;//Разрешение приема\передачи
    UART2->CR |= 1;//Разрешение работы приемопередатчика


    // Настраиваем обработчик прерывания для UART2
    PLIC_SetIrqHandler (Plic_Mach_Target, IsrVect_IRQ_UART2, UART2_IRQHandler);
    PLIC_SetPriority   (IsrVect_IRQ_UART2, 0x1);
    PLIC_IntEnable     (Plic_Mach_Target, IsrVect_IRQ_UART2);
}



/*============Настройка обработчика прерывания по приему UART2==================*/
void UART2_IRQHandler()
{
    if (UART2->RIS & (1 << 4))
    {
    	UART2->IMSC &= ~(1 << 4);//Запрет на прерывания UART
    	UART2->DMACR |= 1;//Разрешаем обслуживание буфера приемника UART

		TMR1->COUNT = (TMR1->COUNT & ~0xFFFFFFFF) | (65000);//Текущее значение таймера
		TMR1->CAPCOM[0].VAL = 65000;//Период отсчета

    	UART2->ICR |= 1 << 4;//Сброс прерывания
    }
}



/*============Настройка SPI0==================*/
void SPI0_init()
{
    //B0 - CLK\SCL, B3 - Tx\SDA, B4 - RES, B5 - CS, B6 - DC
    RCU->CGCFGAHB |= 1 << 9;//Разрешение тактирования порта В
    RCU->RSTDISAHB |= 1 << 9;//Включение порта В

    RCU->CGCFGAHB |= 1 << 5;//Разрешение тактирования блока SPI0
    RCU->RSTDISAHB |= 1 << 5;//Включение блока SPI0


    GPIOB->ALTFUNCNUM = (GPIOB->ALTFUNCNUM & ~(0b11111111)) | 0b01010101;//Выбор алтернативы для 0-3 порта - SPI0
    GPIOB->ALTFUNCSET |= 0b1111;//Включение альтернативы для портов - SPI0


    GPIOB->ALTFUNCCLR = 0b111 << 4;//выключение альтернативы B4-B6
	GPIOB->OUTENSET |= 0b111 << 4;//B4-B6 - можем управлять выводом
	GPIOB->DATAOUTSET = 0b111 << 4;//B4-B6 - высокий уровень


    RCU->SPICLKCFG[0].SPICLKCFG_bit.CLKSEL = RCU_SPICLKCFG_CLKSEL_HSE;//Источник тактов
    RCU->SPICLKCFG[0].SPICLKCFG_bit.DIVN = 1;//Коэф деления
    RCU->SPICLKCFG[0].SPICLKCFG_bit.DIVEN = 1;//включение делителя
    RCU->SPICLKCFG[0].SPICLKCFG_bit.RSTDIS = 1;//Снятие сброса
    RCU->SPICLKCFG[0].SPICLKCFG_bit.CLKEN = 1;//Разрешение тактирования


    SPI0->CPSR = 2;


    SPI0->CR0 &= ~(0b11111111 << 8);//коэф второго делителя
    SPI0->CR0 &= ~(1 << 7);//фаза сигнала
    SPI0->CR0 &= ~(0b11 << 4);//протокол SPI
    SPI0->CR0 = (SPI0->CR0 & ~(0b1111)) | 0b0111;//размер 8 бит

    SPI0->CR1 &= ~(1 << 2);//режим Мастера
    SPI0->CR1 |= 1 << 1;//разрешение работы ресивера
}



/*============Глобальная настройка DMA==================*/
void DMA_GLOBAL_init()
{
	DMA->CFG |= 1 << 3;//Включение контроллера DMA


	/*============DMA UART==================*/
	//Основная структура, прием, канал 13
	DMA_CONFIGDATA.PRM_DATA.CH[13].SRC_DATA_END_PTR |= (uint32_t)(&UART2->DR);//источник приема
	DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b00 << 17;
	DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b11 << 15;

	DMA_CONFIGDATA.PRM_DATA.CH[13].DST_DATA_END_PTR |= (uint32_t)&(DMA_PING[DMA_PING_PONG_SIZE-1]);//конечный пункт приема
	DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b00 << 21;
	DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b00 << 19;

	DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b011 << 29;//канал в режиме пинг-понг
	DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= (DMA_PING_PONG_SIZE-1) << 5;//1 передач
	DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b0000 << 1;//арбитраж после каждой


	//Основная структура, передача, канал 10
	DMA_CONFIGDATA.PRM_DATA.CH[10].CHANNEL_CFG |= 0b00 << 17;
	DMA_CONFIGDATA.PRM_DATA.CH[10].CHANNEL_CFG |= 0b00 << 15;

	DMA_CONFIGDATA.PRM_DATA.CH[10].DST_DATA_END_PTR |= (uint32_t)(&UART2->DR);//конечный пункт приема
	DMA_CONFIGDATA.PRM_DATA.CH[10].CHANNEL_CFG |= 0b00 << 21;
	DMA_CONFIGDATA.PRM_DATA.CH[10].CHANNEL_CFG |= 0b11 << 19;


	//Альтернативная структура, прием, канал 13
	DMA_CONFIGDATA.ALT_DATA.CH[13].SRC_DATA_END_PTR |= (uint32_t)(&UART2->DR);//источник приема
	DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b00 << 17;
	DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b11 << 15;

	DMA_CONFIGDATA.ALT_DATA.CH[13].DST_DATA_END_PTR |= (uint32_t)&(DMA_PONG[DMA_PING_PONG_SIZE-1]);//конечный пункт приема
	DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b00 << 21;
	DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b00 << 19;

	DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b011 << 29;//канал в режиме пинг-понг
	DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= (DMA_PING_PONG_SIZE-1) << 5;//1 передач
	DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b0000 << 1;//арбитраж после каждой


	//Инициализация DMA
	DMA->ENSET |= 1 << 13;//Включение 13 канала


    //Прерывания DMA (10 канал)
    PLIC_SetIrqHandler (Plic_Mach_Target, IsrVect_IRQ_DMA3, DMA_CH_10_IRQHandler);
    PLIC_SetPriority   (IsrVect_IRQ_DMA3, 0x1);
    PLIC_IntEnable     (Plic_Mach_Target, IsrVect_IRQ_DMA3);

    //Прерывания DMA (13 канал)
    PLIC_SetIrqHandler (Plic_Mach_Target, IsrVect_IRQ_DMA4, DMA_CH_13_IRQHandler);
    PLIC_SetPriority   (IsrVect_IRQ_DMA4, 0x1);
    PLIC_IntEnable     (Plic_Mach_Target, IsrVect_IRQ_DMA4);



	/*============DMA SPI0==================*/
    DMA_CONFIGDATA.PRM_DATA.CH[17].CHANNEL_CFG |= 0b00 << 17;//SRC_SIZE по байтово
    DMA_CONFIGDATA.PRM_DATA.CH[17].CHANNEL_CFG |= 0b00 << 15;//SRC_INC по байтово

    DMA_CONFIGDATA.PRM_DATA.CH[17].DST_DATA_END_PTR |= (uint32_t)(&SPI0->DR);
    DMA_CONFIGDATA.PRM_DATA.CH[17].CHANNEL_CFG |= 0b00 << 21;//DST_SIZE
    DMA_CONFIGDATA.PRM_DATA.CH[17].CHANNEL_CFG |= 0b11 << 19;//DST_INC отключен

    DMA_CONFIGDATA.PRM_DATA.CH[17].CHANNEL_CFG |= 0x0 << 1;//арбитраж через 1 передачу

}



/*============Настройка обработчика прерывания DMA по приему\передаче UART2==================*/
void DMA_CH_13_IRQHandler()
{
	if (DMA->IRQSTAT & (1 << 13))
	{
		DMA->IRQSTATCLR = 1 << 13;

		TMR1->CAPCOM[0].VAL = 0;//Период отсчета

		if (DMA->PRIALTSET & (1 << 13))//если активна альтернатива -> была основная -> заполнен буфер PING
		{
			DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b011 << 29;//канал в режиме пинг-понг
			DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= (DMA_PING_PONG_SIZE-1) << 5;//1 передач
			DMA_CONFIGDATA.PRM_DATA.CH[13].CHANNEL_CFG |= 0b0000 << 1;//арбитраж после каждой

			memcpy(&parser_buf[start_for_DMA], DMA_PING, DMA_PING_PONG_SIZE);

		}
		else//если активна основая -> была алтернатива -> заполнен буфер PONG
		{
			DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b011 << 29;//канал в режиме пинг-понг
			DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= (DMA_PING_PONG_SIZE-1) << 5;//1 передач
			DMA_CONFIGDATA.ALT_DATA.CH[13].CHANNEL_CFG |= 0b0000 << 1;//арбитраж после каждой

			memcpy(&parser_buf[start_for_DMA], DMA_PONG, DMA_PING_PONG_SIZE);

		}

		start_for_DMA = (start_for_DMA + DMA_PING_PONG_SIZE) % PARSER_BUF_SIZE;

		TMR1->COUNT = (TMR1->COUNT & ~0xFFFFFFFF) | (65000);//Текущее значение таймера
		TMR1->CAPCOM[0].VAL = 65000;//Период отсчета
	}
}


void DMA_CH_10_IRQHandler()
{
	if (DMA->IRQSTAT & (1 << 10))
	{
		UART2->DMACR &= ~(1 << 1);

		DMA->IRQSTATCLR = 1 << 10;
	}
}



/*============Блок парсера==================*/
uint8_t valid_addres(uint8_t addr)//Проверка правильности адреса
{
	for (uint8_t i = 0; i < sizeof(validAddr); i++)
	{
		if (addr == validAddr[i]) return 1;
	}

	return 0;
}


uint8_t get_cmd_len(uint8_t cmd)//Соответсвие функции ее длине
{
	for (uint8_t i = 0; i < sizeof(validCMD); i++)
	{
		if (cmd == validCMD[i]) return cmd_data_lens[i];
	}

	return 0xFF;
}


void parserByte (uint8_t byte)
{
	switch (parser_state)
	{
	case 0:
		if (byte == MARKER_START)
		{
			parser_state = 1;
			sum_calc = byte;
		}

		break;

	case 1:
		if (valid_addres(byte))
		{
			parser_state = 2;
			packet[start_write_func].addr = byte;
			sum_calc ^= byte;
		}

		else parser_state = 0;

		break;

	case 2:
		expected_len_data = get_cmd_len(byte);

		if (expected_len_data == 0xFF) parser_state = 0;

		else
		{
			real_len_data = 0;
			packet[start_write_func].cmd = byte;
			sum_calc ^= byte;
			parser_state = (expected_len_data > 0) ? 3 : 4;
		}

		break;

	case 3:
		packet[start_write_func].data[real_len_data] = byte;
		real_len_data++;
		sum_calc ^= byte;

		if (real_len_data >= expected_len_data) parser_state = 4;

		break;

	case 4:
		if (sum_calc == byte)
			{
				packet[start_write_func].data_len = expected_len_data;

				start_write_func = (start_write_func + 1) % QUEUE_SIZE;
			}

		parser_state = 0;
		break;
	}
}



/*============Функции парсера==================*/
void redrawWindow1 (uint8_t* data, char* str, uint8_t size)
{
	uint32_t data_FLASH;


    FLASH->ADDR = MEM_FLASH_BASE + MEM_FLASH_PAGE_SIZE * (MEM_FLASH_PAGE_TOTAL - 2);
    FLASH->CMD = (0xC0DE << 16) | (1 << 2);//ключ + чистка области
    while (FLASH->STAT & 1);//ожидание конца


    FLASH->ADDR = MEM_FLASH_BASE + MEM_FLASH_PAGE_SIZE * (MEM_FLASH_PAGE_TOTAL - 2);//1 страница флеш-памяти
    FLASH->DATA[0].DATA = data[0] | (data[1] << 4) | (data[2] << 8) | (data[3] << 12) | (data[4] << 16)
    		| (data[5] << 20) | (data[6] << 24) | (data[7] << 28);
    FLASH->CMD = (0xC0DE << 16) | (1 << 1);//ключ + запись в область
    while (FLASH->STAT & 1);//ожидание конца


	str[7] = data[0] + '0';
	str[8] = data[1] + '0';
	str[14] = data[2] + '0';
	str[15] = data[3] + '0';
	str[23] = data[4] + '0';
	str[24] = data[5] + '0';
	str[30] = data[6] + '0';
	str[31] = data[7] + '0';

	if (current_window == WINDOW_1) DMA_Collect_Row(0, 0, str);
}


void ledOFF (uint8_t* data, char* str, uint8_t size)
{
	for (uint8_t i = 0; i < 5; i++)
	{
		GPIOC->DATAOUTCLR = 1;
		Delay_ms(100);
		GPIOC->DATAOUTSET = 1;
		Delay_ms(100);
	}
}


void displayFull (uint8_t* data, char* str, uint8_t size)
{
	for (uint8_t i = 0; i < 16; i++)
	{
		OLED_full();
		Delay_ms(100);
		OLED_clear();
		Picture_display(picture_tab);
	}
}


void displayClear (uint8_t* data, char* str, uint8_t size)
{
	OLED_clear();
}


void UART2_TX (uint8_t* data, char* str, uint8_t size)
{
	uint8_t special[] =
	{
			0x1, 0x5
	};

	DMA_CONFIGDATA.PRM_DATA.CH[10].SRC_DATA_END_PTR = (uint32_t)&(special[sizeof(special) - 1]);

	DMA_CONFIGDATA.PRM_DATA.CH[10].CHANNEL_CFG |= 0b001 << 29;//канал в режиме основном
	DMA_CONFIGDATA.PRM_DATA.CH[10].CHANNEL_CFG |= (sizeof(special) - 1) << 5;//3 передачи
	DMA_CONFIGDATA.PRM_DATA.CH[10].CHANNEL_CFG |= 0b0000 << 1;//арбитраж после каждой

	DMA->ENSET |= 1 << 10;//Включение 10 канала

	UART2->DMACR |= 1 << 1;//Разрешаем работу с буфером передатчика (ЖДЕМ ОТМАШКИ НА ПЕРЕДАЧУ)

}



/*============Функция передачи SPI0==================*/
void SPI0_send(uint8_t data)
{
    while (SPI0->SR & (1 << 4));// Ждем, пока буфер освободится
    SPI0->DR = data;
    while (SPI0->SR & (1 << 4));// Ждем окончания передачи
}



/*============Блок работы с дисплеем==================*/
void OLED_send_command(uint8_t cmd)
{
	GPIOB->DATAOUTCLR = 1 << 6;//B6\DC низкий уровень
	GPIOB->DATAOUTCLR = 1 << 5;//B5\CS низкий уровень

	SPI0_send(cmd);

	GPIOB->DATAOUTSET = 1 << 5;//B5\CS высокий уровень
}


void OLED_send_data(uint8_t cmd)
{
	GPIOB->DATAOUTSET = 1 << 6;//B6\DC высокий уровень
	GPIOB->DATAOUTCLR = 1 << 5;//B5\CS низкий уровень

	SPI0_send(cmd);

	GPIOB->DATAOUTSET = 1 << 5;//B5\CS высокий уровень
}


void OLED_Init()
{
    // Жесткий обязательный сброс перед запуском
    GPIOB->DATAOUTSET = 1 << 4;//B4\RES высокий уровень

    Delay_ms(100); // пауза
    GPIOB->DATAOUTCLR = 1 << 4;//B4\RES низкий уровень
    Delay_ms(100); // пауза
    GPIOB->DATAOUTSET = 1 << 4;//B4\RES высокий уровень
    Delay_ms(100);

    OLED_send_command(0xFD); //SET COMMAND LOCK
	OLED_send_data(0x12);//UNLOCK

    OLED_send_command(0xAE);//Sleep mode ON

	OLED_send_command(0xB3);//DISPLAY DIVIDE CLOCKRADIO/OSCILLATAR FREQUANCY
	OLED_send_data(0x91);

	OLED_send_command(0xCA);	//multiplex ratio
	OLED_send_data(0x3F);   //duty = 1/64

	OLED_send_command(0xA2);    //set offset
    OLED_send_data(0x00);

	OLED_send_command(0xA1);	//start line
	OLED_send_data(0x00);

	OLED_send_command(0xA0);  //set remap
	OLED_send_data(0x14);
	OLED_send_data(0x11);

	OLED_send_command(0xAB);	//funtion selection
	OLED_send_data(0x01);	//Enable internal VDD regulator

	OLED_send_command(0xB4);
	OLED_send_data(0xA0);
	OLED_send_data(0xFD);

	OLED_send_command(0xC1);	//set contrast current
	OLED_send_data(0x01);

	OLED_send_command(0xC7);	//master contrast current control
	OLED_send_data(0x0F);

	OLED_send_command(0xB1);	//SET PHASE LENGTH
	OLED_send_data(0xE2);

	OLED_send_command(0xD1);
	OLED_send_data(0x82);
	OLED_send_data(0x20);

	OLED_send_command(0xBB);	//SET PRE-CHANGE VOLTAGE
	OLED_send_data(0x1F);	//0.6*vcc

	OLED_send_command(0xB6);	//SET SECOND PRE-CHARGE PERIOD
	OLED_send_data(0x08);

	OLED_send_command(0xBE);	//SET VCOMH
	OLED_send_data(0x07);	//0.86*vcc

	OLED_send_command(0xA6);	//normal display

    OLED_send_command(0xAF);//Sleep Mode OFF
}


const uint32_t* FindBitmap(char c)
{
    for (uint8_t i = 0; i < DICT_SIZE; i++)
    {
        if (my_dict[i].key == c)
        {
            return my_dict[i].bitmap;
        }
    }
    // Если не нашли букву, возвращаем последний элемент (пробел)
    return my_dict[DICT_SIZE - 1].bitmap;
}


void OLED_full()
{
	unsigned int row, column;
	OLED_send_command(0x15);
	OLED_send_data(0x1C);
	OLED_send_data(0x5B);

	OLED_send_command(0x75);
	OLED_send_data(0x00);
	OLED_send_data(0x7F);
	OLED_send_command(0x5C);

	static const uint8_t white[256] = { [0 ... 255] = 0xFF };

	for(row = 0; row < 64; row++)
	{
		DMA_Print_Row(white, 256);
    }
}


void OLED_clear()
{
	unsigned int row,column;

	OLED_send_command(0x15);
	OLED_send_data(0x1C);
	OLED_send_data(0x5B);

	OLED_send_command(0x75);
	OLED_send_data(0x00);
	OLED_send_data(0x7F);
	OLED_send_command(0x5C);

	static const uint8_t black[256] = {0x00};

	for(row = 0; row < 64; row++)
	{
		DMA_Print_Row(black, 256);
    }
}


void OLED_SetCursor(uint8_t column, uint8_t row)
{
    OLED_send_command(0x15);//Адрес столбца
    OLED_send_data(0x1C + (column / 2));//Начальный (со смещением)
    OLED_send_data(0x5B);//Конечный

    OLED_send_command(0x75);//Адрес строки
    OLED_send_data(row);//Начальная
    OLED_send_data(0x3F);//Конечная

    OLED_send_command(0x5C);//Готовка к записи
}


void Picture_display(const unsigned char *ptr_pic)
{
	unsigned int row, column, x = 0;

	OLED_send_command(0x15);
	OLED_send_data(0x1C);
	OLED_send_data(0x5B);

	OLED_send_command(0x75);
	OLED_send_data(0x00);
	OLED_send_data(0x7F);

	OLED_send_command(0x5C);

    for(row = 0; row < 64; row++)
    {
    	DMA_Print_Row(ptr_pic, 256);

    	ptr_pic += 256;
    }
}


void DMA_Print_Row(uint8_t *data, uint16_t size)
{
	GPIOB->DATAOUTSET = 1 << 6;//B6\DC высокий уровень
	GPIOB->DATAOUTCLR = 1 << 5;//B5\CS низкий уровень


	DMA_CONFIGDATA.PRM_DATA.CH[17].SRC_DATA_END_PTR = (uint32_t)(&data[size - 1]);

	DMA_CONFIGDATA.PRM_DATA.CH[17].CHANNEL_CFG |= 0b001 << 29;//базовый режим
	DMA_CONFIGDATA.PRM_DATA.CH[17].CHANNEL_CFG |= (size - 1) << 5;//всего 256 передач

	DMA->ENSET |= 1 << 17;//Включение 17 канала
	SPI0->DMACR |= 1 << 1;//обслуживание буфера передатчика


	while (DMA_CONFIGDATA.PRM_DATA.CH[17].CHANNEL_CFG_bit.CYCLE_CTRL != 0);//ждем окончание загрузки в ДМА

	while (SPI0->SR & (1 << 4));//ждем окончание передачи SPI0


	GPIOB->DATAOUTSET = 1 << 5;//B5\CS высокий уровень
}



void DMA_Collect_Row(uint8_t x, uint8_t y, const char* str)
{
	uint8_t current_x = x;
	uint8_t current_y = y;
	uint8_t i = 0;

	uint8_t line_buffer[256];


	while (str[i] != '\0')
	{
		uint8_t chars_in_line = 0;
		uint8_t bytes_to_process = 0;

		while (str[i + bytes_to_process] != '\0')
		{
			uint8_t next_bytes = 1;

	    	if ((unsigned char)str[i + bytes_to_process] == 0xD0 || (unsigned char)str[i + bytes_to_process] == 0xD1)
	    	{
	    		next_bytes = 2;
	    	}

	    	if ((current_x + ((chars_in_line + 1) * 8)) > 128) break;

	    	chars_in_line++;
	    	bytes_to_process += next_bytes;
		}


		if (chars_in_line == 0)
		{
			current_x = 0;
			current_y += 32;

			if (current_y > 60)
			{
				current_y = 0;
				Delay_ms(10000);
				OLED_clear();
			}
			continue;
		}


		for (uint8_t row = 0; row < 28; row++)
		{
			OLED_SetCursor(current_x, current_y + row);

			uint8_t buf_idx = 0;
			uint8_t parse_idx = i;

			for (uint8_t c = 0; c < chars_in_line; c++)
			{
				if ((unsigned char)str[parse_idx] == 0xD0 || (unsigned char)str[parse_idx] == 0xD1)
				{
					parse_idx++;
				}

				const uint32_t* font_ptr = FindBitmap(str[parse_idx]);
				uint32_t row_bits = font_ptr[row];


				for (uint8_t b = 0; b < 7; b++)
				{
					line_buffer[buf_idx++] = (row_bits & (1 << (6 - b))) ? 0xFF : 0x00;
				}

				line_buffer[buf_idx++] = 0x00;


				parse_idx++;
			}

			DMA_Print_Row(line_buffer, chars_in_line * 8);
		}

		i += bytes_to_process;
		current_x += chars_in_line * 8;
	}
}



void Draw_Cursor(uint8_t x, uint8_t y, uint8_t state)
{
	uint8_t cursor_buf[8];

	for (uint8_t i = 0; i < 8; i++) cursor_buf[i] = state ? 0xFF : 0x0;

	for (uint8_t i = 0; i < 2; i++)
	{
		OLED_SetCursor(x, y + i);
		DMA_Print_Row(cursor_buf, sizeof(cursor_buf));
	}
}



void Inversion_Square(uint8_t state)
{
	if (state)
	{
		switch (current_window)
		{
		case WINDOW_1:
			DMA_Collect_Row(cursor_x, cursor_y - 29, (char[]){window_1[getIndex(cursor_x, cursor_y)], '\0'});
			break;
		}
	}
	else
	{
		uint8_t line_buf[8] = {0x00};

		for (uint8_t i = 0; i < 28; i++)
		{
			OLED_SetCursor(cursor_x, cursor_y - 29 + i);
			DMA_Print_Row(line_buf, 8);
		}
	}
}



uint8_t getIndex(uint8_t x, uint8_t y)
{
	return ((y / 32) * 16) + x / 8;
}



void FLASH_Window_init(char* str, uint8_t size, char* str1, uint8_t size1)
{
	uint32_t data_FLASH;


	data_FLASH = *(uint32_t*)(MEM_FLASH_BASE + MEM_FLASH_PAGE_SIZE * (MEM_FLASH_PAGE_TOTAL - 1));
	str[0] = data_FLASH & 0xFF;
	str[16] = data_FLASH >> 8 & 0xFF;

	for (uint8_t i = 0; i < size1; i++)
	{
		if (str[0] == str1[i]) current_window1_1 = i;

		if (str[16] == str1[i]) current_window1_2 = i;
	}


	data_FLASH = *(uint32_t*)(MEM_FLASH_BASE + MEM_FLASH_PAGE_SIZE * (MEM_FLASH_PAGE_TOTAL - 2));

	str[7] = (data_FLASH & 0b1111) + '0';
	str[8] = (data_FLASH >> 4 & 0b1111) + '0';
	str[14] = (data_FLASH >> 8 & 0b1111) + '0';
	str[15] = (data_FLASH >> 12 & 0b1111) + '0';
	str[23] = (data_FLASH >> 16 & 0b1111) + '0';
	str[24] = (data_FLASH >> 20 & 0b1111) + '0';
	str[30] = (data_FLASH >> 24 & 0b1111) + '0';
	str[31] = (data_FLASH >> 28 & 0b1111) + '0';
}





