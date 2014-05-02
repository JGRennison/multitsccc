## multitsccc: Multi TS Continuity Counter Corrector

### multitsccc
**multitsccc** is a tool to correct MPEG2 TS continuity counter errors across multiple files.  
This removes any discontinuities in TS files and between the end of one TS file and the start of the next.  
The TS files can then be concatenated without causing a continuity counter discontinuity.  
This is useful for HLS, as some clients require that the continuity counters are continuous across file boundaries.  
TS files are **modified in place**.  
Usage: `multitsccc TS_FILE_1 [TS_FILE_2] [TS_FILE_3] ...`

### multitsccc_hls
**multitsccc_hls** is a wrapper script to execute multitsccc with the contents of one or more HLS manifests.  
It is not mandatory to use this wrapper, it is for convenience.  
Usage: `multitsccc_hls HLS_MANIFEST_1 [HLS_MANIFEST_2] [HLS_MANIFEST_3] ...`

### URLs
This project is currently hosted at:  
https://github.com/JGRennison/multitsccc  

### Dependencies
* Perl (multitsccc_hls only)  

### Building
Should build out of the box with `make` and `make install`.  
Requires a relatively recent gcc (4.7.1 or later).  
Requires a POSIX/*nix OS. Does not support Windows.  

### License
GPLv2  
