function [ message, success ] = smpGetNextSendMessage()
% smpGetNextSendMessage Gets the next message in the send buffer.
% message = Vector that holds the bytes that should be sent over the
% interface.
% success = indicating a success if true, or if false the operation
% could'nt complete. Therefore the data in the message vector is invalid.
%

if libisloaded('libsmp_x64')
    messageSize = calllib('libsmp_x64','libsmp_getNextMessageLength');
    if messageSize == 0
        message = zeros(1,1);
        success = false;
    else
        msg = zeros(messageSize,1);
        data.message = libpointer('uint8Ptr',msg);
        data.size = uint32(0);
        cstr = libstruct('message_t',data);
        [~,cstr] = calllib('libsmp_x64','libsmp_getMessage',cstr);
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

