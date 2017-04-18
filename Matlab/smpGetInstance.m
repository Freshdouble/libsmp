function [ instance ] = smpGetInstance( useRS )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here

NET.addAssembly(strcat(pwd,'\libSMP.dll'));
instance = libSMP.SMP(useRS);
end

