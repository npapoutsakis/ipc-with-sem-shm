all:
	gcc -g -o ipc ipc.c -lrt

clean:
	rm -f ipc ; ipcrm --all ; clear
