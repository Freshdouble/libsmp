function [ message, success ] = smpGetNextReceiveMessage( instance )
% smpGetNextReceiveMessage Gets the next message in the recieve buffer.
% message = Vector that holds the bytes of the message.
% success = indicating a success if true, or if false the operation
% could'nt complete. Therefore the data in the message vector is invalid.
%

try
    messageInst = instance.NextReceivedMessage;
    message = uint8(messageInst.message);
    success = true;
catch
    success = false;
    message = 0;
end


end

