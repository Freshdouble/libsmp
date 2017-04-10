clear all;
%% Prepare Data to send over the interface

instance = smpGetInstance(true); %Use library with Reed Solomon Codes change to false to use the library without reed solomon encoding


message = randi(255,20,1);
smpSendBytes(message,instance); %Prepare message for sending
if smpSendMessagesCount(instance) == 0 %Check how many messages are in the send buffer
    error('smp should have one message in the buffer at this point');
end
[sendMessage, success] = smpGetNextSendMessage(instance); %Get one message to send over the interface
if ~success
    %if success is false the data in the sendMessage vector is not
    %valid
    error('smp send Error');
end
%Send Data over the interface at this point

%% Receive Data from the interface

smpReceiveBytes(sendMessage,instance); %Send the received bytes to the smp library

if smpMessagesToReceive(instance) == 0 %Check if the smp library has received a valid message
    error('Data should be correct and therefore the receive counter should be greater than zero');
end

[receivedMessage, success] = smpGetNextReceiveMessage(instance); %Get received message from the buffer
if ~success %if success is zero there is no Data in the buffer, you should use smpMessagesToReceive before to check that
    %if success is false the data in the receivedMessage vector is not
    %valid
    error('smp receive error');
end

if receivedMessage == message
    success = true
else
    success = false
end
