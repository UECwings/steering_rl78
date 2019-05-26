#include "FIFO.h"

//リングバッファ

void FIFO_Initialize(FIFO *fifo, unsigned char *buf, int size)
{
	fifo->buf = buf;
	fifo->size = size;
	fifo->readPtr = 0;
	fifo->writePtr = 0;

	return;
}

int FIFO_Write(FIFO *fifo, unsigned char data)
{
	if ((fifo->writePtr + 1) % fifo->size == fifo->readPtr) {
		//空きがない
		return(1);
	}

	fifo->buf[fifo->writePtr] = data;
	fifo->writePtr = (fifo->writePtr + 1) % fifo->size;

	return(0);
}

int FIFO_Read(FIFO *fifo)
{
	unsigned char data;

	if (fifo->readPtr == fifo->writePtr) {
		//データが無い
		return(-1);
	}
	
	data = fifo->buf[fifo->readPtr];
	fifo->readPtr = (fifo->readPtr + 1) % fifo->size;
	return((int) data);
}

int FIFO_Length(FIFO *fifo)
{
	return((fifo->writePtr - fifo->readPtr + fifo->size) % fifo->size);
}