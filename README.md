# cs4700 project 2: FTP client

This program interprets the command line arguments as:
```
./4700ftp [operation] [param1] [param2]
```

## operations 
- `ls <URL>`
    - lists the contents of the directory in the FTP server
- `mkdir <URL>`
    - makes a directory on the server at the given path
- `rm <URL>`
    - removes a file at the given path
- `rmdir <URL>`
    - removes a directory on the server at the given path
- `cp <ARG1> <ARG2>`
    - copy a file from the path at ARG1 to the path at ARG2 
- `mv <ARG1> <ARG2>`
    - move a file from the path at ARG1 to the path at ARG2 

## URL format 
remote URLs are formatted as follows:

`ftp://[USER[:PASSWORD]@]HOST[:PORT]/PATH`

where USER is the username, PASSWORD is the password, HOST is the hostname to connect to (required), PORT is the port to connect to the host at, and PATH is the path on the remote server 

local URLs should be formatted as paths

## approach
my approach for this project followed the suggested approach. i started by parsing FTP URLs in a separate program, and then making that code fit my abstractions and functions.

mainly, i parsed the input to the program with `parse_args` into an operation struct, which contains the type of operation, and its parameters as param structs. a param struct contains any information needed for a local path or an FTP URL, making it versatile.

operations are then executed in `operation_execute`, which opens required sockets, and calls `send_request` to translate operations into sequences of requests. sending and receiving from and to files is done with `send_from_file` and `recv_to_file` respectively.

## challenges
my biggest challenges were string parsing in C (which sucks) and the autograder (which is still crashing while i write this). i was relatively familiar with sockets from the last project, and with file io in C/Linux from previous experience, so the requirements of FTP were not very foreign to me, except for the protocol itself.

## testing
i tested this project with trial and error, and a slew of debug statements.
