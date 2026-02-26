.PHONY: build run

build:
	gcc -g -Ilibtdmm main.c libtdmm/tdmm.c -o hw6
	@echo "build done"
run:
	./hw6
valgrind:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./hw6