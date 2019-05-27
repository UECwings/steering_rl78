
typedef struct {
    unsigned char *buf;
    int readPtr, writePtr, size;
} FIFO;

void FIFO_Initialize(FIFO *fifo, unsigned char *buf, int size);
int FIFO_Write(FIFO *fifo, unsigned char data);
int FIFO_Read(FIFO *fifo);
int FIFO_Length(FIFO *fifo);