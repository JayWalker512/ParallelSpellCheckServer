Spell
-----

Spell is a networked spell checking daemon authored by Brandon Foltz. 
Clients connect to the daemon via TCP sockets, and transmit words 
(delimited by newline characters). The daemon responds to the request with
either "<word> OK" or "<word> MISSPELLED" where <word> is substituted by 
the text sent by the client. The results of spell checking requests are 
logged in the file "log.txt".

Spell has several optional configuration parameters that should be passed as 
arguments to the program when starting:

    -t <number> : The number of worker threads to spawn. This also serves as an 
                  upper bound on the number of simultaneously connected clients.
                  The default number of threads is 4.
    -d <file>   : Dictionary file to use. Words should be listed one per line.
                  The default dictionary is the included file "words".
    -p <number> : TCP port to listen for incoming connections on. Default is 
                  port 2667.
                  
Background
----------

This program was for a class assignment at a university I won't name, because I don't want this to be especially easy to search. Inspiration and reference are all well and good, but don't copy this
because you won't learn anything. 
                  
Design Decisions
----------------

To facilitate quick spell checking, I implemented a Trie structure to contain
the dictionary (see here: https://en.wikipedia.org/wiki/Trie). Certainly a 
linear search of the word list would be sufficient and use less memory, though
it would not be nearly as fast. This implementation can check upwards of 100k
words per second per core on a modern machine. 

Testing
-------

It is desirable to really hammer the daemon process and encourage any race 
conditions to show themselves. To do this, I run the daemon with at least as 
many threads as I have cores available and then connect many clients to it
simultaneously using a long command string such as the following:

cat words | nc localhost 2667 & cat words | nc localhost 2667 &

With as many copies of "cat words | nc localhost 2667 &" concatenated to the
end of the string as you like. This is best done locally as network throughput
will limit concurrency when connected to a remote server. I check the log when
all the client processes finish and ensure that each word shows up exactly as
many times as there were clients connected.

License
-------

Copyright 2019 Brandon Foltz

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
