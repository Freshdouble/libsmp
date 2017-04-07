function [] = smpInit( useRS )
% smpInit Loads and initialize the smp library.
% useRS = Use reed solomon encoding true/false.
%

if libisloaded('libsmp_x64')
    unloadlibrary('libsmp_x64')
end

[~,~]=loadlibrary('libsmp_x64',@mxsmp);

if useRS
    calllib('libsmp_x64','libsmp_useRS',1);
end

end

