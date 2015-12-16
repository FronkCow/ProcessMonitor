all:
	g++ -pthread monitorMain.cpp -o monitor

install: all
	cp -f monitor /usr/local/bin/monitor

uninstall:
	rm -f /usr/local/bin/monitor

clean:
	rm -rf monitor
