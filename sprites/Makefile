
OS := $(shell uname)

ifeq ($(OS), Linux)
	GIMP=gimp
else
	GIMP=/Applications/GIMP.app/Contents/MacOS/gimp
endif


../dist/trade-font.png ../dist/trade-font.atlas : trade.xcf trade-font.list extract-sprites.py pack-sprites
	rm -rf trade-font
	./extract-sprites.py trade-font.list trade-font --gimp='$(GIMP)'
	./pack-sprites ../dist/trade-font trade-font/*

../dist/the-planet.png ../dist/the-planet.atlas : the-planet.xcf the-planet.list trade.xcf trade-font.list extract-sprites.py pack-sprites
	rm -rf the-planet
	./extract-sprites.py the-planet.list the-planet --gimp='$(GIMP)'
	./extract-sprites.py trade-font.list the-planet --gimp='$(GIMP)'
	./pack-sprites ../dist/the-planet the-planet/*
