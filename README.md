DRAGON's LAIR
=============

Ah .. such a nice game to play on the Bitbox ! (see Angry Video Game Nerd or Joueur du Grenier reviews ;) ... )

For more info about the game see : http://www.dragons-lair-project.com/games/pages/dl.asp

for Bitbox itself see main repo / wiki : https://github.com/makapuf/bitbox/

![dragons lair!](https://github.com/makapuf/bitbox-fmv/blob/master/dlair.jpeg?raw=true)

This game / engine is aimed at reproducing the dragons lair game on the Bitbox. It's only compatible with the standard bitbox since the data is streamed from SD card.

To play it
----------

The game should be playable almost like the original game, however only the video file has been used, not the program or the
Put the frames.ani animation on your SD card and load the fmv.bin file, then press the buttons just when needed .. or you'll die (often)


To build levels
----------------

Levels are typically built using blender, by inserting markers on the timeline and coding game logic as python.

Blender animation takes the video and encodes it to the needed framerate, resolution, which is currently 400x300 15fps.

In order to render an animation to game data :

    - Save the following video https://www.youtube.com/watch?v=znO_m00s8II to dragons_lair_arcade_game_laserdisc.mp4
    - Open the video_15fps.blend in Blender, then press the render/audio button to save audio as mixdown.wav
    - Now, do Render->Render Animation.
    - Still in Blender, change the editor back to Text Editor and run the python script post_render.py (alt+p).
    - On the command line, run make -j4 -f frames/Makefile.
    - Back in Blender, get to the final_build.py script and run it (alt+p). This will pack frames and audio to frames.ani
    - Back on the command line, type make, to create the bitbox and emulator executable.

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
