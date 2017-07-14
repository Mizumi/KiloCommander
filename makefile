default:
	gcc src/main/Main.c -o main
	chmod +x main
clean:
	rm -rf main
