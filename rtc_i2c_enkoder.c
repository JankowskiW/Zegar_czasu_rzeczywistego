#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif
#include <cr_section_macros.h>
#include "oled.h"
#include <math.h>
#define M_2PI 6.28318530
#define ENCODER_PORT 1
#define ENCODER_A 31
#define ENCODER_B 16
#define ENCODER_C 28
#define SPEED_100KHZ 100000
#define SPEED_400KHZ 400000
#define MENU_POS 8
#define MONTHS 12
uint8_t SlaveAddr = 0x51;
char sbuffer[30];
volatile uint8_t Mode = MENU_POS, M1 = MENU_POS;
uint8_t Set = 0;
const char Days[7][10] = { { " Sunday" },
		{ " Monday" },
		{ " Tuesday" },
		{ "Wednesday" },
		{ "Thursday" },
		{ " Friday" },
		{ "Saturday" } };

const char Msg[2][10] = {
		{ " Cancel "},
		{ "   OK   "}
};


typedef struct {
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t dayOfWeek;
	uint8_t dayOfMonth;
	uint8_t month;
	uint8_t year;
} RTC_T;

RTC_T RTC;


typedef struct {
	uint8_t cx0;
	uint8_t cy0;
	uint8_t cx1;
	uint8_t cy1;
	uint8_t *rtcs;
	uint8_t rtcmin;
	uint8_t rtcmax;
} CLOCK_SET_T;

const CLOCK_SET_T CLOCK_SET[MENU_POS] =
{ { 0, 7, 12, 15, &RTC.hour, 0, 23 },
		{ 18, 7, 30, 15, &RTC.minute, 0, 59 },
		{ 36, 7, 48, 15, &RTC.second, 0, 59 },
		{ 0, 23, 12, 31, &RTC.dayOfMonth, 1, 31 },
		{ 18, 23, 30, 31, &RTC.month, 1, 12 },
		{ 36, 23, 48, 31, &RTC.year, 0, 99 },
		{ 0, 39, 55, 47, &RTC.dayOfWeek, 0, 6 },
		{ 0, 55, 48 , 63, &Set, 0, 1}
};

const uint8_t DPM[MONTHS] = {31,28,31,30,31,30,31,31,30,31,30,31};

