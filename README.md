# PNG Brute Forcer

**A small PNG Brute Forcing program I've written in C, building off of a CRC Computing Program written by minatu2d, over at [https://gist.github.com/minatu2d/a1bd400adb41312dd842](https://gist.github.com/minatu2d/a1bd400adb41312dd842)**

# Installation Instructions

```jsx
cd png_brute_force

gcc crcBrute.c pngcrc.c pngcrc.h -o png_brute_force

./png_brute_force <PNG file path> <starting width> <starting height> <ending width> <ending height>
```

# Example

For brute forcing a PNG with the name "map.png", with an estimate that the PNG is less than 1000x1000.

```bash
xxd scream.png | head -n 2

00000000: 8950 4e47 0d0a 1a0a 0000 000d 4948 4452  .PNG........IHDR
00000010: 0000 0000 0000 0000 0806 0000 0056 1aec  .............V..
```

```bash
./png_brute_force scream.png 0 0 1000 1000
```

![alt text](https://en.wikipedia.org/wiki/File:Small_scream.png)

Overall, this program will brute force PNGs pretty quickly. By avoiding invoking a new process by running the pngcheck program each time we want to check the dimensions, it makes things many times faster. Being able to compute the CRC in the program speeds things up immensely.

# Further work needed

To achieve even greater speeds, I'm going to be looking into replacing the fread() aspects of the crcBrute.c, in order to not have to use a FILE stream. The current implementation will mmap in the image, then use fmemopen() to convert that to a file pointer, leading to an extra fseek() being needed prior to each write, so as to not clobber values beyond the PNG dimensions.