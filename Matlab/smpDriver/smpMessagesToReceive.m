function [ number ] = smpMessagesToReceive(instance)
% smpMessagesToReceive Returns the number of message in the receive buffer.
%

number = instance.getReceivedMessageCount();

end

