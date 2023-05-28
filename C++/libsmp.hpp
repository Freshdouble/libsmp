#include "libsmp.h"
#include <array>
#include <cstdint>
#include <iterator>

#pragma once

/**
 * @brief Class for the smp receiver and sender functions. This is an abstraction from the standard C functions.
 *
 * This class generates an internal buffer that is about 2*maxpacketSize bytes big.
 * The transmit functions use a local version of that buffer on the stack memory, so maxpacketSize has a main impact on the memory consumption.
 */
template <size_t maxmessageLength>
class SMP
{
public:
    static constexpr auto CalcReceiveArrayLength(size_t msglen)
    {
        return msglen;
    }

    static constexpr auto CalcTransmitArrayLength(size_t msglen)
    {
        return SMP_SEND_BUFFER_LENGTH(msglen);
    }

    static constexpr size_t InternalBufferLength = CalcTransmitArrayLength(maxmessageLength);
    static constexpr size_t MaximumSupportedMessageLength = 0xFEFF;
    static constexpr auto ReceiveArrayLength = CalcReceiveArrayLength(maxmessageLength);
    static constexpr auto TransmitArrayLength = InternalBufferLength;

    static constexpr size_t GetMinimumMessageLengthField(size_t messageLength)
    {
        return messageLength + 2;
    }

    static_assert(GetMinimumMessageLengthField(maxmessageLength) <= MaximumSupportedMessageLength);

    /**
     * @brief Construct a new SMP object.
     */
    SMP()
    {
        SMP_Init(&smp);
    }

    template <typename functorType>
    size_t Transmit(functorType& callback, const void *buffer, size_t length)
    {
        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(buffer);
        const uint8_t *end = ptr + length;
        return Transmit<functorType, const uint8_t *>(callback, ptr, end);
    }

    template <typename functorType, typename Iterator>
    size_t Transmit(functorType& callback, const Iterator &start, const Iterator &end)
    {
        std::array<uint8_t, TransmitArrayLength> buffer;
        buffer[0] = FRAMESTART;
        size_t length = 0;
        uint16_t crc = 0;
        for (auto it = start; it < end; it++)
        {
            auto b = *it;
            length += sizeof(b);
            crc = CalcCRC(*it, crc);
        }
        size_t crclength = CalculateByteStuffedLength(crc);
        if (length > InternalBufferLength || (length + crclength) > MaximumSupportedMessageLength)
            return 0;

        uint16_t lengthField = length + crclength;

        buffer[1] = lengthField & 0xFF;
        buffer[2] = (lengthField >> 8) & 0xFF;
        size_t offset = 3;
        for (auto it = start; it < end; it++)
        {
            auto data = *it;
            offset += AddDataToBuffer(data, buffer, offset);
        }
        offset += AddDataToBuffer(crc, buffer, offset, true);
        if (callback(buffer.data(), offset) == offset)
        {
            return length;
        }
        else
        {
            return 0;
        }
    }

    template <typename functorType>
    size_t Receive(functorType& callback, const void *buffer, size_t length)
    {
        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(buffer);
        const uint8_t *end = ptr + length;
        return Receive<functorType, const uint8_t *>(callback, ptr, end);
    }

    template <typename functorType, typename Iterator>
    size_t Receive(functorType& callback, const Iterator &start, const Iterator &end)
    {
        std::array<uint8_t, ReceiveArrayLength> buffer;
        static size_t offset = 0;
        size_t bytecount = 0;
        for (auto it = start; it < end; it++)
        {
            uint8_t d;
            auto ret = SMP_RecieveInByte(*it, &d, &smp);
            switch (ret)
            {
            case PACKET_START_FOUND:
            	offset = 0;
            	break;
            case RECEIVED_BYTE:
                buffer[offset] = d;
                offset++;
                break;
            case PACKET_READY:
            	callback(buffer.data(), offset);
            	offset = 0;
                break;
            default:
                break;
            }
            bytecount++;
        }
        return bytecount;
    }

private:
    template <typename Iterator>
    bool AdvanceIteratorSave(Iterator &it, size_t offset, Iterator end)
    {
        std::advance(it, offset);
        if (it > end)
        {
            return false;
        }
        return true;
    }

    template <typename T>
    inline size_t CalculateByteStuffedLength(const T &data)
    {
        size_t ret = sizeof(T);
        for (size_t i = 0; i < sizeof(T); i++)
        {
            if (((data >> (i * 8)) & 0xFF) == FRAMESTART)
            {
                ret++;
            }
        }
        return ret;
    }

    template <typename T, size_t size>
    inline size_t AddDataToBuffer(const T &data, std::array<uint8_t, size> &buffer, const size_t offset, const bool reverseByteOrder = false)
    {
        size_t ret = 0;
        if (reverseByteOrder)
        {
            for (size_t i = (sizeof(T) - 1); i > 0; i--)
            {
                uint8_t d = (data >> (i * 8)) & 0xFF;
                if ((offset + ret) >= buffer.size())
                {
                    return 0;
                }
                buffer[offset + ret] = d;
                ret++;
                if (d == FRAMESTART)
                {
                    if ((offset + ret) >= buffer.size())
                    {
                        return 0;
                    }
                    buffer[offset + ret] = FRAMESTART;
                    ret++;
                }
            }
            if ((offset + ret) >= buffer.size())
            {
                return 0;
            }
            buffer[offset + ret] = data & 0xFF;
            ret++;
            if ((data & 0xFF) == FRAMESTART)
            {
                if ((offset + ret) >= buffer.size())
                {
                    return 0;
                }
                buffer[offset + ret] = FRAMESTART;
                ret++;
            }
        }
        else
        {
            for (size_t i = 0; i < sizeof(T); i++)
            {
                uint8_t d = (data >> (i * 8)) & 0xFF;
                if ((offset + ret) >= buffer.size())
                {
                    return 0;
                }
                buffer[offset + ret] = d;
                ret++;
                if (d == FRAMESTART)
                {
                    if ((offset + ret) >= buffer.size())
                    {
                        return 0;
                    }
                    buffer[offset + ret] = FRAMESTART;
                    ret++;
                }
            }
        }
        return ret;
    }

    template <typename T>
    inline uint16_t CalcCRC(const T &data, uint16_t crc)
    {
        for (size_t i = 0; i < sizeof(T); i++)
        {
            uint8_t d = (data >> (i * 8)) & 0xFF;
            crc = SMP_crc16(crc, d, CRC_POLYNOM);
        }
        return crc;
    }

    /**
     * @brief The smp data structure that holds the configuration as well as the status of the decoder.
     *
     */
    smp_struct_t smp;
};
