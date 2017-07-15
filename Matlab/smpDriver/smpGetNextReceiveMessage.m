function [ message, success ] = smpGetNextReceiveMessage( instance )
% smpGetNextReceiveMessage Gets the next message in the recieve buffer.
% message = Vector that holds the bytes of the message.
% success = indicating a success if true, or if false the operation
% could'nt complete. Therefore the data in the message vector is invalid.
%

if instance.getReceivedMessageCount()
    message = instance.getReceivedMessage();
    success = true;
else
    success = false;
    message = [];
end


end

