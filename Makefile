spell: main.c trie.c trie.h sck.c sck.h logger.c logger.h threadsafeQueue.c threadsafeQueue.h
	gcc -std=gnu99 -Wall -g main.c trie.c sck.c logger.c threadsafeQueue.c -o spell -lpthread

clean: 
	rm spell