# MaxLight
## Alpha 1.0
This is a little tool to get the average screen color of a monitor of choice and send it to an rgb LED strip as fast as possible to tint a whole room in the displayed color. \
It is not another copy of an Ambilight system, yet, but can be used for it if you'd assign Desktop-Areas to different LEDs and update the USB-buffer and Arduino code. \
Instead, it's a room light to destress the eyes and raise the immersive power of entertainment setups and work spaces. \
This is a quick/test setup for which i just dropped one LED strip behind my desk:


<p align="center"><img src="https://user-images.githubusercontent.com/29096190/120733553-bbab7e80-c4e7-11eb-9283-43d97362c2dc.gif" width="320" height="160" /></p>



Keeping the CPU usage low was one of the goals, because gaming on my very own setup revealed: \
  my CPU is the performance bottleneck while the GPU stays cool and calm. 
A super low latency was important as well.

## Using the program aka finding your preferred config.-values: 
Make sure to install an USB-driver for the arduino. In my case it's the CH340-driver.\
Your computer recognizes the device? \
Go ahead and run the program:
```
Inserting higher saturation and brightness values in the setup will show you the current color accents of the screen. 
For watching a movie, I'd recommend lower values to get an ambient lighting and smoother transitions. 
Higher saturation values than zero can be used to ignore the 256 shades of grey (from black to white) and 
a higher brighness value ignores dark pixels and raises the overall brightness of the LEDs,
which then adapt only the color of the accents on the display. 
In combination with a low saturation value, a smooth and ambient effect can be created. 
The fading factor is something like the "aggressiveness". Higher fading factors will lead to a smoother transition between your captured colors. 
Just play around. 
The presets were optimized for "Cat Quest 2"(video games) and animes. 
```

## TODOs:
- [x] Check the fading function for flaws 
    - [x] Write an extra gamma_correction() function. 
    - [x] Apply the gamma correction only while sending, not in retrieve_pixel(). 
- [x] better USB-connection, e.g. detect disconnect and wait for re-connect
- [ ] Reinitialize enumerator and devices after device failure
- [ ] Implement modes 
    - [ ] static light
    - [ ] standard light effects (rainbow, smooth, ...)
    - [ ] overall darken/brighten factor
- [ ] Assign monitor areas to distinguished LEDs or LED-stripes (Ambilight(TM)-style)
    - [ ] monitor iterations can be split in different loops(/threads) 
    - [ ] re-write buffer-send
    - [ ] Arduino-script to distribute buffer-values to LEDs/stripes
- [ ] Gamma-correction value shift and matched gamma-values for "night mode"
- [x] (Think about image compression)
- [ ] Turn the console application into a nice UI after the bachelor's thesis.
    


## Appendix:
The project got compiled by Visual Studios' "Release Mode"  with winSDK and only works on Windows 8.1 and higher (x64). \
'lumos_maxima.cpp' is the Arduino code and got compiled to the arduino by the Arduino-IDE, using it's compiler (just press the "upload" button for CH340 after choosing the old boot loader) \
..nothing fancy.. 
Just ask me for a license and i will add one. 

More features will come soon.
 


## Known issues:
- It can only find a USB-port with a COM-port number below 10. Make sure to connect the arduino to the pc, not to a hub on a hub.
- Rotated monitors won't get captured. I will re-read how to solve this issue. 
- A lot of programs and games handle the DirectX full screen switch well enough to work with this program. But the following happens sometimes: \
Switching from or to a fullscreen mode programm causes an access loss of the GPU-variables or it gets stuck in an windows-api-function. Restart the program and switch / 'Alt+Tab' back to the game within 5 seconds. This accounts to every display mode switch (change of resolution, lockscreen, changing games from window mode to fullscreen, ...).
- Netflix' desktop app and some other apps and sites with strong DRM-protection will block the win desktop duplication api from grabbing the frames. 
Workaround: use Firefox or whatever... 

Porting it to Linux would require a replacement of the AcquireNextFrame()-function of the windows desktop duplication api and some other adjustments. \
I'd suggest to grab the backbuffer of the GPU and load it in a \<D3D11Texture2D\> while using a directX-swap-chain.
