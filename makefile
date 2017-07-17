default:
	gcc src/main/kiloCommanderExample.c src/main/kiloCommander.c -Isrc/main -o main
	chmod +x main
clean:
	rm -rf main
