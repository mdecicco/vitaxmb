# vitaxmb
A more visually appealing shell for the PS Vita

# Notes
This doesn't actually do anything yet, other than present you with an xmb-like interface with no options

# Currently working on
- Improving font appearance by:
  - Implementing signed distance field fonts, generated at runtime from .ttf files and saved to the storage device
  - This will allow users to add custom .ttf fonts without having to deal with weird programs for rendering the SDF textures themselves
- Improving icon appearance by:
  - Maybe finding or making SVG versions of each XMB icon
  - Rendering the SVGs also as a signed distance field font
  - Calculating shadows in the icon shaders rather than baking them into the images
- Making icon sliding animation more responsive, and more similar to the PSP/PS3 XMB

# Coming up
- Making the application more data-oriented, for custom themes and custom menu items and such
- Adding the 'tick' sound effect when moving through the XMB
- Maybe finding the sound effect from the PSP that is played when there's an error
- Slide-out options pane like the PSP has
- USB/FTP menu item with option for which device to mount
- Loading a list of vita/psp games and showing them in the XMB (with icons/backgrounds but probably not PMFs)
- Figuring out how to start applications and make them return to this application
- If the above works out, overriding the PS button and rendering the XMB over the current application like the PS3 does

# Build Requirements
- wine (if on linux or OS X)
- PSP2CGC.exe, (set PSP2CGC to the path of that file in CMakeLists.txt) (google it if you don't have it)
- set PSVITAIP in CMakeLists.txt to your Vita's IP when FTP is enabled
- Move the contents of `$VITASDK/arm-vita-eabi/include/freetype2` to `$VITASDK/arm-vita-eabi/include`
  - the freetype include paths appear to be broken, so you have to do this if you want to use it
- curl (to send the files to the vita over FTP when built

# Installation
```
git clone https://github.com/mdecicco/vitaxmb.git
cd vitasdk
cmake .
make
make s (sends the .vpk to your vita, once FTP is enabled on the device)
```

# Adding shaders, fonts, icons
If you add a new file to `/resources/icons` or `/resources/fonts` you will need to:
```
cmake . (updates the makefile with the new resource files)
make (re-packs the .vpk with those files)
```

If you add a new shader to `/resources/shaders` you will need to:
```
cmake . (updates the makefile with the new shader sources)
make (build the new shaders)
cmake . (updates the makefile with the new compiled shaders that will be in `/build` if they compiled successfully)
```

# Updating without re-installing (FTP must be enabled on the vita, and the VPK must be installed)
`make u` Only sends eboot.bin to the device

`make uf` Only sends fonts to the device (from `/resources/fonts`)

`make ui` Only sends xmb icons to the device (from `/resources/icons`)

`make us` Only sends compiled shaders to the device (from `/build`)
