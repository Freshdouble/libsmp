classdef smp
    %UNTITLED2 Summary of this class goes here
    %   Detailed explanation goes here
    
    properties (SetAccess = private, GetAccess = private)
      flags
      crc
      crcHighByte
      bytesToReceive
      
      fifo
    end
    
    properties (SetAccess = private, GetAccess = public)
        CRC_POLYNOM
        FRAMESTART
        MAX_PAYLOAD
    end
   
    methods (Access = private)
        function crc = crc16(obj,crc,c,mask)
            for i = 0:7
                if bitand(mod(crc,c),1) ~= 0
                    crc = mod(bitshift(crc,-1),mask);
                else
                    crc = bitshif(crc,-1);
                end
                c = bitshift(c,-1);
            end
        end
        
        function [stat,message] = strippFramestart(obj,data)
            if data == obj.FRAMESTART
                if obj.flags.receivedDelimeter
                    obj.flags.receivedDelimeter = false;
                    [stat,message] = obj.processByte(data);
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
                        obj.fifo = [];
                    end
                    obj.flags.receiving = true;
                end
                [stat,message] = obj.processByte(data);
                return
            end
            stat = 0;
        end
        
        function [stat,message] = processByte(obj,data)
            switch obj.flags.status
                case 0
                case 1
                    if obj.bytesToReceive == 0
                        obj.bytesToReceive = data;
                    else
                        obj.bytesToReceive = bitor(obj.bytesToReceive,bitshift(data,-8));
                        obj.flags.status = 2;
                        obj.fifo = [];
                        obj.crc = 0;
                    end
                case 2
                    if obj.bytesToReceive == 2
                        obj.flags.status = 3;
                        [stat,message] = obj.processByte(data);
                        return
                    else
                        obj.bytesToReceive = obj.bytesToReceive - 1;
                        obj.fifo = [obj.fifo,data];
                        obj.crc = obj.crc16(obj.crc,data,obj.CRC_POLYNOM);
                    end
                case 3
                    if obj.bytesToReceive == 0
                        obj.flags.status = 0;
                        obj.flags.receiving = false;
                        if obj.crc == (bitor(bitshift(obj.crcHighByte,8),data))
                            message = obj.fifo;
                            stat = 1;
                        else
                            message = [];
                            stat = -1;
                        end
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
                    stat = -3;
                    message = [];
                    return
            end
            message = [];
            stat = 0;
         end
    end
    
    methods
        function obj = smp()
            obj.flags = struct(receiving,false,receivedDelimeter,false,noCRC, false,status, 0);
            obj.crc = 0;
            obj.crcHighByte = 0;
            obj.bytesToReceive = 0;
            obj.fifo = [];
            
            obj.CRC_POLYNOM = hex2dec('A001');
            obj.FRAMESTART = hex2dec('FF');
            obj.MAX_PAYLOAD = 65000;
        end
        
        function [number,message] = SendData(obj,data)
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
            
            message1(i + offset) = bitshift(crc,-8);
            if message1(i + offset) == obj.FRAMESTART
                offset = offset + 1;
                message1(i + offset) = obj.FRAMESTART;
            end
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
            message = message2;
        end
        
        function [stat,message] = receiveByte(obj,data)
            [stat,message] = obj.strippFramestart(data);
        end
    
        function [stat,message] = ReceiveInBytes(obj,data)
            stat = [];
            message = [];
            for i = 1:length(data)
                [status,nachricht] = obj.strippFramestart(data);
                if status ~= 0
                    stat = [stat; status];
                    if status == 1
                        message = [message; nachricht];
                    end
                end
            end
        end
    end    
end

