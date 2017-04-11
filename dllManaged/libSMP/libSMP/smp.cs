using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace libSMP
{
    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Ansi)]
    struct message_t
    {

        /// unsigned char[65536]
        public byte[] message;

        /// unsigned int
        public uint size;
    }

    partial class NativeMethods
    {

        /// Return Type: void
        ///bytes: char*
        ///length: unsigned int
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_addReceivedBytes")]
        public static extern void libsmp_addReceivedBytes(byte[] bytes, uint length, System.IntPtr obj);


        /// Return Type: size_t->unsigned int
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_bytesMessagesToReceive")]
        [return: System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.SysUInt)]
        public static extern uint libsmp_bytesMessagesToReceive(System.IntPtr obj);


        /// Return Type: unsigned short
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_getNextReceivedMessageLength")]
        public static extern ushort libsmp_getNextReceivedMessageLength(System.IntPtr obj);


        /// Return Type: unsigned char
        ///msg: message_t*
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_getReceivedMessage")]
        public static extern byte libsmp_getReceivedMessage(ref message_t msg, System.IntPtr obj);


        /// Return Type: size_t->unsigned int
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_getMessagesToSend")]
        [return: System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.SysUInt)]
        public static extern uint libsmp_getMessagesToSend(System.IntPtr obj);


        /// Return Type: unsigned short
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_getNextMessageLength")]
        public static extern ushort libsmp_getNextMessageLength(System.IntPtr obj);


        /// Return Type: unsigned char
        ///msg: message_t*
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_getMessage")]
        public static extern byte libsmp_getMessage(ref message_t msg, System.IntPtr obj);


        /// Return Type: unsigned int
        ///bytes: char*
        ///length: unsigned int
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_sendBytes")]
        public static extern uint libsmp_sendBytes(byte[] bytes, uint length, System.IntPtr obj);


        /// Return Type: void*
        ///useRS: boolean
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_createNewObject")]
        public static extern IntPtr libsmp_createNewObject([System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.I1)] bool useRS);


        /// Return Type: void
        ///obj: void*
        [System.Runtime.InteropServices.DllImportAttribute("libsmp_x64.dll", EntryPoint = "libsmp_deleteObject")]
        public static extern void libsmp_deleteObject(System.IntPtr obj);

    }


    public class SMP
    {
        public delegate void messageReceived(object sender);
        public delegate void messageSend(byte[] buffer, int offset, int size);

        public event messageReceived received;
        public event messageSend send;

        private IntPtr obj;

        public SMP(bool useRS)
        {
            NativeMethods.libsmp_createNewObject(useRS);
            received = null;
            send = null;
        }

        ~SMP()
        {
            NativeMethods.libsmp_deleteObject(obj);
            received = null;
            send = null;
        }

        public void addReceivedBytes(byte[] bytes, UInt32 length)
        {
            NativeMethods.libsmp_addReceivedBytes(bytes, length, obj);
            if ((received != null) && (getReceivedMessageCount() > 0))
            {
                received(this);
            }
        }

        public UInt32 getReceivedMessageCount()
        {
            return NativeMethods.libsmp_bytesMessagesToReceive(obj);
        }

        public byte[] getReceivedMessage()
        {
            message_t msg = new message_t();
            msg.message = new byte[NativeMethods.libsmp_getNextReceivedMessageLength(obj)];
            if (NativeMethods.libsmp_getReceivedMessage(ref msg, obj) != 0)
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
            return NativeMethods.libsmp_getMessagesToSend(obj);
        }

        public byte[] getMessageToSend()
        {
            message_t msg = new message_t();
            msg.message = new byte[NativeMethods.libsmp_getNextMessageLength(obj)];
            if (NativeMethods.libsmp_getMessage(ref msg, obj) != 0)
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
            NativeMethods.libsmp_sendBytes(bytes, length, obj);
            if (send != null && getMessagesToSend() > 0)
            {
                byte[] message = getMessageToSend();
                send(message, 0, message.Length);
            }
        }
    }
}
