using System;

namespace libsmp
{
    public interface ISMP : IDisposable
    {
        int StoredMessages {get;}
        byte[] GetMessage();
        byte[] GenerateMessage(byte[] payload);
        sbyte ProcessBytes(Span<byte> data);
    }
}