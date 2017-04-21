using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using libSMP;

namespace smpTest
{
    class Program
    {
        static SMP send;
        static SMP receive;
        static byte[] message = { 10, 20, 50, 40, 50, 10, 20, 50, 40, 50, 10, 20, 50, 40, 50, 10, 20, 50, 40, 50, 10, 20, 50, 40, 50};
        static void Main(string[] args)
        {
            send = new SMP(true);
            send.frameReceived += Send_frameReceived;
            send.send += Send_send;

            receive = new SMP(true);
            receive.frameReceived += Receive_frameReceived;
            receive.send += Receive_send;

            send.SendData(message, (uint)message.Length);
        }

        private static ushort Receive_send(byte[] buffer, int length)
        {
            throw new NotImplementedException();
        }

        private static int Receive_frameReceived(List<byte> fifo)
        {
            byte[] receivedMessage = fifo.ToArray();
            if (receivedMessage.SequenceEqual(message))
            {
                Console.WriteLine("Test successfull");
            }
            else
                Console.WriteLine("Test failed");
            return 0;
        }

        private static ushort Send_send(byte[] buffer, int length)
        {
            buffer[1] = 255;
            return (ushort)receive.RecieveInBytes(buffer, (uint)length);
        }

        private static int Send_frameReceived(List<byte> fifo)
        {
            throw new NotImplementedException();
        }
    }
}
