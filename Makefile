mschedule:
	gcc -Wall -o mschedule  mschedule.c -lpthread -lm

clean:
	rm -f mschedule