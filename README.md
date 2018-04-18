# FileSystem
Google File System (Spoof)

## Version Log:

### Version 1:

* Span read to more than one file

* Add replicas

* MServer creates chunk at three servers

* Writing to a chunk writes to all replicas

* Write uses two-phase protocol to commit write

* Handles multiple append requests from clients by holding requests

* Read requests to be taken care of from any replica of the chunk

* A recovering chunk server may have missed appends to its chunks while it was down.
In such situations, the recovering chunk server, with the help of the M-server, 
determines which of its chunks are out of date, and synchronizes those chunks with
their latest version by copying the missing appends from one of the current replicas.

* M-Server needs to hold information of what chunks have to be updated at a failed server.

* When a failed server becomes live, M-Server updates all need-to-be-updated chunks at the 
failed server.

* Failed server resumes only after all the chunks have been updated.

### Version 0:

* Heartbeat messages from server to meta-server

* Meta-server detects failure of a server on three missed heartbeats

* Recovering server resumes sending heartbeat messages

* Failed server => unavailable chunks.

* Any request to read or write from/to unavailable chunks results in sending of "FAILED"
 to the client from meta-server

* Chunk size is set to 8192 bytes.

* A file can have any number of chunks.

* On a write to a file, if the chunk is exceeding the allowed chunksize, a new chunk is 
created on a random server to write the message.

* This write is split into two parts, first that fits into what can be written on the chunk 
and the second that has to be written to the new chunk.

* Read takes place at any random time, till the time the file is available.

* Clients wishing to create a file, read or append to a file in your file system send their 
request (with the name of the file) to the M-server. If a new file is to be created, the 
M-server randomly asks one of the servers to create the first chunk of the file, and adds an 
entry for that file in its directory. For read and append operations, based on the file name 
and the offset, the M-server determines the chunk, and the offset within that chunk where the 
operation has to be performed, and sends the information to the client. Then, the client 
directly contacts the corresponding server and performs the operations.

