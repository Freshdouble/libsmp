/*****************************************************************************************************
 File: libsmp
 Autor: Peter Kremsner
 Date: 12.9.2014

 Basic data transmission protocoll to ensure data integrity

 ******************************************************************************************************/
#include "libsmp.h"
#include <stdio.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_PAYLOAD 65534

/***********************************************************************
 * Private function definiton to calculate the crc checksum
 */
unsigned int crc16(unsigned int crc, unsigned int c, unsigned int mask)
{
	unsigned char i;
	unsigned int _crc = crc;
	for (i = 0; i < 8; i++)
	{
		if ((_crc ^ c) & 1)
		{
			_crc = (_crc >> 1) ^ mask;
		}
		else
			_crc >>= 1;
		c >>= 1;
	}
	return (_crc);
}

/************************************************************************
 * Initialize the smp-buffers
 * Set:
 * 		st->buffer
 * 		st->send
 * 		st->frameReadyCallback
 * 		st->rogueFrameCallback
 * 		and if compiling with RS_Code support
 * 		st->ecc
 * before calling this function.
 * Return -1 if the datasize of the buffer is not the sizeof(uin8_t)
 * 0 otherwise

 ************************************************************************/
signed char SMP_Init(smp_struct_t* st)
{
#ifdef USE_RS_CODE
	if (st->ecc)
	{
		initialize_ecc(st->ecc);
	}
	st->rsPtr = 0;
#endif // USE_RS_CODE
	st->flags.recieving = 0;
	st->crc = 0;
	st->flags.status = 0;
	return 0;
}

/************************************************************************
 * @brief Estimate the number of bytes in the full smp packet.
 * 
 * Since this function is faster than the calculation of the smp packet itself
 * one could check if the sendfunction is able to send all data and if it's not
 * avoid packet creation, especialy when using reed solomon codes
 * 
 * @return estimated size of the smp packet
 ************************************************************************/

unsigned int SMP_estimatePacketLength(byte* buffer, unsigned short length, smp_struct_t *st)
{
	unsigned short overheadCounter = 0;
	unsigned int ret = 0;
	for(unsigned short i = 0; i < length; i++)
	{
		if(buffer[i] == FRAMESTART)
			overheadCounter++;
	}
	ret = length + overheadCounter + 5;
	#ifdef USE_RS_CODE
	if(st->ecc)
	{
		unsigned char Blocknumber = ret/BlockData;
		ret = ret * Blocksize;
		if((double)ret/(double)BlockData > Blocknumber)
			ret += Blocksize;
	}
	#endif
	return ret;
}

/************************************************************************/
/* Sends data to the smp outputbuffer                                   */

