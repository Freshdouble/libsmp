function [ number ] = smpMessagesToReceive(instance)
% smpMessagesToReceive Returns the number of message in the receive buffer.
%

if libisloaded('libsmp_x64')
    number = calllib('libsmp_x64','libsmp_bytesMessagesToReceive',instance);
else
    error('The library is not loaded!');
end

end

