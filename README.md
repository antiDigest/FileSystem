# FileSystem
Google File System (Spoof)

## TODO:

### Version 0.1:

* Span read to more than one file

* Add replicas

* MServer creates chunk at three servers

* Writing to a chunk writes to all replicas

* Write uses two-phase protocol to commit write

* Handles multiple append requests from clients by holding requests

### Version 0.2:

* Read requests to be taken care of from any replica of the chunk

* A recovering chunk server may have missed appends to its chunks while it was down.
In such situations, the recovering chunk server, with the help of the M-server, 
determines which of its chunks are out of date, and synchronizes those chunks with
their latest version by copying the missing appends from one of the current replicas.

* M-Server needs to hold information of what chunks have to be updated at a failed server.

* When a failed server becomes live, M-Server updates all need-to-be-updated chunks at the 
failed server.

* Failed server resumes after all the chunks have been updated.