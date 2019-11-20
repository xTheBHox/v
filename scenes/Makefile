

OS := $(shell uname)

ifeq ($(OS), Linux)
	BLENDER=../../blender/blender
else
	BLENDER=/Applications/Blender.app/Contents/MacOS/Blender
endif

all : \
	../dist/level1.pnct \
	../dist/level1.scene \
	../dist/level2.pnct \
	../dist/level2.scene


../dist/level1.scene : v.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- v.blend:Level1 '$@'

../dist/level1.pnct : v.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- v.blend:Level1 '$@'
  
../dist/level2.scene : v.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- v.blend:L2 '$@'

../dist/level2.pnct : v.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- v.blend:L2 '$@'
