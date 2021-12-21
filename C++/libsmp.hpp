#include "libsmp.h"
#include <array>
#include <cstdint>

#pragma once

template <size_t maxpacketSize>
class SMP
{
public:
    static const size_t BUFFER_SIZE = MINIMUM_SMP_BUFFERLENGTH(maxpacketSize);
    static_assert(maxpacketSize <= (numeric_limits<uint16_t>::max() >> 1), "SMP supports only 15bit message size");

    /**
     * @brief Construct a new SMP object.
     *
     * @param framereadyfunction - Function pointer to the callback function that is called when a new message is received.
     */
    SMP(const SMP_Frame_Ready framereadyfunction)
    {
        smp_settings_t settings;
        settings.buffer.buffer = buffer.data();
        settings.buffer.maxlength = buffer.size();
        settings.rogueFrameCallback = 0;
        settings.frameReadyCallback = framereadyfunction;

        SMP_Init(&smp, &settings);
    }

    /**
     * @brief Send data using the sendfunc.
     *
     * @tparam iterator - iterator to the start of the cointer with the data to send.
     * @tparam sendfunc - function used to send the data. This function must accept a std::array with arbitrary size.
     * @param start
     * @param end
     * @return true if the smp packet was created successfully and the send function accepted all bytes.
     */
    template <typename iterator, auto sendfunc>
    bool SendData(const iterator &start, const iterator &end)
    {
        std::array<uint8_t, maxpacketSize> buf;
        size_t inputlength = std::distance(start, end);
        if (inputlength > maxpacketSize)
        {
            return false; // return error if we can not send the full packet
        }
        std::copy(start, end, buf.begin());
        return SendData<maxpacketSize, sendfunc>(buf);
    }

    /**
     * @brief Send data using the sendfunc.
     *
     * @tparam length - length of the std::array used as buffer.
     * @tparam sendfunc - function used to send the data. This function must accept a std::array with arbitrary size.
     * @param buffer
     * @param bytesToSend
     * @return true if the smp packet was created successfully and the send function accepted all bytes.
     */
    template <const size_t length, auto sendfunc>
    bool SendData(const std::array<uint8_t, length> &buffer, const size_t bytesToSend)
    {
        static_assert(maxpacketSize <= length, "We can not send packages that are larger than the max allowed message size");
        std::array<uint8_t, BUFFER_SIZE> buf;
        uint8_t *messageStart = 0;
        size_t packetLength = 0;
        if ((packetLength = SMP_Send(buffer.data(), min<size_t>(bytesToSend, buffer.size()), buf.data(), buf.size(), &messageStart)) > length)
        {
            size_t sentbytes = sendfunc(buf);
            return sentbytes == packetLength;
        }
        else
        {
            return false;
        }
    }

private:
    /**
     * @brief Internal data storage for received data.
     *
     */
    std::array<uint8_t, BUFFER_SIZE> buffer;

    /**
     * @brief The smp data structure that holds the configuration as well as the status of the decoder.
     *
     */
    smp_struct_t smp;
};