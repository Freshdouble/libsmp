classdef smp < handle
    %UNTITLED2 Summary of this class goes here
    %   Detailed explanation goes here
    
    properties (SetAccess = private, GetAccess = private)
      flags
      crc
      crcHighByte
      bytesToReceive
      
      fifo
      
      receivedLengthBytes
    end
    
    properties (SetAccess = protected, GetAccess = protected)
      receivedMessages
      MessagesToSend
    end
    
    properties (SetAccess = private, GetAccess = public)
        CRC_POLYNOM
        FRAMESTART
        MAX_PAYLOAD
        rogueByteCounter
    end
   
    methods (Access = private)
        function crc = crc16(obj,crc,c,mask)
            for i = 0:7
                if bitand(bitxor(crc,c),1) ~= 0
                    crc = bitxor(bitshift(crc,-1),mask);
                else
                    crc = bitshift(crc,-1);
                end
                c = bitshift(c,-1);
            end
        end
        
        function stat = strippFramestart(obj,data)
            if data == obj.FRAMESTART
                if obj.flags.receivedDelimeter
                    obj.flags.receivedDelimeter = false;
                    stat = obj.processByte(data);
                    return
                else
                    obj.flags.receivedDelimeter = true;
                end
            else
                if obj.flags.receivedDelimeter
                    obj.flags.status = 1;
                    obj.bytesToReceive = 0;
                    obj.flags.receivedDelimeter = false;
                    if obj.flags.receiving
                        [number,~] = size(obj.fifo);
                        obj.rogueByteCounter = obj.rogueByteCounter + number + 1 + obj.receivedLengthBytes;
                        obj.fifo = [];
                        obj.receivedLengthBytes = 0;
                    end
                    obj.flags.receiving = true;
                end
                stat = obj.processByte(data);
                return
            end
            stat = 0;
        end
        
        function stat = processByte(obj,data)
            switch obj.flags.status
                case 0
                    obj.rogueByteCounter = obj.rogueByteCounter + 1;
                case 1
                    if obj.bytesToReceive == 0
                        obj.bytesToReceive = data;
                        obj.receivedLengthBytes = 1;
                    else
                        obj.receivedLengthBytes = 2;
                        obj.bytesToReceive = bitor(obj.bytesToReceive,bitshift(data,8));
                        obj.flags.status = 2;
                        obj.fifo = [];
                        obj.crc = 0;
                    end
                case 2
                    if obj.bytesToReceive == 2
                        obj.flags.status = 3;
                        stat = obj.processByte(data);
                        return
                    else
                        obj.bytesToReceive = obj.bytesToReceive - 1;
                        obj.fifo = [obj.fifo;data];
                        obj.crc = obj.crc16(obj.crc,data,obj.CRC_POLYNOM);
                    end
                case 3
                    if obj.bytesToReceive == 0
                        obj.flags.status = 0;
                        obj.flags.receiving = false;
                        if obj.crc == (bitor(bitshift(obj.crcHighByte,8),data))
                            if isempty(obj.receivedMessages)
                                obj.receivedMessages = obj.fifo;
                            else
                                obj.receivedMessages = [obj.receivedMessages, obj.fifo];
                            end
                            stat = 1;
                        else
                            [number,~] = size(obj.fifo);
                            obj.rogueByteCounter = obj.rogueByteCounter + number + 5;
                            stat = -1;
                        end
                        obj.receivedLengthBytes = 0;
                        obj.fifo = [];
                        return
                    else
                        obj.crcHighByte = data;
                        obj.bytesToReceive = 0;
                    end
                otherwise
                    obj.flags.status = 0;
                    obj.bytesToReceive = 0;
                    obj.flags.receiving = false;
                    obj.receivedLengthBytes = 0;
                    stat = -3;
                    return
            end
            stat = 0;
         end
    end
    
    methods
        function obj = smp()
            obj.flags = struct('receiving',false,'receivedDelimeter',false,'noCRC', false,'status', 0);
            obj.crc = 0;
            obj.crcHighByte = 0;
            obj.bytesToReceive = 0;
            obj.fifo = [];
            obj.MessagesToSend = [];
            obj.receivedMessages = [];
            obj.rogueByteCounter = 0;
            obj.receivedLengthBytes = 0;
            
            obj.CRC_POLYNOM = hex2dec('A001');
            obj.FRAMESTART = hex2dec('FF');
            obj.MAX_PAYLOAD = 65000;
        end
        
        function number = SendData(obj,data)
            len = length(data);
            message1 = zeros(2*(len+2),1);
            message2 = zeros(2*(len+2) + 5,1);
            offset = 0;
            crc = 0;
            for i = 1:len
                if(data(i) == 255)
                    message1(i + offset) = 255;
                    offset = offset + 1;
                end
                
                message1(i + offset) = data(i);
                crc = obj.crc16(crc,data(i),obj.CRC_POLYNOM);
            end
            i = i + 1;
            
            message1(i + offset) = bitshift(crc,-8);
            if message1(i + offset) == obj.FRAMESTART
                offset = offset + 1;
                message1(i + offset) = obj.FRAMESTART;
            end
            i = i + 1;
            message1(i + offset) = bitand(crc,255);
            if message1(i + offset) == obj.FRAMESTART
                offset = offset + 1;
                message1(i + offset) = obj.FRAMESTART;
            end
            
            packageSize = len + 2;
            completeFramesize = packageSize + offset;
            offset = 0;
            message2(1) = obj.FRAMESTART;
            message2(2) = bitand(packageSize,255);
            if message2(2) == obj.FRAMESTART
                message2(2) = obj.FRAMESTART;
                offset = 1;
            end
            message2(3+offset) = bitshift(packageSize,-8);
            if message2(3+offset) == obj.FRAMESTART
                message2(4+offset) = obj.FRAMESTART;
                offset = 2;
            end
            
            for i = (4+offset):(completeFramesize+3+offset)
                message2(i) = message1(i - (3+offset));
            end
            
            number = completeFramesize + 3 + offset;
            message2 = message2(1:number,:);
            if isempty(obj.MessagesToSend)
                obj.MessagesToSend = message2;
            else
                obj.MessagesToSend = [obj.MessagesToSend, message2];
            end
        end
        
        function stat = receiveByte(obj,data)
            stat = obj.strippFramestart(data);
        end
    
        function stat = ReceiveInBytes(obj,data)
            stat = [];
            for i = 1:length(data)
                status = obj.receiveByte(data(i));
                stat = and(stat,status);
            end
        end
        
        function number = getReceivedMessageCount(obj)
            [~,number] = size(obj.receivedMessages);
        end
        
        function message = getReceivedMessage(obj)
            if obj.getReceivedMessageCount() > 0
                message = obj.receivedMessages(:,1);
                obj.receivedMessages(:,1) = [];
            else
                message = [];
            end
        end
        
        function number = getMessageToSendCount(obj)
            [~,number] = size(obj.MessagesToSend);
        end
        
        function message = getMessageToSend(obj)
            if obj.getMessageToSendCount() > 0
                message = obj.MessagesToSend(:,1);
                obj.MessagesToSend(:,1) = [];
            else
                message = [];
            end
        end
        
        function resetRougeByteCounter(obj)
            obj.rogueByteCounter = 0;
        end
    end    
end