/*--------------------------------------------------------------------------------*/
uint8_t bcdToDec(uint8_t value) {
	return ((value >> 4) * 10 + (value & 0x0F));
}
/*--------------------------------------------------------------------------------*/
uint8_t decToBcd(uint8_t value) {
	return (((value / 10) << 4) + (value % 10));
}
/*--------------------------------------------------------------------------------*/
void RTC_Set(RTC_T *rtc) {
	uint8_t buffer[8] = { 0x02 };
	buffer[1] = decToBcd(rtc->second);
	buffer[2] = decToBcd(rtc->minute);
	buffer[3] = decToBcd(rtc->hour);
	buffer[4] = decToBcd(rtc->dayOfMonth);
	buffer[5] = decToBcd(rtc->dayOfWeek);
	buffer[6] = decToBcd(rtc->month);
	buffer[7] = decToBcd(rtc->year);
	Chip_I2C_MasterSend(I2C0, SlaveAddr, &buffer[0], 8);
}
/*--------------------------------------------------------------------------------*/
void RTC_Get(RTC_T *rtc) {
	uint8_t buffer[8] = { 0x02 };
	Chip_I2C_MasterSend(I2C0, SlaveAddr, &buffer[0], 1);
	Chip_I2C_MasterRead(I2C0, SlaveAddr, &buffer[1], 7);
	rtc->second = bcdToDec(buffer[1] & 0b01111111);
	rtc->minute = bcdToDec(buffer[2] & 0b01111111);
	rtc->hour = bcdToDec(buffer[3] & 0b00111111);
	rtc->dayOfMonth = bcdToDec(buffer[4] & 0b00111111);
	rtc->dayOfWeek = bcdToDec(buffer[5] & 0b00000111);
	rtc->month = bcdToDec(buffer[6] & 0b00011111);
	rtc->year = bcdToDec(buffer[7] & 0b01111111);
}
/*--------------------------------------------------------------------------------*/
void Analog_Clock(uint8_t x, uint8_t y, uint8_t radius, RTC_T *rtc) {
	uint8_t radius_s = radius;
	uint8_t radius_m = radius * 0.95;
	uint8_t radius_h = radius * 0.5;
	uint8_t radius_p = radius - 4;
	float h, m, s, p;
	s = (M_2PI * rtc->second) / 60.0;
	m = (M_2PI * rtc->minute) / 60.0;
	h = (M_2PI * ((rtc->hour % 12) + (rtc->minute / 60.))) / 12.0;
	for (int i = 0; i < 60; i++) {
		p = (M_2PI * i) / 60.0;
		if (i % 5)
			OLED_Draw_Point(x + radius * sin(p), y - radius * cos(p), 1);
		else
			OLED_Draw_Line(x + radius_p * sin(p), y - radius_p * cos(p),
					x + radius * sin(p), y - radius * cos(p));
	}
	OLED_Draw_Line(x, y, x + radius_s * sin(s), y - radius_s * cos(s));
	OLED_Draw_Line(x, y, x + radius_m * sin(m), y - radius_m * cos(m));
	OLED_Draw_Line(x, y, x + radius_h * sin(h), y - radius_h * cos(h));
}
/*--------------------------------------------------------------------------------*/
void Digital_Clock(uint8_t x, uint8_t y, RTC_T *rtc, uint8_t mode) {
	sprintf(sbuffer, "%02d:%02d.%02d", rtc->hour, rtc->minute, rtc->second);
	OLED_Puts(x + 1, y + 1, sbuffer);
	sprintf(sbuffer, "%02d-%02d-%02d", rtc->dayOfMonth, rtc->month, rtc->year);
	OLED_Puts(x + 1, y + 3, sbuffer);
	OLED_Puts(x + 1, y + 5, (char*) Days[rtc->dayOfWeek]);
	if (mode == MENU_POS-1)
		OLED_Puts(x + 1, y + 7, (char*) Msg[Set]);
	if (mode < MENU_POS)
		OLED_Invert_Rect(x + CLOCK_SET[mode].cx0, y + CLOCK_SET[mode].cy0,
				x + CLOCK_SET[mode].cx1, y + CLOCK_SET[mode].cy1);
}
/*--------------------------------------------------------------------------------*/
void I2C_IRQHandler(void) {
	/* State machine for I2C */
	if (Chip_I2C_IsMasterActive(I2C0)) {
		Chip_I2C_MasterStateHandler(I2C0);
	} else {
		Chip_I2C_SlaveStateHandler(I2C0);
	}
}
/*--------------------------------------------------------------------------------*/
void PIN_INT0_IRQHandler(void) {
	Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH0);
	uint8_t max;
	if (Mode < MENU_POS)
	{
		if (Mode == 3)
		{
			if (((*CLOCK_SET[4].rtcs) == 2)&&(!((*CLOCK_SET[5].rtcs)%4)))
			{
				max = DPM[(*CLOCK_SET[4].rtcs)-1]+1;
			}
			else
			{
				max = DPM[(*CLOCK_SET[4].rtcs)-1];
			}
		}else
		{
			max = CLOCK_SET[Mode].rtcmax;
		}

		if (Chip_GPIO_ReadPortBit(LPC_GPIO_PORT, ENCODER_PORT, ENCODER_B)) {
			//{ { 0, 7, 12, 15, &RTC.hour, 0, 23 },

			if(Mode == 7)
			{
				*CLOCK_SET[Mode].rtcs = 1;
			} else if(*(CLOCK_SET[Mode].rtcs)<max)
			{
				(*CLOCK_SET[Mode].rtcs)++;
			}
			else
			{
				(*CLOCK_SET[Mode].rtcs) = (CLOCK_SET[Mode].rtcmin);
			}

		} else
		{
			if(Mode == 7)
			{
				(*CLOCK_SET[Mode].rtcs) = 0;;
			}
			else if(*(CLOCK_SET[Mode].rtcs)>(CLOCK_SET[Mode].rtcmin))
			{
				(*CLOCK_SET[Mode].rtcs)--;
			}
			else
			{
				(*CLOCK_SET[Mode].rtcs) = max;
			}
		}
	}

	if (((*CLOCK_SET[4].rtcs) == 2)&&(!((*CLOCK_SET[5].rtcs)%4)))
	{
		max = DPM[(*CLOCK_SET[4].rtcs)-1]+1;
	}
	else
	{
		max = DPM[(*CLOCK_SET[4].rtcs)-1];
	}
	if(*CLOCK_SET[3].rtcs>max) (*CLOCK_SET[3].rtcs) = max;
}
/*--------------------------------------------------------------------------------*/
void PIN_INT1_IRQHandler(void) {
	Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH1);
	Mode++;
	if (Mode > MENU_POS)
		Mode = 0;
}
/*--------------------------------------------------------------------------------*/

