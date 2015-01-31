#include <avr/io.h>

#define PIN_CLK    (1 << PINB0)
#define PIN_DAT    (1 << PINB1)

uint8_t readByte()
{
	uint8_t i;
	uint8_t pins;
	uint8_t prevPins = 0;
	uint8_t byte = 0;

	for(i = 0; i < 8; ++i)
	{
		// wait for clk low
		prevPins = pins;
		while((pins = PINB) & PIN_CLK)
		{
			// if data changes while clock is high then bail out
			// as it's a stop or a start
			if((pins & PIN_DAT) != (prevPins & PIN_DAT))
				return 0;
			prevPins = pins;
		}
	
		// wait for clk high
		while(!((pins = PINB) & PIN_CLK))
			;

		byte = (byte << 1) | ((pins & PIN_DAT) > 0);
	}

	// wait for clk low
	prevPins = pins;
	while((pins = PINB) & PIN_CLK)
	{
		if((pins & PIN_DAT) != (prevPins & PIN_DAT))
			return 0;
		prevPins = pins;
	}
	// wait for clk high
	while(!((pins = PINB) & PIN_CLK))
		;

	return byte;
}

// force the specified bit (1-8) to be zero
uint8_t overrideByte(uint8_t bit)
{
	uint8_t pins;
	uint8_t prevPins = 0;

	while(--bit)
	{
		// wait for clk low
		prevPins = pins;
		while((pins = PINB) & PIN_CLK)
		{
			// if dat changes then bail out, it's a start or stop
			if((pins & PIN_DAT) != (prevPins & PIN_DAT))
				return 0;
		}

		// wait for clk high
		while(!((pins = PINB) & PIN_CLK))
			;
	}

	// wait for clk low
	while((pins = PINB) & PIN_CLK)
		;

	// force dat low
	DDRB = (1 << DDB2) | (1 << DDB1);
	PORTB = 1 << PORTB2;

	// wait for clk high
	while(!((pins = PINB) & PIN_CLK))
		;

	// wait for clk low
	while(PINB & PIN_CLK)
		;

	// release dat (back to high-z)
	DDRB = (1 << DDB2);
	PORTB = 0;

	return 1;
}

void main()
{
	uint8_t currentTransmitter = 0;
	uint8_t pins = 0;
	uint8_t prevPins = 0;
	uint8_t addr = 0;
	uint8_t lastRegAddr = 0;

	CCP = 0xD8;
	CLKPSR = 0;
	PORTB = 0;
	DDRB = 1 << DDB2;

	for(;;)
	{
		PORTB = 0;
		DDRB = (1 << DDB2);

		prevPins = pins;
		pins = PINB;

		// if it's a START condition
		if(!(pins & PIN_DAT) && (prevPins & PIN_DAT) && (pins & PIN_CLK))
		{
RESTART:
			// read device address
			if((addr = readByte()) == 0)
				continue;

			// if it's a write to the EP9132
			if(addr == 0x70)
			{
				// read the register address
				if((lastRegAddr = readByte()) == 0)
					continue;

				if(lastRegAddr == 7)
				{
					if((currentTransmitter = readByte()) == 0)
						goto RESTART;
					currentTransmitter &= 1;
				}
			}
			// if it's a read from the EP9132
			else if(addr == 0x71)
			{
				if(lastRegAddr == 0x07)
				{
					// force the 4th bit to zero (bit 4, RX_ENC_ON)
					overrideByte(4);
				}
				else if(lastRegAddr == 0x40)
				{
					// force the 8th bit to zero (bit 0, RX_M0_RDY)
					overrideByte(8);
				}

				lastRegAddr = 0;
			}
		}
	}
}

