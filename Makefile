NAME = kptt.prg
SRC = gamucs.c asm.s
CONVERTER = @sbcl --noinform --load pnm2c.lisp

all: title.h button.h mouse.h spin.h bat.h tunnel.h cheese.h
	lua5.2 generate.lua > tables.h
	cl65 -t c64 $(SRC) -o $(NAME)
	c1541 -format kptt,00 d64 kptt.d64 \
		-attach kptt.d64 \
		-write kptt.prg kptt

vice: all
	x64 -autostartprgmode 1 +confirmonexit $(NAME)

mame: all
	mame -w -r 640x480 c64 -flop kptt.d64

%.ppm: pictures/%.png
	@magick -compress none $< $@

tunnel.h: tunnel.ppm
	@echo build TUNNEL
	$(CONVERTER) --eval '(convert "tunnel.ppm" "tunnel.h")'

cheese.h: cheese.ppm
	@echo build CHEESE
	$(CONVERTER) --eval '(convert "cheese.ppm" "cheese.h")'

title.h: title.ppm
	@echo build TITLE
	$(CONVERTER) --eval '(add-color #(0 0 0))' \
		     --eval '(add-color #(255 168 0))' \
		     --eval '(add-color #(255 216 0))' \
		     --eval '(add-color #(64 64 64))' \
                     --eval '(convert "title.ppm" "title.h")'

button.h: button-z.ppm button-x.ppm \
          button-up.ppm button-down.ppm arrow.ppm
	@echo build BUTTON
	$(CONVERTER) --eval '(add-file "button-up.ppm")' \
		     --eval '(add-file "button-down.ppm")' \
		     --eval '(add-file "button-z.ppm")' \
		     --eval '(add-file "button-x.ppm")' \
		     --eval '(add-file "arrow.ppm")' \
		     --eval '(straight "button.h")'

mouse.h: mouse-0.ppm mouse-1.ppm mouse-2.ppm mouse-3.ppm \
         mouse-4.ppm mouse-5.ppm mouse-6.ppm mouse-7.ppm
	@echo build MOUSE
	$(CONVERTER) --eval '(add-file "mouse-0.ppm")' \
		     --eval '(add-file "mouse-1.ppm")' \
		     --eval '(add-file "mouse-2.ppm")' \
		     --eval '(add-file "mouse-3.ppm")' \
		     --eval '(add-file "mouse-4.ppm")' \
		     --eval '(add-file "mouse-5.ppm")' \
		     --eval '(add-file "mouse-6.ppm")' \
		     --eval '(add-file "mouse-7.ppm")' \
		     --eval '(straight "mouse.h")'

spin.h: spin-0.ppm spin-1.ppm spin-2.ppm spin-3.ppm \
        spin-4.ppm spin-5.ppm spin-6.ppm spin-7.ppm
	@echo build SPIN
	$(CONVERTER) --eval '(add-file "spin-0.ppm")' \
		     --eval '(add-file "spin-1.ppm")' \
		     --eval '(add-file "spin-2.ppm")' \
		     --eval '(add-file "spin-3.ppm")' \
		     --eval '(add-file "spin-4.ppm")' \
		     --eval '(add-file "spin-5.ppm")' \
		     --eval '(add-file "spin-6.ppm")' \
		     --eval '(add-file "spin-7.ppm")' \
		     --eval '(straight "spin.h")'

bat.h: bat-0.ppm bat-1.ppm bat-2.ppm bat-3.ppm \
       bat-4.ppm bat-5.ppm bat-6.ppm bat-7.ppm bat-8.ppm
	@echo build BATS
	$(CONVERTER) --eval '(add-color #(0 0 0))' \
		     --eval '(add-color #(60 60 90))' \
		     --eval '(add-color #(60 90 90))' \
		     --eval '(add-color #(90 60 60))' \
	             --eval '(add-file "bat-0.ppm")' \
	             --eval '(add-file "bat-1.ppm")' \
	             --eval '(add-file "bat-2.ppm")' \
	             --eval '(add-file "bat-3.ppm")' \
	             --eval '(add-file "bat-4.ppm")' \
	             --eval '(add-file "bat-5.ppm")' \
	             --eval '(add-file "bat-6.ppm")' \
	             --eval '(add-file "bat-7.ppm")' \
	             --eval '(add-file "bat-8.ppm")' \
		     --eval '(straight "bat.h")'

clean:
	rm -f *.o *.map *.ppm *.h $(NAME) kptt.d64
