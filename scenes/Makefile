

OS := $(shell uname)

ifeq ($(OS), Linux)
	BLENDER=../../blender/blender
else
	BLENDER=/Applications/Blender.app/Contents/MacOS/Blender
endif

all : \
	../dist/level1.pnct \
	../dist/level1.scene \



../dist/level1.scene : level1-mockup.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- level1-mockup.blend:Level1 '$@'

../dist/level1.pnct : level1-mockup.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- level1-mockup.blend:Level1 '$@'

#../dist/pool.scene : pool.blend export-scene.py
#	$(BLENDER) --background --python export-scene.py -- 'pool.blend:Table 1' '$@'

#hexapod.pnct : hexapod.blend export-meshes.py
#	$(BLENDER) --background --python export-meshes.py -- hexapod.blend:Main '$@'
#hexapod.scene : hexapod.blend export-scene.py
#	$(BLENDER) --background --python export-scene.py -- hexapod.blend:Main '$@'
