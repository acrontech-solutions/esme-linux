#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

constexpr std::string_view GET_VERSION = "{\"get\":{\"cde\":\"versions\"}}\n";
constexpr std::string_view GET_CAM_ID = "{\"get\":{\"cde\":\"id\"}}\n";
constexpr std::string_view GET_STATUS = "{\"get\":{\"cde\":\"status\"}}\n";
constexpr std::string_view SET_HEIGHT = "{\"post\":{\"cde\":\"heightPosition\",\"args\":290}}\n";
constexpr std::string_view SET_USECASE = "{\"post\":{\"cde\":\"useCases\",\"args\":\"openSpace\"}}\n";
constexpr std::string_view SET_JSON_PERIOD = "{\"post\":{\"cde\":\"config\",\"args\":{\"json_per\":1}}}\n";


void sendCommand(int fd, std::string command)
{
    ssize_t writtenBytes;
    writtenBytes = write(fd,command.c_str(),command.size());
    if(writtenBytes!=command.size()) {
    	std::cout <<"write error" << std::endl;
    } else {
    	std::cout << "command sent: " << command << std::endl;
    }
}

void waitForResponse(int fd)
{
	char message[1024];
	ssize_t readBytes;
	readBytes = read(fd,message,1024);
	if(readBytes==0) {
		std::cout << "EOF, connection closed" << std::endl << std::flush;
	} else if(readBytes>0) {
		message[readBytes]='\0';
		std::string messageStr(message);
		std::cout << messageStr;
	} else {
		std::cout << "read error: " << strerror(errno) << std::endl;
	}
}

int main()
{
	/*	open FIFOs
		configure edgesoft
		get version
		get camera serial number
		set room height to 290 cm
		set use case to open space
		set periodic data period to 1 sec
		set 4 zones
		get status, wait for operational
		wait on periodic json messages and print them
		*/

	std::cout << "edgesoft example application" << std::endl;

	/* Via this FIFO, edgesoft application sends result periodically. */
	int pipePassive = open("passive.fifo", O_RDONLY);
	if(pipePassive==-1) {
		std::cout << "failed to open passive.fifo: " << strerror(errno) << std::endl;
	} else {
		std::cout << "passive.fifo opened succesfully" << std::endl;
	}

	/* Via this FIFO, this example application can request things, like change use-case, get list of zones. */
	int pipeMiso = open("miso.fifo", O_WRONLY);
	if(pipeMiso==-1) {
		std::cout << "failed to open miso.fifo: " << strerror(errno) << std::endl;
	} else {
		std::cout << "miso.fifo opened succesfully" << std::endl;
	}

	/* Via this FIFO, edgesoft respondes to commands with status, and return values, like camera ID, or list of zones. */
	int pipeMosi = open("mosi.fifo", O_RDONLY);
	if(pipeMosi==-1) {
		std::cout << "failed to open mosi.fifo: " << strerror(errno) << std::endl;
	} else {
		std::cout << "mosi.fifo opened succesfully" << std::endl;
	}


	sendCommand(pipeMiso,std::string(GET_VERSION));
	waitForResponse(pipeMosi);

	sendCommand(pipeMiso,std::string(GET_CAM_ID));
	waitForResponse(pipeMosi);

	sendCommand(pipeMiso,std::string(GET_STATUS));
	waitForResponse(pipeMosi);

	sendCommand(pipeMiso,std::string(SET_HEIGHT));
	waitForResponse(pipeMosi);

	sendCommand(pipeMiso,std::string(SET_USECASE));
	waitForResponse(pipeMosi);

	sendCommand(pipeMiso,std::string(SET_JSON_PERIOD));
	waitForResponse(pipeMosi);

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
			std::cout << "read error: " << strerror(errno) << std::endl;
		}
	}

	close(pipePassive);
	close(pipeMiso);
	close(pipeMosi);

	return 0;
}