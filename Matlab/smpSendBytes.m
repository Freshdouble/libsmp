function [ number ] = smpSendBytes( data,instance )
% smpSendBytes Process and add new message to the send buffer.
% data = Vector holding the bytes to be send in the message.
% number = Number of Bytes in the Message or zero if there was an error.
%

number = instance.SendData(data,length(data));

end

