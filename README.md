# vitaxmb
A more visually appealing shell for the PS Vita

# Notes
This doesn't actually do anything yet, other than present you with an xmb-like interface with options that don't do anything yet

# Currently working on
- Implementing XMB navigation logic

# Features
- Customizable XMB options
- Customizable theme
- TTF font support
- Font glyph map caching (rendering glyphs from a TTF to a SDF glyph map turned out to be expensive)

# Coming up
- Figuring out why rendered text is ugly af
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
If you add a new file to `/resources/icons`, `/resources/fonts`, or `/resources/config` you will need to:
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

`make uc` Only sends config files to the device (from `/resources/config`)

# Thanks to:
xerpi - libvita2d, gxmfun (reference material)

other libvita2d contributors - libvita2d (reference material)

FirebirdTA01 - Vita3D_Sample_cpp (reference material)

poly04 - libvita3d (reference material)

Rinnegatamante - vitaGL (reference material)

Everyone who worked to provide people with the ability to make PS Vita applications

