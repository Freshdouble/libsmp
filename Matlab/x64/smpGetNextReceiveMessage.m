function [ message, success ] = smpGetNextReceiveMessage( instance )
% smpGetNextReceiveMessage Gets the next message in the recieve buffer.
% message = Vector that holds the bytes of the message.
% success = indicating a success if true, or if false the operation
% could'nt complete. Therefore the data in the message vector is invalid.
%

if libisloaded('libsmp_x64')
    messageSize = calllib('libsmp_x64','libsmp_getNextReceivedMessageLength',instance);
    if messageSize == 0
        message = zeros(1,1);
        success = false;
    else
        msg = zeros(messageSize,1);
        data.message = libpointer('uint8Ptr',msg);
        data.size = uint32(0);
        cstr = libstruct('message_t',data);
        [~,cstr] = calllib('libsmp_x64','libsmp_getReceivedMessage',cstr,instance);
        if cstr.size == 0
            message = zeros(1,1);
            success = false;
        else
            message = cstr.message;
            success = true;
        end
    end
else
    error('The library is not loaded!');
end


end

