function [ number ] = smpSendBytes( data,instance )
% smpSendBytes Process and add new message to the send buffer.
% data = Vector holding the bytes to be send in the message.
% number = Number of Bytes in the Message or zero if there was an error.
%

if libisloaded('libsmp_x64')
    v = libpointer('uint8Ptr',data);
    number = calllib('libsmp_x64','libsmp_sendBytes',v,length(data),instance);
else
    error('The library is not loaded!');
end

end

