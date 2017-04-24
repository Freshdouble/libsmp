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
            send = new SMP(false);
            send.frameReceived += Send_frameReceived;
            send.send += Send_send;

            receive = new SMP(false);
            receive.frameReceived += Receive_frameReceived;
            receive.send += Receive_send;

            send.SendData(message, (uint)message.Length);

            SMP transceive = new SMP(false);
            transceive.SendData(message, (uint)message.Length);
            byte[] rogueBytes = { 216, 209, 216 };
            SMP.Message msg = transceive.NextSendMessage;
            transceive.RecieveInBytes(rogueBytes, (uint)rogueBytes.Length);
            transceive.RecieveInBytes(msg.message, msg.length);
            ulong totalErrors = transceive.totalRogueBytes;
            Console.WriteLine("RogueBytes: " + totalErrors.ToString());
        }

        private static ushort Receive_send(byte[] buffer, int length)
        {
            throw new NotImplementedException();
        }

        private static int Receive_frameReceived(List<byte> fifo)
        {
            byte[] receivedMessage = fifo.ToArray();
            if(receive.totalRogueBytes > 0)
            {
                Console.WriteLine("RogueBytes: " + receive.totalRogueBytes.ToString());
            }
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
            byte[] rogueBytes = { 216, 209, 216};
            ushort ret;
            receive.RecieveInBytes(rogueBytes, (uint)rogueBytes.Length);
            ret =(ushort)receive.RecieveInBytes(buffer, (uint)length);
            return ret;
        }

        private static int Send_frameReceived(List<byte> fifo)
        {
            throw new NotImplementedException();
        }
    }
}
