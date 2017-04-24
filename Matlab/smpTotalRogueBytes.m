function [ number ] = smpTotalRogueBytes(instance)
%totalRogueBytes returns the number of rogueBytes received in total
%   This function returns the total amount of roguebytes received of the
%   smp decoder. This value could be reset by a call to resetTotalRogueBytes

number = instance.totalRogueBytes;

end

