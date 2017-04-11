![alt tag](http://williamsamtaylor.co.uk/github-images/wpl-animation.gif)

<img align='right' width='150' height='150' src='https://image.flaticon.com/icons/svg/332/332433.svg' />

# Win32 Playback Library [![Build status](https://ci.appveyor.com/api/projects/status/o8afonef8k6qrs0k?svg=true)](https://ci.appveyor.com/project/william-taylor/wpl) [![Open Source Love](https://badges.frapsoft.com/os/v1/open-source.svg?v=102)](https://github.com/ellerbrock/open-source-badge/) [![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

WPL or Windows Playback Library is a small C++ library to play video files inside a normal window on the Win32 operating system. I built it as having an intro video in many of my OpenGL projects was something I really wanted to have. The library just wraps DirectShow and handles the painting of the window for the user. It has been used in my projects successfully and I have put it here if others are curious. 

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
* Set drawing region for window.

## Links

Find below some quick links to the technology used to build the application.

[DirectShow](https://msdn.microsoft.com/en-us/library/windows/desktop/dd390351(v=vs.85).aspx)
