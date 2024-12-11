# Distributed Filesystem

## Authors
## [Bohdan Hashchuk](https://github.com/gashchukk)
## [Dmytro Khamula](https://github.com/hamuladm)

### Project description
This paper presents a distributed file system inspired by the Google File System (GFS), designed to handle large amounts of data on multiple computers. The system is built to be fault-tolerant and efficient, storing multiple copies of data and a master-slave architecture to manage files.

Architecture of GFS:

![](imgs/gfs.png)



### Compilation of the project
Use the following command to compile the whole project
```bash
make compile
```
It will create for you 3 instances of chunkNode project to emualate different computers.
Also there is a useful tool for cleaning the project
```bash
make clean
```
It removes executables, build folders, etc.


### To run client
```bash
./bin/client
```

### To run chunkNode server
```bash
./bin/node {port}
```

### To run masterNode server
```bash
./bin/masterNode
```

By default `masterMode` uses port 8080, so make sure you used different ports for `chunkNode`s. For instance, 8081, 8082, 8083 would do perfectly.

## How to run each instance independently
On each instance you have `./compile.sh` file that compiles only currect instance. Executable can be found in `bin` dir.

