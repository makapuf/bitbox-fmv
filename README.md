DRAGON's LAIR
=============

Ah .. such a nice game to play on the Bitbox ! (see Angry Video Game Nerd or Joueur du Grenier reviews ;) ... )

![./dlair.jpg](dragons lair !)

This game / engine is aimed at reproducing the dragons lair game on the Bitbox. It's only compatible with the standard bitbox since the data is streamed from SD card.

To play it
----------

The game should be playable almost like the original game, however only the video file has been used, not the program or the
Put the frames.ani animation on your SD card and load the fmv.bin file, then press the buttons just when needed .. or you'll die (often)


To build levels
----------------

Levels are typically build using blender, by inserting markers on the timeline and coding game logic as python.

Blender animation takes the video and encodes it to the neede framerate, resolution.
Render audio to mixdown.wav
and frames as is
Blender script post_render.py then exports a makefile which needs to be run

run  : make -j4 -f frames/Makefile
# .. wait "a bit" while video gets encoded
run final script ```build_final.py ``` from blender to pack everything to frames.ani

The orignial data used for this has been saved from : https://www.youtube.com/watch?v=znO_m00s8II (save it locally to dragons_lair_arcade_game_laserdisc.mp4)


Frame binary format / player logic
-----------------------------------

Most of the logic is encoded in the export script, the player executes each frame data and is actually quite simple.

Each frame is coded independently - allowing jump to any frame - , and gets encoded as :

    - u32 event_id, frame_id
    - u32 width, height
	- palette of 256 u16
    - btc frame as 1 u32 for each 4x4 block (ie size=w*h/4) -- see blog entry for btc4
    - u8 sound[1066]
    - padding up to next 512B

event_id is button_A, up,down,leftright, nothing(no action), or always.
frame_id is the number of the frame in case the event has been found to be true this frame. If not, continue to next frame.

that's it !
