function [ number ] = smpSendMessagesCount()
% smpSendMessagesCount  Returns the number of messages in the send buffer.
%

if libisloaded('libsmp_x64')
    number = calllib('libsmp_x64','libsmp_getMessagesToSend');
else
    error('The library is not loaded!');
end


end

