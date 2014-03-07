NAME = kptt
SRC = gamucs.c asm.s
CONVERTER = sbcl --noinform --load pnm2c.lisp

all: title.h button.h mouse.h spin.h bat.h tunnel.h cheese.h
	lua generate.lua > tables.h
	cl65 -O -C c64.cfg -m $(NAME).map --lib c64.lib $(SRC) -o $(NAME)
	x64 +confirmexit $(NAME)

%.pnm: pictures/%.png
	convert -compress none $< $@

tunnel.h: tunnel.pnm
	$(CONVERTER) --eval '(convert "tunnel.pnm" "tunnel.h")'

cheese.h: cheese.pnm
	$(CONVERTER) --eval '(convert "cheese.pnm" "cheese.h")'

title.h: title.pnm
	$(CONVERTER) --eval '(add-color #(0 0 0))' \
		     --eval '(add-color #(255 168 0))' \
		     --eval '(add-color #(255 216 0))' \
		     --eval '(add-color #(64 64 64))' \
                     --eval '(convert "title.pnm" "title.h")'

button.h: button-z.pnm button-x.pnm \
          button-up.pnm button-down.pnm arrow.pnm
	$(CONVERTER) --eval '(add-file "button-up.pnm")' \
		     --eval '(add-file "button-down.pnm")' \
		     --eval '(add-file "button-z.pnm")' \
		     --eval '(add-file "button-x.pnm")' \
		     --eval '(add-file "arrow.pnm")' \
		     --eval '(straight "button.h")'

mouse.h: mouse-0.pnm mouse-1.pnm mouse-2.pnm mouse-3.pnm \
         mouse-4.pnm mouse-5.pnm mouse-6.pnm mouse-7.pnm
	$(CONVERTER) --eval '(add-file "mouse-0.pnm")' \
		     --eval '(add-file "mouse-1.pnm")' \
		     --eval '(add-file "mouse-2.pnm")' \
		     --eval '(add-file "mouse-3.pnm")' \
		     --eval '(add-file "mouse-4.pnm")' \
		     --eval '(add-file "mouse-5.pnm")' \
		     --eval '(add-file "mouse-6.pnm")' \
		     --eval '(add-file "mouse-7.pnm")' \
		     --eval '(straight "mouse.h")'

spin.h: spin-0.pnm spin-1.pnm spin-2.pnm spin-3.pnm \
        spin-4.pnm spin-5.pnm spin-6.pnm spin-7.pnm
	$(CONVERTER) --eval '(add-file "spin-0.pnm")' \
		     --eval '(add-file "spin-1.pnm")' \
		     --eval '(add-file "spin-2.pnm")' \
		     --eval '(add-file "spin-3.pnm")' \
		     --eval '(add-file "spin-4.pnm")' \
		     --eval '(add-file "spin-5.pnm")' \
		     --eval '(add-file "spin-6.pnm")' \
		     --eval '(add-file "spin-7.pnm")' \
		     --eval '(straight "spin.h")'

bat.h: bat-0.pnm bat-1.pnm bat-2.pnm bat-3.pnm \
       bat-4.pnm bat-5.pnm bat-6.pnm bat-7.pnm bat-8.pnm
	$(CONVERTER) --eval '(add-color #(0 0 0))' \
		     --eval '(add-color #(60 60 90))' \
		     --eval '(add-color #(60 90 90))' \
		     --eval '(add-color #(90 60 60))' \
	             --eval '(add-file "bat-0.pnm")' \
	             --eval '(add-file "bat-1.pnm")' \
	             --eval '(add-file "bat-2.pnm")' \
	             --eval '(add-file "bat-3.pnm")' \
	             --eval '(add-file "bat-4.pnm")' \
	             --eval '(add-file "bat-5.pnm")' \
	             --eval '(add-file "bat-6.pnm")' \
	             --eval '(add-file "bat-7.pnm")' \
	             --eval '(add-file "bat-8.pnm")' \
		     --eval '(straight "bat.h")'

clean:
	rm -f *.o *.map *.pnm *.h $(NAME)
