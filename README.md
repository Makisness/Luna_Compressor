#Luna Compressor
---
###C++ learning project


This mess of a creation is my barely functional image compressor.
Created to learn and practice C++.

It will take any JPG or PNG image and increase its size by ~6x
* decompressor coming soon
Currently, all of my decompressed files are .ppm files, which makes viewable images a mere 60x the original JPG size.
(very efficient)

The theory was entirely designed by me. But like all great technology, I merely re-invented a technology that has been around since the mid-1970s
Obviously for my first C++ project, I didnt think I was creating a state-of-the-art algorithm.

I implemented a subtly unique color quantization algorithm.

##Theory


Choose 256 colors.
Then, map all of the real colors from the original image to the nearest color from the palette.

I additionally represent each pixel of the original image as a integer that represents the index color from the palette.
Since there are 256 colors, the integer will be 1 byte.
So the minimum size of this compression algorithm is going to be ~1 byte * total pixels * palette size.
My palette is RGB (1 byte per channel) with 256 colors, so it's 3 bytes * 256. ~768 bytes + total pixels

The difference with my algorithm comes from the algorithm that selects the colors.

My algorithm finds every unique color in the image and sorts it by how often the color appears.
The idea is that most images have lots of similar colors. and lots of rare colors that only show up a few times.

Traditionally, it may make sense to sample just the 256 most frequent colors.

But my hypothesis is that all colors are valuable to an image. If you sample just a few less-frequent colors. 
Then you may have better options when finding the nearest color.

My goal was to choose my colors cleverly by analyzing the image and using the image data to inform how the colors should be selected
The function picks colors based on frequency in a logarithmic way.
Such that more frequent colors get picked more often, but there should be a few less frequent colors represented aswell.

### the function
Starting with a palette of colors sorted by frequency.

I then define a metric to measure how heavily weighted the color frequency is.
ie, what percentage of the colors make up what percentage of the total pixels?
This is the "sharpness" of the image.

If a few different colors make up a large portion of the image. The sharpness will be very high. (a photo of a pure blue sky)
if a a lot of colors make up the image in fairly even ratios, the sharpness will be ~1.

I scale and clamp the sharpness using a function, and this value is called beta.

This function loops through i between 0 and 255.
palette[i] = (i/255)^b * (totalcolors - 1)

The value that's outputted from this function is the index of the color selected.
If b = 1, the colors will be sampled linearly. So the frequency of the color will have no impact on the probability that it's chosen.
But as b goes up, more frequent colors will be sampled more often.

Once the palette is created. I follow the classical color quantization.
I loop through all of the pixels in the original image and I find the closest color using Euclidean distance.
The image isn't stored as pixel data, each pixel is represented as a 8bit int that represents the index of the color from the 256 color palette.
The final binary is labeled .luna 
Named after my cat.

The GitHub repo is a bit of a mess (and I committed it after I was finished with V1)
The code is... not great. But it was absolutely a fun project.
I have some ideas for stretch goals. 
* RLE encoding, slight optimization
* A better decompression algorithm so that the file is viewable without converting to PPM
* Iterative palettes and encoding (might help with banding)
* A refactor may be in order
* rewriting this readme (so many errors)


