function [] = smpReceiveBytes( data )
% smpReceiveBytes Process and add new message to the receive buffer.
% data = Vector holding the bytes that should be parsed by the smp library.
%

if libisloaded('libsmp_x64')
    v = libpointer('uint8Ptr',data);
    calllib('libsmp_x64','libsmp_addReceivedBytes',v,length(data));
else
    error('The library is not loaded!');
end

end

