CC = gcc
CFLAG_NO_LINK = -c
CFLAGS = -Wall -O3
RM = rm -f

.PHONY: all prog1 clean

prog1:
	$(CC) $(CFLAGS) -o prog1 textProcessing.c sharedRegion.c textProcessingFunctions.c -lpthread -lm

clean veryclean:
	$(RM) prog1