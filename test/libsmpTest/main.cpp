#include "libsmp.hpp"
#include <cstdlib>
#include <functional>

std::array<uint32_t, 10> test;
uint8_t received[sizeof(decltype(test)::value_type) * test.size()];

SMP<100> smp;

size_t Receive(const void* buffer, size_t length);

size_t Transmit(const void* buffer, size_t length)
{
    auto callback = Receive;
    smp.Receive(callback, buffer, length);
    return length;
}

size_t Receive(const void* buffer, size_t length)
{
    memcpy(received, buffer, length);
    return length;
}

int main()
{
    for(auto& b : test)
    {
        b = rand();
    }
    auto callback = Transmit;
    smp.Transmit(callback, test.begin(), test.end());
    decltype(test) testrecv;
    memcpy(testrecv.data(), received, sizeof(received));
    return 0;
}