int main(void) {
#if defined (__USE_LPCOPEN)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
	Board_LED_Set(0, true);
#endif
#endif
	// TODO: insert code here
	/*--------------------- I2C Init ------------------------*/
	Chip_SYSCTL_PeriphReset(RESET_I2C0);
	Chip_I2C_Init(I2C0);
	Chip_I2C_SetClockRate(I2C0, SPEED_400KHZ);
	Chip_I2C_SetMasterEventHandler(I2C0, Chip_I2C_EventHandler);
	NVIC_EnableIRQ(I2C0_IRQn);
	/*--------------------- OLED Init -----------------------*/
	OLED_Init();
	/*-------------------- GPIO INT Init --------------------*/
	Chip_IOCON_PinMuxSet(LPC_IOCON, ENCODER_PORT, ENCODER_A,
			(IOCON_FUNC0 | IOCON_MODE_PULLUP | IOCON_HYS_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, ENCODER_PORT, ENCODER_C,
			(IOCON_FUNC0 | IOCON_MODE_PULLUP | IOCON_HYS_EN));
	/* Enable PININT clock */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PINT);
	/* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SYSCTL_SetPinInterrupt(0, ENCODER_PORT, ENCODER_A);
	/* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH0);
	Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH0);
	Chip_PININT_EnableIntLow(LPC_PININT, PININTCH0);
	/* Enable interrupt in the NVIC */
	NVIC_SetPriority(PIN_INT0_IRQn, 0);
	NVIC_ClearPendingIRQ(PIN_INT0_IRQn);
	NVIC_EnableIRQ(PIN_INT0_IRQn);
	/* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SYSCTL_SetPinInterrupt(1, ENCODER_PORT, ENCODER_C);
	/* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH1);
	Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH1);
	Chip_PININT_EnableIntLow(LPC_PININT, PININTCH1);
	/* Enable interrupt in the NVIC */
	NVIC_SetPriority(PIN_INT1_IRQn, 0);
	NVIC_ClearPendingIRQ(PIN_INT1_IRQn);
	NVIC_EnableIRQ(PIN_INT1_IRQn);
	/*-------------------- End of Init ----------------------*/
	while (1) {
		if (Mode == MENU_POS)
		{
			if ((M1 == (Mode - 1)) && *CLOCK_SET[MENU_POS-1].rtcs)
				RTC_Set(&RTC);
			RTC_Get(&RTC);
		}
		M1 = Mode;
		OLED_Clear_Screen(0);
		Digital_Clock(0, 0, &RTC, Mode);
		Analog_Clock(96, 32, 30, &RTC);
		OLED_Refresh_Gram();
	}
	return 0;
}
