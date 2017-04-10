function [ instance ] = smpGetInstance( useRS )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here

if libisloaded('libsmp_x64')
    unloadlibrary('libsmp_x64')
end

[~,~]=loadlibrary('libsmp_x64',@mxsmp);

    instance = calllib('libsmp_x64','libsmp_createNewObject',useRS);

end

