using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace libSMP
{
    public class SMPOutputBuffer : SMP
    {
        private Queue<Message> messagesReceived;
        private ChunkQueu<byte> rogueBytes;

        public override event EventHandler MessageReceived;

        public SMPOutputBuffer(bool useRS, ITransmitionInterface inter) : base(useRS, inter)
        {
            messagesReceived = new Queue<Message>();
            rogueBytes = new ChunkQueu<byte>();
        }

        public override int Length => messagesReceived.Count;

        public override Message Read()
        {
            return messagesReceived.Dequeue();
        }

        public IEnumerable<byte> getRogueData(int count)
        {
            return rogueBytes.DequeueChunk(count);
        }

        protected override void frameReceived(List<byte> data)
        {
            Message m = new Message(data);
            messagesReceived.Enqueue(m);
            MessageReceived(this, null);
        }

        protected override void rogueFrameReceived(List<byte> data)
        {
            rogueBytes.EnqueuChunk(data);
        }

        public int RogueDataCount => rogueBytes.Count;
        public int ReceivedMessageCount => messagesReceived.Count;
    }
}
