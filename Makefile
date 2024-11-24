
all:
	gcc -o ipc ipc.c

abort:
	ps -ef | grep "./ipc" | grep -v grep | awk '{print $$2}' | xargs -r kill
	@echo "All parent_proc processes have been terminated."

clean:
	rm -f ipc ; ipcrm --all ; clear
