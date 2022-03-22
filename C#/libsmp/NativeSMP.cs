using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;

namespace libsmp
{
    public class NativeSMP : SMP, IDisposable
    {
        private delegate sbyte FrameReady(IntPtr data, UInt32 length);
        [DllImport("libsmp")]
        private static extern IntPtr SMP_BuildObject(uint bufferlength, FrameReady frameReadyCallback, FrameReady rogueFrameCallback);
        [DllImport("libsmp")]
        private static extern void SMP_DestroyObject(IntPtr instance);
        [DllImport("libsmp")]
        private static extern uint SMP_SendRetIndex(byte[] buffer, ushort length, byte[] messagebuffer, ushort messagebufferlength, out ushort messageStartIndex);
        [DllImport("libsmp")]
        private static extern uint SMP_CalculateMinimumSendBufferSize(ushort length);
        [DllImport("libsmp")]
        private static extern sbyte SMP_RecieveInBytes(byte[] data, uint length, IntPtr instance);

        private IntPtr instance = IntPtr.Zero;
        private readonly FrameReady receiveFrameCallback;

        public override uint ReceiveErrors { get; protected set; } = 0;
        public override uint ReceivedMessages { get; protected set; } = 0;

        public NativeSMP(int bufferlength = 1000)
        {
            receiveFrameCallback = ReceiveFrame;
            instance = SMP_BuildObject((uint)bufferlength, receiveFrameCallback, null);
        }

        private sbyte ReceiveFrame(IntPtr data, uint length)
        {
            byte[] payload = new byte[length];
            Marshal.Copy(data, payload, 0, (int)length);
            receivedmessages.Enqueue(payload);
            return 0;
        }

        ~NativeSMP()
        {
            Dispose();
        }

        public override void Dispose()
        {
            base.Dispose();
            if(instance != IntPtr.Zero)
            {
                SMP_DestroyObject(instance);
                instance = IntPtr.Zero;
            }
        }

        public byte[] GenerateMessage(byte[] payload, int maxMessageLength)
        {
            byte[] messageBuffer = new byte[maxMessageLength];
            uint messagelength = SMP_SendRetIndex(payload, (ushort)payload.Length, messageBuffer, (ushort)messageBuffer.Length, out ushort startIndex);
            IEnumerable<byte> d = messageBuffer.Skip(startIndex).Take((int)messagelength);
            return d as byte[] ?? d.ToArray();
        }

        public override byte[] GenerateMessage(byte[] payload)
        {
            return GenerateMessage(payload, (int)SMP_CalculateMinimumSendBufferSize((ushort)(payload.Length)));
        }

        public override sbyte ProcessBytes(byte[] data)
        {
            return SMP_RecieveInBytes(data, (uint)data.Length, instance);
        }
    }
}
