![alt tag](http://williamsamtaylor.co.uk/github-images/wpl-animation.gif)

<img align='right' width='150' height='150' src='https://image.flaticon.com/icons/svg/148/148744.svg' />

# Windows Playback Library

WPL or Windows Playback Library is a small C++ library to play video files inside a normal Window on the Win32 operating system.
I built it as having an intro video in many of my OpenGL projects was something I really wanted to have. The library just wraps
DirectShow and handles the painting of the window for the user. It has been used in my projects successfully and I have put it here
if others are curious.

## Example

```c++

// Create a player & open a file
VideoPlayer videoPlayer;
videoPlayer.openVideo("demo.wmv");

// Set player state
videoPlayer.pause();
videoPlayer.stop();
videoPlayer.play();

// Notify the player to re render the window
videoPlayer.updateVideoWindow();
videoPlayer.repaint();

// State check functions
videoPlayer.hasFinished();
videoPlayer.hasVideo();

```

## Installation

If you would like to build the library you can download the project with a simple clone.

```git clone https://github.com/william-taylor/WPL```

Once you have done that you can run the example or build the library yourself from inside Visual Studio.

## Features

* Load Avi/Wmv Video Files
* DirectX based drawing
* The ability to Pause, Stop and Resume Videos.
* Tell when a video has finished.

## Roadmap

* Adjust the playback speed.
* Disable and control audio.
* Set drawing region.

## Links

Find below some quick links to the technology used to build the application.

[DirectShow](https://msdn.microsoft.com/en-us/library/windows/desktop/dd390351(v=vs.85).aspx)
