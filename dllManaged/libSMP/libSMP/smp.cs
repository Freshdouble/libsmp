using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace libSMP
{
    class SMP
    {
        private struct message_t
        {
            public byte[] message;
            public UInt32 size;
        }

        [DllImport("libsmp_x64.dll")]
        private static extern void libsmp_addReceivedBytes(ref byte[] bytes, UInt32 length, IntPtr obj);

        [DllImport("libsmp_x64.dll")]
        private static extern UInt32 libsmp_bytesMessagesToReceive(IntPtr obj);

        [DllImport("libsmp_x64.dll")]
        private static extern UInt16 libsmp_getNextReceivedMessageLength(IntPtr obj);

        [DllImport("libsmp_x64.dll")]
        private static extern byte libsmp_getReceivedMessage(ref message_t msg, IntPtr obj);

        [DllImport("libsmp_x64.dll")]
        private static extern UInt32 libsmp_getMessagesToSend(IntPtr obj);

        [DllImport("libsmp_x64.dll")]
        private static extern UInt16 libsmp_getNextMessageLength(IntPtr obj);

        [DllImport("libsmp_x64.dll")]
        private static extern byte libsmp_getMessage(ref message_t msg, IntPtr obj);

        [DllImport("libsmp_x64.dll")]
        private static extern UInt32 libsmp_sendBytes(ref byte[] bytes, UInt32 length, IntPtr obj);

        [DllImport("libsmp_x64.dll")]
        private static extern IntPtr libsmp_createNewObject(bool rs);

        [DllImport("libsmp_x64.dll")]
        private static extern void libsmp_deleteObject(IntPtr obj);

        public delegate void messageReceived(object sender);
        public delegate void messageSend(byte[] buffer, int size);

        public event messageReceived received;
        public event messageSend send;

        IntPtr obj;

        public SMP(bool useRS)
        {
            obj = libsmp_createNewObject(useRS);
            received = null;
            send = null;
        }

        ~SMP()
        {
            libsmp_deleteObject(obj);
            received = null;
            send = null;
        }

        public void addReceivedBytes(byte[] bytes, UInt32 length)
        {
            libsmp_addReceivedBytes(ref bytes, length, obj);
            if ((received != null) && (getReceivedMessageCount() > 0))
            {
                received(this);
            }
        }

        public UInt32 getReceivedMessageCount()
        {
            return libsmp_bytesMessagesToReceive(obj);
        }

        public byte[] getReceivedMessage()
        {
            message_t msg = new message_t();
            msg.message = new byte[libsmp_getNextReceivedMessageLength(obj)];
            if (libsmp_getReceivedMessage(ref msg, obj) != 0)
            {
                return msg.message;
            }
            else
            {
                return new byte[0];
            }
        }

        public UInt32 getMessagesToSend()
        {
            return libsmp_getMessagesToSend(obj);
        }

        public byte[] getMessageToSend()
        {
            message_t msg = new message_t();
            msg.message = new byte[libsmp_getNextMessageLength(obj)];
            if (libsmp_getMessage(ref msg, obj) != 0)
            {
                return msg.message;
            }
            else
            {
                return new byte[0];
            }
        }

        public void sendBytes(byte[] bytes, UInt32 length)
        {
            libsmp_sendBytes(ref bytes, length, obj);
            if (send != null && getMessagesToSend() > 0)
            {
                byte[] message = getMessageToSend();
                send(message, message.Length);
            }
        }
    }
}
