#include <stdio.h>
#include <stdint.h>

#include "ring_buffer.h"

void RingBuffer_init(RingBuffer *buffer)
{
	buffer->length  = RX_BUFF_SIZE;
	buffer->start = 0;
	buffer->end = 0;
}

int RingBuffer_write(RingBuffer *buffer, char *data, int amount)
{
	if(RingBuffer_available_data(buffer) == 0) {
		buffer->start = buffer->end = 0;
	}
		    
	if(amount > RingBuffer_available_space(buffer))
		return -1;
			    
	unsigned int i;
	int temp = buffer->end;
	for(i = 0; i < amount; i++){
		buffer->buffer[temp] = data[i];
		temp++;
		temp %= buffer->length;
	}
	RingBuffer_commit_write(buffer, amount);
	return amount;
}

bool RingBuffer_writebyte(RingBuffer *buffer, char byte){
  if(RingBuffer_available_data(buffer) == 0) {
    buffer->start = buffer->end = 0;
  }
		    
  if(1 > RingBuffer_available_space(buffer))
    return false;
			    
  buffer->buffer[buffer->end] = byte;
  RingBuffer_commit_write(buffer, 1);
  return true;
}

int RingBuffer_read(RingBuffer *buffer, char *target, int amount)
{
	if(amount > RingBuffer_available_data(buffer))
		return -1;

	unsigned int i;
	int temp = buffer->start;
	for(i = 0; i < amount; i++){
		target[i] = buffer->buffer[temp];
		temp++;
		temp %= buffer->length;
	}
					    
	RingBuffer_commit_read(buffer, amount);					    
	if(buffer->end == buffer->start) {
		buffer->start = buffer->end = 0;
	}
	return amount;
}

bool RingBuffer_readbyte(RingBuffer *buffer, char* target){
	if(RingBuffer_available_data(buffer) == 0)
		return false;

	*target = buffer->buffer[buffer->start];				    
	RingBuffer_commit_read(buffer, 1);

	if(buffer->end == buffer->start) {
		buffer->start = buffer->end = 0;
	}
	return true;
}