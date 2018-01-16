using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace libSMP
{
    public class ChunkQueu<T> : Queue<T>
    {
        public IEnumerable<T> DequeueChunk(int chunkSize)
        {
            for (int i = 0; i < chunkSize && Count > 0; i++)
            {
                yield return Dequeue();
            }
        }

        public void EnqueuChunk(IEnumerable<T> chunk)
        {
            foreach(T data in chunk)
            {
                Enqueue(data);
            }
        }
    }
}
