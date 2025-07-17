#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int main()
{
	/* check if edgesoft service is running */
	/* start if not running */
	/* open FIFOs */
	/* configure edgesoft */
	/* 	get version
		get camera serial number
		set room height to 290 cm
		set use case to open space
		set periodic data period to 1 sec
		set grid to 400x400
		set 4 zones
		get status, wait for operational
	*/
	/* wait on periodic json messages and print them */

	std::cout << "edgesoft example application" << std::endl;
	int pipePassive = open("passive.fifo", O_RDONLY);
	if(pipePassive==-1) {
		std::cout << "failed to open passive.fifo: " << strerror(errno) << std::endl;
	} else {
		std::cout << "passive.fifo opened succesfully" << std::endl;
	}

	char message[1024];
	while(1) {
		ssize_t readBytes;
		readBytes = read(pipePassive,message,1024);
		if(readBytes==0) {
			std::cout << "EOF, connection closed" << std::endl << std::flush;
			break;
		} else if(readBytes>0) {
			message[readBytes]='\0';
			std::string messageStr(message);
			std::cout << messageStr;
		} else {
			std::cout << "error" << std::endl;
			printf("read error! %s\n", strerror(errno));
		}
	}

	return 0;
}