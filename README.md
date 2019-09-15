# cli-audiosytem
A basic audio playback, and recording program.
In order to read from a file, 
or write to a file,
the user must use redirects.
The only file type excepted is .raw.

## Running
./cli-audiosystem [dir] [audio info]

[dir]
- -p Playback
- -c Capture

[audio info]
- --r=samplerate(Hz)
- --b=bitformat(Check Available Bit Formats bellow)
- --l=channels

## Available Bit Formats 
s16le = Signed 16 bit Little Endian 
s16be = Signed 16 bit Big Endian 
u16le = Unsigned 16 bit Little Endian 
u16be = Unsigned 16 bit Big Endian 

## Example
### Playback
./cli-audiosystem -p --r=41000 --b=s16le --l=2 < audiofile.raw
### Capture
./cli-audiosystem -c --r=41000 --b=s16le --l=2 > audiofile.raw
