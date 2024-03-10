OUTNAME = exploit
PYTHON_SCRIPT = ./post-process.py

.PHONY: clean all

all: $(OUTNAME)-raw.txt

$(OUTNAME)-raw.txt: $(OUTNAME).s
	gcc -c $(OUTNAME).s
	objdump -d $(OUTNAME).o > $(OUTNAME).d
	PYTHON_SCRIPT $(OUTNAME).d
	./hex2raw < $(OUTNAME).txt > $(OUTNAME)-raw.txt

clean:
	rm $(OUTNAME).[od] $(OUTNAME).txt $(OUTNAME)-raw.txt
