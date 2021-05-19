# maxlight
## Alpha 1.0
This is a little tool to get the average screen color of a monitor of choice and send it to an rgb LED strip as fast as possible to tint a whole room in the displayed color. \
It is not another copy of an Ambilight system, but can be used for it. \
Instead, it's a room light to destress the eyes and raise the immersive power of entertainment setups. \
Razors' Lightbar, that's attachable to the monitor can also grab the average screen color, but it costs about 106$. Of course, it is way fancier designed and pre-definded effects can be controlled by an app, but it costs more and in case of screen capturing, the performance consumption is a bit higher. \
Keeping the CPU usage low was one of the goals, because gaming on my very own setup revealed: \
  my CPU is the performance bottleneck while the GPU stays cool and calm. 

```
Set the USB port number yourself. 
Inserting a higher saturation and brightness values in the setup will show you the current color accents of the screen. 
For watching a movie, I'd recommend lower values to get an ambient lighting and smoother transitions. 
Higher saturation values than zero can be used to ignore the 256 shades of grey (from black to white). 
A higher brighness value ignores dark pixels and raises the overall brightness of the LEDs. 
In combination with a low saturation value, a smooth and ambient effect can be created. 
Just play around. 
Btw.: Netflix' desktop app and some other apps and sites with drm protection will block the win desktop duplication api from grabbing the frames. 
Workaround: use Firefox or something else... 
```

The project got compiled by VS' release mode and only works on Windows 8.1 and higher with winSDK and x64. \
More features will come soon. 

Known bugs: Switching from or to a fullscreen mode programm cuases an access loss of the variables. Restart the Program and switch/'Alt+Tab' back to the game within 5 seconds. 

Porting it to Linux would require a replacement of the AcquireNextFrame() function of the windows desktop duplication api and some other adjustments. \
I'd suggest to grab the backbuffer of the GPU and load it in a \<D3D11Texture2D\>.
