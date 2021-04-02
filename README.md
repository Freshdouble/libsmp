# SMP - Simple Message Protcol

The goal of this library is to provide a simple to use but stable message protcol. 
The intended use of the smp is the communication between microcontrollers or between a microcontroller and pc, over simple "raw" bus interfaces
like I2C, SPI, UART,...

The library contains C code for the use on micrcontrollers which can be compiled without the use of the malloc function. The C code
is gcc compatible and can also be compiled to a shared library.

The C# library consists of a implementation of the SMP in pure C# and a interface class to use a shared library version of the SMP in C code.

# The protocol

A SMP Frame consists of:
| Name      | Bytes | Standard value |
|-----------|-------|----------------|
| Framstart | 1     | 0xFF           |
| Length    | 2     |                |
| Payload   | n     |                |
| CRC       | 2     |                |

## Bytestuffing

If the framestart occures inside the packet (Length, Payload, CRC) it is masked with a second framestart byte. This results in a worst case overhead of 100% if
the packet consists only of framestarts. The typical overhead on random messages is about 3%.
When receiving the added Framestarts are stripped before the databytes are passed to the decoding statemachine.

## CRC

The crc is a 16 Byte CRC over the whole payload without the bytestuffing bytes in the payloadsection. The used generator polynom is 0xA001.
Tests showed that in 500 000 packets with an average payload of 70 byte and a byterror propability of 2% 2 to 5 messages get a false positive on the crc check and are treated as valid packets.
If the transmitted data do not tolerate spurios faulty packets the integrity of the payload should be ensured using additional methods.
