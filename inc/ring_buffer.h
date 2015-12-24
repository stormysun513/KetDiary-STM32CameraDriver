#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#define RX_BUFF_SIZE            128 
#define BYTES_PER_INTERRUPT     1

typedef struct {
	char buffer[RX_BUFF_SIZE];
	int length;
	int start;
	int end;
} RingBuffer;

void RingBuffer_init(RingBuffer *buffer);

int RingBuffer_read(RingBuffer *buffer, char *target, int amount);
int RingBuffer_write(RingBuffer *buffer, char *data, int amount);
bool RingBuffer_readbyte(RingBuffer *buffer, char *target);
bool RingBuffer_writebyte(RingBuffer *buffer, char byte);
/*
int RingBuffer_empty(RingBuffer *buffer);
int RingBuffer_full(RingBuffer *buffer);
int RingBuffer_available_data(RingBuffer *buffer);
int RingBuffer_available_space(RingBuffer *buffer);
*/
#define RingBuffer_available_data(B) (uint8_t)(((B)->end + (B)->length - (B)->start) % (B)->length)
#define RingBuffer_available_space(B) (uint8_t)((B)->length - 1 - RingBuffer_available_data((B)))
#define RingBuffer_full(B) (RingBuffer_available_data((B)) - ((B)->length - 1) == 0)
#define RingBuffer_empty(B) (RingBuffer_available_data((B)) == 0)
#define RingBuffer_puts(B, D) RingBuffer_write((B), bdata((D)), blength((D)))
#define RingBuffer_get_all(B) RingBuffer_gets((B), RingBuffer_available_data((B)))
#define RingBuffer_starts_at(B) ((B)->buffer + (B)->start)
#define RingBuffer_ends_at(B) ((B)->buffer + (B)->end)
#define RingBuffer_commit_read(B, A) ((B)->start = ((B)->start + (A)) % (B)->length)
#define RingBuffer_commit_write(B, A) ((B)->end = ((B)->end + (A)) % (B)->length)


#endif