/************************************************************************/
unsigned char SMP_Send(byte *buffer, unsigned short length, smp_struct_t *st)
{
	unsigned int i = 2;
	unsigned int offset = 0;
	unsigned short crc = 0;

	if (length > MAX_PAYLOAD)
		return 0;

	unsigned char message1[2 * (length + 2)];
	unsigned char message2[2 * (length + 2) + 5];

	for (i = 0; i < length; i++)
	{

		if (buffer[i] == FRAMESTART)
		{
			message1[i + offset] = FRAMESTART;
			offset++;
		}

		message1[i + offset] = buffer[i];
		crc = crc16(crc, buffer[i], CRC_POLYNOM);
	}

	message1[i + offset] = crc >> 8; //CRC high byte
	if (message1[i + offset] == FRAMESTART)
	{
		offset++;
		message1[i + offset] = FRAMESTART;
	}
	message1[i + offset + 1] = crc & 0xFF; //CRC low byte
	if (message1[i + offset + 1] == FRAMESTART)
	{
		offset++;
		message1[i + offset + 1] = FRAMESTART;
	}
	
	unsigned int packageSize = length + 2;
	unsigned int completeFramesize = packageSize + offset;
	offset = 0;
	message2[0] = FRAMESTART;
	message2[1] = packageSize & 0xFF;
	if (message2[1] == FRAMESTART)
	{
		message2[2] = FRAMESTART;
		offset = 1;
	}
	message2[2 + offset] = packageSize >> 8;
	if (message2[2 + offset] == FRAMESTART)
	{
		message2[3 + offset] = FRAMESTART;
		offset = 2;
	}

	for (i = 3 + offset; i < completeFramesize + 3 + offset; i++)
	{
		message2[i] = message1[i - (3 + offset)];
	}

	unsigned int messageSize = completeFramesize + 3 + offset;

#ifdef USE_RS_CODE
	if (st->ecc)
	{
		messageSize--;
		unsigned int BlockNumber = messageSize / BlockData;
		if (BlockNumber * BlockData < messageSize)
		{
			BlockNumber++;
		}
		unsigned int newMessageSize = BlockNumber * Blocksize;
		unsigned char newBuffer[newMessageSize + 1];
		unsigned char CurrentBlock[BlockData];
		unsigned int ptr = 1;
		unsigned int newPtr = 1;
		for (i = 0; i < BlockNumber; i++)
		{
			if ((unsigned int) (messageSize - ptr + 1) >= BlockData)
			{
				MEMCPYFUNCTION(&message2[ptr], CurrentBlock, BlockData);
				ptr += BlockData;
			}
			else
			{
				const unsigned char zeroes[BlockData] =
				{ 0 };
				MEMCPYFUNCTION(CurrentBlock, zeroes, BlockData);
				MEMCPYFUNCTION(&message2[ptr], CurrentBlock, messageSize - ptr + 1);
				ptr = messageSize;
			}
			encode_data(CurrentBlock, BlockData, &newBuffer[newPtr], st->ecc);
			newPtr += Blocksize;
			if (ptr == messageSize)
				break;
		}
		newBuffer[0] = FRAMESTART;
		if (st->send)
			return (st->send(newBuffer, newMessageSize + 1) == (newMessageSize + 1)) ? newMessageSize + 1 : 0;
	}
	else
	{
		if (st->send)
			return (st->send(message2, messageSize) == (messageSize)) ? messageSize : 0;
	}
#else
	if(st->send)
		return (st->send(message2, messageSize) == (messageSize)) ? messageSize : 0;
#endif // USE_RS_CODE
	return 0;
}

/************************************************************************

 Recieve multiple bytes from the Interface and writes it to the
 recievebuffer.

 ************************************************************************/

inline signed char SMP_RecieveInBytes(byte* data, unsigned int length, smp_struct_t* st)
{
	unsigned int i;
	signed char ret = 0;
	for (i = 0; i < length; i++)
	{
		ret = SMP_RecieveInByte(data[i], st);
		if (ret < 0)
			break;
	}
	return ret;
}

signed char private_SMP_RecieveInByte(byte data, smp_struct_t* st)
{
	switch (st->flags.status)
	//State machine
	{
		case 0: //Idle State Waiting for Framestart
			break;
		case 1:
			if (!st->bytesToRecieve)
			{
				st->bytesToRecieve = data;
			}
			else
			{
				st->bytesToRecieve |= data << 8;
				st->flags.status = 2;
				fifo_clear(st->buffer);
				st->crc = 0;
			}
			break;
		case 2:
			if (st->bytesToRecieve == 2) //Only CRC to recieve
			{
				st->flags.status = 3;
			}
			else
			{
				st->bytesToRecieve--;
				if (!fifo_write_bytes(&data, st->buffer,1))
				{
					//Bufferoverflow
					st->flags.status = 0;
					st->flags.recieving = 0;
#ifdef USE_RS_CODE
					st->rsPtr = 0;
#endif
					return -1;
				}
				st->crc = crc16(st->crc, data, CRC_POLYNOM);
				break;
			}
		case 3:
			if (!st->bytesToRecieve)
			{
				st->flags.status = 0;
				st->flags.recieving = 0;
#ifdef USE_RS_CODE
				st->rsPtr = 0;
#endif
				if (st->crc == ((unsigned short) (st->crcHighByte << 8) | data)) //Read the crc and compare
				{
					//Data ready
					if (st->frameReadyCallback)
						return st->frameReadyCallback(st->buffer);
					else
						return -2;
				}
				else //crc doesnt match.
				{
					if (st->rogueFrameCallback)
						return st->rogueFrameCallback(st->buffer);
				}
			}
			else
			{
				st->crcHighByte = data; //Save first byte of crc
				st->bytesToRecieve = 0;
			}
			break;

		default: //Invalid State
			st->flags.status = 0;
			st->bytesToRecieve = 0;
			st->flags.recieving = 0;
#ifdef USE_RS_CODE
			st->rsPtr = 0;
#endif // USE_RS_CODE
			break;
	}
	return 0;
}

