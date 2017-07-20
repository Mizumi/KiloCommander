default: server

server:
	gcc src/main/kiloCommanderExampleCalicoServer.c src/main/kiloCommander.c src/main/calico/driver/kilobotCalicoDriver.c -Isrc/main -Isrc/main/calico -Isrc/main/calico/driver -o server
	chmod +x server

clean:
	rm -rf server
