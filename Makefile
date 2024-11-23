
all:
	gcc -o parent_proc parent_proc.c

abort:
	ps -ef | grep "./parent_proc" | grep -v grep | awk '{print $$2}' | xargs -r kill
	@echo "All parent_proc processes have been terminated."

clean:
	rm -f parent_proc ; clear
