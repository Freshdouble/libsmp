classdef lora < smp
    %UNTITLED Summary of this class goes here
    %   Detailed explanation goes here
    
    properties (SetAccess = private, GetAccess = private)
        subsmp
    end
    
    properties (SetAccess = private, GetAccess = public)
        directPassthrough
        
        LORA_MESSAGE_DATA = 1
        LORA_MESSAGE_HEARTBEAT = 2
        LORA_MESSAGE_SEND_STOP = 3
        LORA_MESSAGE_SEND_START = 4
        LORA_MESSAGE_SET_MODE = 5
    end
    
    methods (Access = private)
        function parsePacket(obj, packet)
            identifier = packet(1);
            data = packet(2:length(packet));
            
            switch identifier
                case obj.LORA_MESSAGE_DATA
                    obj.subsmp.ReceiveInBytes(data);
                    for i = 1:obj.subsmp.getReceivedMessageCount()
                        if isempty(obj.receivedMessages)
                            obj.receivedMessages = obj.subsmp.getReceivedMessage();
                        else
                            obj.receivedMessages = [obj.receivedMessages, obj.subsmp.getReceivedMessage()];
                        end
                    end
                case obj.LORA_MESSAGE_HEARTBEAT
                    disp('Lora Hearbeat received');
                otherwise
            end
        end
    end
    
    methods
        function obj = lora(directPassthrough)
            obj.subsmp = smp();
            obj.directPassthrough = directPassthrough;
        end
        
        function number = SendData(obj,data)
            if ~obj.directPassthrough
                obj.subsmp.SendData(data);
                if obj.subsmp.getMessageToSendCount == 1
                    data = obj.subsmp.getMessageToSend();
                    data = [obj.LORA_MESSAGE_DATA; data];
                else
                    number = 0;
                    return;
                end
            end
            number = SendData@smp(obj,data);
        end
        
        function stat = receiveByte(obj,data)
            stat = receiveByte@smp(obj,data);
            if ~obj.directPassthrough
                messagecounter = obj.getReceivedMessageCount;
                if messagecounter > 0
                    messages = obj.getReceivedMessage();
                    if messagecounter > 1
                        for i = 2:messagecounter
                            messages = [messages; obj.getReceivedMessage()];
                        end
                    end
                    for i = 1:messagecounter
                        obj.parsePacket(messages(:,i));
                    end
                end
            end
        end
    end
    
end

