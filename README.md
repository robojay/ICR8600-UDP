# ICR8600-UDP
Icom R8600 UDP Streamer using the ExtIO driver

This app will stream IQ data from an Icom R8600 via UDP.

For example, this can be used with the UDP Source block of GNU Radio.

It is very raw, PRE-Alpha, a work in progress, use AS-IS, etc.

## Features
- 24-bit IQ support by converting to 32-bit
- 16-bit IQ as-is
- 64-bit Complex Float conversion for both 24-bit and 16-bit
- Optional inclusion of a 64-bit sequence number for missed packet detection
- 1024 bytes of data sent per packet, alternating I and Q pairs

## Usage
- Turn on the R8600
- The R8600 DLLs must be in the same folder as this app
- Enter the tuning frequency in Hertz
- Enter the IP address for the stream (127.0.0.1 if local)
- Enter the UDP port for the stream
- Check the Send Sequence box if you want the 64-bit sequence number
- Check the Float box if you want to stream a 64-bit complex float
- Click the Show GUI button to configure the radio
- Click the Start button when ready to stream

## Performance
Keep in mind the volume of data coming over the USB interface from the radio and now streaming through the network layer of your PC.  A low performance PC may pose a challenge.

The R8600's USB interface should be directly connected to a port on your PC, not through an external hub.  Depending on your PC, there may be an internal hub and you may need to consider which devices (internal and external) are sharing that hub.

On the network side, using a hard wired gigabit ethernet connection is recommended, or keep the streaming internal to the PC.

Use the included Python 3 test program iq_rx.py to check your setup.  It will listen for UDP packets (with sequence numbers) on port 50005.  A Python 3 installation is required.

## Notes
- A pre-built executable is in the releases
- Icom provides two DLLs: ExtIO_ICR8600.dll and HidCtrl.dll
- Since the R8600 DLLs are 32-bit Windows, this app is 32-bit Windows
- Yes, read the lines above, you need the R8600 DLLs from Icom
- Yes, this is a Windows only application
- Compiled using Microsoft Visual Studio 2019

## Support / Issues / Pull Requests / New Features
- I have limited time to devote to this project.
- Contact me in advance before submitting a pull request.
- Open an Issue for bugs, I will do my best to address them.
- This is meant as a simple utility. Feature requests in line with that model will be considered. 
- I cannot help you debug your network issues...

## Reference Links
[HDSDR Homepage](http://www.hdsdr.de/): For information about ExtIO and links to specific [hardware](http://www.hdsdr.de/hardware.html) support.

[LC_ExtIO_Types.h](http://www.hdsdr.de/download/LC_ExtIO_Types.h): C Header file for ExtIO drivers

[Icom R8600 USB I/Q Package for HDSDR](https://www.icomjapan.com/support/firmware_driver/1972/): Includes the ExtIO driver for the R8600

[GNURadio](https://www.gnuradio.org/)

