# Windows Playback Library

WPL is a C++ library to playback video files inside a normal Window on the Win32 operating system.

![alt tag](http://williamsamtaylor.co.uk/images/projects/wpl.png)

## Demo

![alt tag](http://williamsamtaylor.co.uk/github-images/wpl-animation.gif)

## Overview

The library is a simple wrapper on top of DirectShow and replaces a lot of boiler plate code with just a few simple classes.

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

```git clone https://github.com/WilliamTaylor/WPL```

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
