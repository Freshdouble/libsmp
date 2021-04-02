using System;
using libsmp;
using System.Linq;

namespace Test
{
    class Program
    {
        static void Main(string[] args)
        {
            var watch = System.Diagnostics.Stopwatch.StartNew();
            using (var smp = new ManagedSMP())
            {
                Random rand = new Random();
                for (int i = 0; i < 500000; i++)
                {
                    byte[] payload = new byte[rand.Next(20, 100)];
                    rand.NextBytes(payload);
                    var msg = smp.GenerateMessage(payload);
                    if (msg.Length == 0)
                    {
                        Console.WriteLine("Message was not created");
                        return;
                    }
                    else
                    {
                        smp.ProcessBytes(msg);
                        if(smp.StoredMessages > 1)
                        {
                            Console.WriteLine("Framing Error");
                            return;
                        }
                        if (smp.StoredMessages == 0)
                        {
                            Console.WriteLine("Message was not received: {0}", payload.Length);
                            foreach (var b in msg)
                            {
                                Console.Write("{0:x} ", b);
                            }
                            return;
                        }
                        else
                        {
                            var recv = smp.GetMessage();
                            if (!Enumerable.SequenceEqual(payload, recv))
                            {
                                Console.WriteLine("Received Message was not correct");
                                Console.WriteLine("Payload: ");
                                foreach (var b in payload)
                                {
                                    Console.Write("{0:x} ", b);
                                }
                                Console.WriteLine();
                                Console.WriteLine("Transmitted: ");
                                foreach (var b in msg)
                                {
                                    Console.Write("{0:x} ", b);
                                }
                                Console.WriteLine();
                                Console.WriteLine("Received: ");
                                foreach(var b in recv)
                                {
                                    Console.Write("{0:x} ", b);
                                }
                                Console.WriteLine();
                                Console.WriteLine("Difference: ");
                                foreach(var b in payload.Except(recv))
                                {
                                    Console.Write("{0:x} ", b);
                                }
                                return;
                            }
                        }
                    }
                }
            }
            watch.Stop();
            Console.WriteLine("Elapsed Time: {0}ms", watch.ElapsedMilliseconds);
        }
    }
}
