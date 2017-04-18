function [] = smpReceiveBytes( data,instance )
% smpReceiveBytes Process and add new message to the receive buffer.
% data = Vector holding the bytes that should be parsed by the smp library.
%

instance.RecieveInBytes(data,length(data));

end

