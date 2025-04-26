## Factorio's Terrain Texture Dimensions

* The top solid rectangle is 1024x64px (Width,Height) in size, and the top left
  is at 0,0.
* The middle rectangle is twice the size at 2048x128px in size, and the top
  left is at 0,128
* The bottom rectangle is twice the size of the middle at 4096x256px in size,
  and the top left is at 0,360 

## Plan for drawing tiled background

My current plan for drawing a tiled background is to have a single box vao with
a uniform buffer containing the tile to use at every position. I will then call
`glDrawElementsInstanced()` to draw that single box n times where n is the
number of squares to fill the whole screen.
  
The other important piece is going to be making sure I can give each instance
information for things like the texture to use and the offset of the square.
There are 3 ways I see to do this, uniform buffer objects (UBO), shader storage
buffer objects (SSBO), and instanced attribute arrays. It sounds like in this
case instanced attribute arrays make the most sense because I don't need to
access data from any other element besides the current one.

There is a chance that using attribute arrays might be more complicated than
just using a UBO or SSBO. If that is the case then I'll fall back to one of
those options.

### Steps

1. Get a single tile on the screen with a 64x64px texture.
2. Get multiple textures on the screen with instancing.
3. Pass an index of where the texture to use is to control the texture each
   instance uses.
4. Fill the entire screen with textured squares.
