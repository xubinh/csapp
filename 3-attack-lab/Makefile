OUTNAME = exploit

PYTHON_SCRIPT = ./dis2op.py
HEX2RAW = ./hex2raw

.PHONY: clean all

all: $(OUTNAME)-raw.txt

$(OUTNAME)-raw.txt: $(OUTNAME).s $(PYTHON_SCRIPT)
	gcc -c $(OUTNAME).s  # 将汇编代码汇编为机器代码
	objdump -d $(OUTNAME).o > $(OUTNAME).d  # 使用 objdump 反汇编以生成对应的机器码字节
	$(PYTHON_SCRIPT) $(OUTNAME).d $(OUTNAME).txt 40  # 使用 python 脚本提取所有机器码字节
	# $(HEX2RAW) < $(OUTNAME).txt > $(OUTNAME)-raw.txt  # 使用 hex2raw 生成攻击字符串

clean:
	rm $(OUTNAME).[od] $(OUTNAME).txt $(OUTNAME)-raw.txt
