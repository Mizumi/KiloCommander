default: lib server

lib:
	gcc -shared -o libkilobotcalicodriver.so -fPIC src/main/kiloCommander.c src/main/calico/driver/kilobotCalicoDriver.c -Isrc/main -Isrc/main/calico -Isrc/main/calico/driver


lib-mac: lib
	mv libkilobotcalicodriver.so libkilobotcalicodriver.dylib
	
server:
	gcc src/main/kiloCommanderExampleCalicoServer.c -Isrc/main -Isrc/main/calico -Isrc/main/calico/driver -L. -lkilobotcalicodriver -o server
	chmod +x server

clean:
	rm -rf server
	rm -rf libkilobotcalicodriver.so
