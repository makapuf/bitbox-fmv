#testkernel makefile
NAME = fmv
GAME_C_FILES = player.c
GAME_C_OPTS = -DVGAMODE_400 -DBITBOX_SAMPLERATE=16000 -DBITBOX_SNDBUF_LEN=1066

USE_SDCARD = 1

include $(BITBOX)/lib/bitbox.mk

# to make the video (this step is manual, because I'm lazy)
# - open the blender file, render audio to mixdown.wav and frames as is
# - run  : make -j4 -f frames/Makefile
# .. wait "a bit"
# run final script
