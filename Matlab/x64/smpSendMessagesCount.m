function [ number ] = smpSendMessagesCount(instance)
% smpSendMessagesCount  Returns the number of messages in the send buffer.
%

if libisloaded('libsmp_x64')
    number = calllib('libsmp_x64','libsmp_getMessagesToSend',instance);
else
    error('The library is not loaded!');
end


end

