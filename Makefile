build:
	gcc -g -Ilibtdmm main.c libtdmm/tdmm.c -o hw6
	@echo "build done"
run:
	./hw6