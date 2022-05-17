#include "libsmp.h"
#include <array>
#include <cstdint>
#include <tuple>

#pragma once

/**
 * @brief Class for the smp receiver and transmitter functions. This is an abstraction from the standard C functions.
 *
 * This class generates an internal buffer that is about 2*maxpacketSize bytes big.
 * The transmit functions use a local version of that buffer on the stack memory, so maxpacketSize has a main impact on the memory consumption.
 */
template <size_t maxpacketSize>
class SMP
{
public:
    static const size_t BUFFER_SIZE = MINIMUM_SMP_BUFFERLENGTH(maxpacketSize);
    static_assert(maxpacketSize <= (std::numeric_limits<uint16_t>::max() >> 1), "SMP supports only 15bit message size");

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
        auto inputlength = std::distance(start, end);
        if (inputlength > maxpacketSize || inputlength < 0)
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
        static_assert(length <= maxpacketSize, "We can not send packages that are larger than the max allowed message size");
        std::array<uint8_t, BUFFER_SIZE> buf;
        uint8_t *messageStart = 0;
        size_t packetLength = 0;
        assert(length >= bytesToSend); //Check if the buffer even has enough space for all bytes to send
        if ((packetLength = SMP_Send(buffer.data(), std::min<size_t>(bytesToSend, buffer.size()), buf.data(), buf.size(), &messageStart)) > bytesToSend) //Some plausibility check that the transmited data must be larger than the input data
        {
            size_t sentbytes = sendfunc(messageStart, packetLength);
            return sentbytes == packetLength;
        }
        else
        {
            return false;
        }
    }

    template<const size_t length>
    void ReceiveData(const std::array<uint8_t, length> &buffer)
    {
    	ReceiveData(buffer.data(), length);
    }

    void ReceiveData(const uint8_t* data, size_t length)
    {
    	SMP_RecieveInBytes(data, static_cast<uint32_t>(length), &smp);
    }

    constexpr const std::array<uint8_t, BUFFER_SIZE>& GetBuffer()
    {
    	return buffer;
    }

    constexpr auto GetData(const uint8_t* ptr)
    {
    	auto dist = std::distance(buffer.data(), ptr);
    	assert(dist >= 0);
    	return std::make_tuple(buffer.begin() + dist, buffer.end());
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