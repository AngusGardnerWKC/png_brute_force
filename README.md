# PNG Brute Forcer

**A small PNG Brute Forcing program I've written in C, building off of a CRC Computing Program written by minatu2d, over at [https://gist.github.com/minatu2d/a1bd400adb41312dd842](https://gist.github.com/minatu2d/a1bd400adb41312dd842)**

**This program will take a PNG image with invalid dimensions (0x0, for example) and will brute-force the dimensions of the image, computing the CRC for the first chunk of the PNG to check if the dimensions are valid. This can be modified to check the CRC for all chunks in the image, but the purpose of this program is to simply check the dimensions.** 

# Why?

Because it's speedy, and it was a fun tiny project in C which builds off of something from last year in Python. Brute forcing should be quick, and writing the whole thing in C complements that.

# Installation Instructions

```jsx
cd png_brute_force

gcc crcBrute.c pngcrc.c pngcrc.h -o png_brute_force

./png_brute_force <PNG file path> <starting width> <starting height> <ending width> <ending height>
```

# Example

For brute forcing a PNG with the name "scream.png", with an estimate that the PNG is less than 1000x1000.

```bash
xxd scream.png | head -n 2

00000000: 8950 4e47 0d0a 1a0a 0000 000d 4948 4452  .PNG........IHDR
00000010: 0000 0000 0000 0000 0806 0000 0056 1aec  .............V..
```

```bash
./png_brute_force scream.png 0 0 1000 1000
```

![](https://github.com/AngusGardnerWKC/png_brute_force/blob/main/scream.png)

Overall, this program will brute force PNGs pretty quickly. By avoiding invoking a new process by running the pngcheck program each time we want to check the dimensions, it makes things many times faster. Being able to compute the CRC in the program speeds things up immensely.

# How does it work?

Initially, the chosen PNG is mmap'd into the current address space, then converted to a file pointer using fmemopen(). This is because the original code for computing the CRC handles everything as if it is reading from a PNG in one iteration. Once loaded in, the first chunk of the PNG gets run through the CRC checker to make sure it's okay. This means that we only have to check the width and height chunk at the beginning, instead of every single chunk in the PNG. If the PNG has invalid dimensions, then the CRC will be wrong. If this is the case, then the program will do the following:

1. Take the starting width and height from the user at runtime and use these as a starting point
2. Change these values to Big Endian to mimic that of the PNG file structure
3. memcpy() these values into the correct offsets in the PNG file according to the PNG header specification
4. Check the CRC on the now modified chunk
5. Repeat until a valid CRC is reached.

All of this happens on the file in memory, removing the need to consistently open/close the PNG, and also removing the need to begin a new process such as running pngcheck on the newly generated image. 

I'm not yet experienced enough to go through and rewrite the functions that handle the file pointer input, and convert them into memcpy() statements to read from the buffer. However, the implementation that I've got works particularly well.

# Further work needed

To achieve even greater speeds, I'm going to be looking into replacing the fread() aspects of the crcBrute.c, in order to not have to use a FILE stream. The current implementation will mmap in the image, then use fmemopen() to convert that to a file pointer, leading to an extra fseek() being needed prior to each write, so as to not clobber values beyond the PNG dimensions.

[https://stackoverflow.com/questions/2438953/how-is-fseek-implemented-in-the-filesystem](https://stackoverflow.com/questions/2438953/how-is-fseek-implemented-in-the-filesystem)

Overall though, it seems that fseek() isn't actually too bad due to the aggressive caching algorithms of modern OS's. So whilst removing fseek() and FILE streams from the program entirely may yield some slight performance increase, the amount would likely be very small.

Something else I've noticed is that whilst having printf() run on every iteration is nice, because it shows the progress, it actually slows the program down a LOT. We're adding an enormous amount of instructions in just to print the same stuff to the screen. Taking this out is easy. 