/************************************************************************/
/* Recieve a byte from the interface                                    */

/************************************************************************/
#ifdef USE_RS_CODE
signed char private_SMP_RecieveInByte_2(byte data, smp_struct_t* st)
#else
signed char SMP_RecieveInByte(byte data, smp_struct_t* st)
#endif // USE_RS_CODE
{
	if (data == FRAMESTART)
	{
		if (st->flags.recievedDelimeter)
		{
			st->flags.recievedDelimeter = 0;
			return private_SMP_RecieveInByte(data, st);
		}
		else
			st->flags.recievedDelimeter = 1;
	}
	else
	{
		if (st->flags.recievedDelimeter)
		{
			st->flags.status = 1;
			st->bytesToRecieve = 0;
			st->flags.recievedDelimeter = 0;
			if (st->flags.recieving)
			{
				if (st->rogueFrameCallback)
					st->rogueFrameCallback(st->buffer);
			}
			st->flags.recieving = 1;
		}
		return private_SMP_RecieveInByte(data, st);
	}
	return 0;
}

#ifdef USE_RS_CODE
unsigned int seekFramestart(byte* data, unsigned int length)
{
	unsigned int i;
	for (i = 0; i < length; i++)
	{
		if (data[i] == FRAMESTART)
			return i + 1;
	}
	return 0;
}

signed char SMP_RecieveInByte(byte data, smp_struct_t* st)
{
	unsigned int i;
	signed char ret = 0;
	if ((st->flags.recieving || st->flags.recievedDelimeter) && st->ecc)
	{
		st->rsBuffer[st->rsPtr] = data;
		st->rsPtr++;
		if (st->rsPtr >= Blocksize)
		{
			st->rsPtr = 0;
			decode_data(st->rsBuffer, Blocksize, st->ecc);
			if (check_syndrome(st->ecc) != 0)
			{
				if (!correct_errors_erasures(st->rsBuffer, Blocksize, 0, 0, st->ecc))
				{
					st->flags.status = 0;
					st->bytesToRecieve = 0;
					st->flags.recieving = 0;
					unsigned int Framepos = seekFramestart(st->rsBuffer,
					Blocksize);
					if (Framepos > 0)
					{
						Framepos--;
						ret = private_SMP_RecieveInByte_2(st->rsBuffer[Framepos], st);
						unsigned int cpyLength = Blocksize - Framepos - 1;
						byte newBlock[cpyLength];
						MEMCPYFUNCTION(&st->rsBuffer[Framepos + 1], newBlock, cpyLength);
						MEMCPYFUNCTION(newBlock, st->rsBuffer, cpyLength);
						st->rsPtr = cpyLength;
						return ret;
					}
					return -1;
				}
			}
			for (i = 0; i < BlockData; i++)
			{
				ret = private_SMP_RecieveInByte_2(st->rsBuffer[i], st);
				if (ret < 0)
					break;
			}
		}
	}
	else
	{
		return private_SMP_RecieveInByte_2(data, st);
	}
	return ret;
}
#endif // USE_RS_CODE

/**********************************************************************
 * Get the amounts of byte to recieve from the Interface for a full frame
 * This value is only valid if status.recieving is true
 * However if status.recieving is false this value should hold 0 but this
 * isn't confirmed yet
 **********************************************************************/
byte SMP_GetBytesToRecieve(smp_struct_t* st)
{
	if (st->flags.recieving)
		return st->bytesToRecieve;
	else
		return 0;
}

/**
 * Returns true if the smp stack for the smp_struct object is recieving
 * @param st
 * @return
 */
byte SMP_IsRecieving(smp_struct_t* st)
{
	return st->flags.recieving;
}
