function [ message, success ] = smpGetNextSendMessage(instance)
% smpGetNextSendMessage Gets the next message in the send buffer.
% message = Vector that holds the bytes that should be sent over the
% interface.
% success = indicating a success if true, or if false the operation
% could'nt complete. Therefore the data in the message vector is invalid.
%

if instance.getMessageToSendCount()
    message = instance.getMessageToSend();
    success = true;
else
    success = false;
    message = [];
end
end

