//
// Created by ebassetti on 20/08/15.
//

#include "IPaddr.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

IPaddr::IPaddr() {
	ipaddr = 0;
}

IPaddr::IPaddr(uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4) {
	ipaddr = (x1*16777216)+(x2*65536)+(x3*256)+x4;
}

//IPaddr::IPaddr(int x1, int x2, int x3, int x4) {
//	IPaddr((uint8_t)x1, (uint8_t)x2, (uint8_t)x3, (uint8_t)x4);
//}

IPaddr::IPaddr(uint32_t x) {
	ipaddr = x;
}

IPaddr::operator uint32_t() {
	return ipaddr;
}

IPaddr& IPaddr::operator=(uint32_t address) {
	ipaddr = address;
	return *this;
}

IPaddr IPaddr::resolve(std::string hostname) {
	struct hostent *dnsLookup = gethostbyname(hostname.c_str());
	if(dnsLookup == NULL) {
		printf("Unable to resolve %s\n", hostname.c_str());
		return false;
	}
	uint32_t s_addr;
	memcpy(&s_addr, dnsLookup->h_addr, dnsLookup->h_length);
	return IPaddr(htonl(s_addr));
}

IPaddr IPaddr::localIP() {
	struct ifreq ifr;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	return IPaddr(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
}

std::string IPaddr::asString() {
	char buf[512];

	uint8_t *p1 = (uint8_t*)&ipaddr;
	snprintf(buf, 512, "%u.%u.%u.%u", p1[3], p1[2], p1[1], p1[0]);

	return std::string(buf);
}