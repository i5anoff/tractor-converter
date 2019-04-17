Tractor converter
=======

This converter was made to modify resources of Vangers game. https://github.com/KranX/Vangers

It can perform the following conversions:

* .m3d/.a3d -> Wavefront .obj -> .m3d/.a3d.
* Vangers item .bmp -> indexed Truevision .tga -> Vangers item .bmp.
* Common image and video formats -> Vangers item preview .avi. Requires [GIMP](https://www.gimp.org/) and [FFmpeg](https://ffmpeg.org/) executables for this conversion.
* Vangers end screen .bmp -> indexed Truevision .tga -> Vangers end screen .bmp.



How to use
=======

Download the [latest release](https://github.com/tractortractor/tractor-converter/releases/latest) and read the [docs](./docs). [en](./docs/en) and [ru](./docs/ru) subfolders contains exact instructions and limitations. [common](./docs/common) subfolder contains useful tables and images.

Video guides: https://www.youtube.com/watch?v=ykKCx6cgkqA&list=PL-AfSpdu5KiUcdAZMFrtokJtNxctVKey3

Demo video: https://www.youtube.com/watch?v=T9ZYQcRSpNk&list=PL-AfSpdu5KiUcdAZMFrtokJtNxctVKey3



Donate
=======

If you like the software, please consider a donation.

https://my.qiwi.com/form/Andrei-KdYrSoJFgk



Included libraries
=======

All included libraries were modified.

* alphanum (MIT License) http://davekoelle.com/alphanum.html
* tinyobjloader (MIT License) https://github.com/syoyo/tinyobjloader/
* volInt (public domain) https://people.eecs.berkeley.edu/~jfc/mirtich/massProps.html



Dependencies
=======

* [Boost](https://www.boost.org/) 1.60 or higher
  * system
  * filesystem
  * program_options
