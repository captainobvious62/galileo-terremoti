//
// Created by ebassetti on 23/07/15.
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>
#include <sstream>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include "common.h"
#include "Utils.h"



#ifdef __linux__
#include <sys/sysinfo.h>
unsigned long Utils::freeRam() {
	struct sysinfo sys_info;
	if (sysinfo(&sys_info) == 0) {
		return sys_info.freeram;
	} else {
		return 0;
	}
}

uint32_t Utils::uptime() {
	struct sysinfo sys_info;
	if (sysinfo(&sys_info) == 0) {
		return (uint32_t)sys_info.uptime;
	} else {
		return 0;
	}
}

uint32_t Utils::millis() {
	struct timespec ts;
	unsigned theTick = 0U;
	clock_gettime( CLOCK_REALTIME, &ts );
	theTick  = ts.tv_nsec / 1000000;
	theTick += ts.tv_sec * 1000;
	return theTick;
}
#else

#if defined(OPENBSD) || defined(FREEBSD) ||defined(__APPLE__) || defined(__darwin__)

#include <sys/sysctl.h>
#include <sys/timeb.h>
#include <ifaddrs.h>
#include <net/if_dl.h>

unsigned long Utils::freeRam() {
	return 0;
}

uint32_t Utils::uptime() {
	struct timeval boottime;
	size_t len = sizeof(boottime);
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	if( sysctl(mib, 2, &boottime, &len, NULL, 0) < 0 )
	{
		return 0;
	}
	time_t bsec = boottime.tv_sec, csec = time(NULL);

	return (uint32_t)difftime(csec, bsec);
}

uint32_t Utils::millis() {
	timeb tb;
	ftime( &tb );
	time_t nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return (uint32_t)nCount;
}
#endif
#endif

bool Utils::fileExists(const char *filename) {
	return access(filename, F_OK) != -1;
}

std::string Utils::readFirstLine(std::string filename) {
	std::string buf = "";

	FILE *fp = fopen(filename.c_str(), "rb");
	if(fp) {
		char cb[1024];
		memset(cb, 0, 1024);
		fread(cb, 1023, 1, fp);
		fclose(fp);
		buf.append(cb);
	}

	return buf;
}

double Utils::atofn(const char *str, size_t max)  {
	size_t bufLen = strlen(str);
	char buf[bufLen+1];
	buf[bufLen] = 0;

	memcpy(buf, str, bufLen);

	max = (max <= bufLen ? max : bufLen);
	char termination = buf[max];
	buf[max] = 0;
	double ret = atof(buf);
	buf[max] = termination;
	return ret;
}

float Utils::absavg(int *buf, int size) {
	float ret = 0;
	for (int i = 0; i < size; i++) {
		ret += (buf[i] < 0 ? buf[i] * -1 : buf[i]);
	}
	return ret / size;
}

// Standard Deviation
double Utils::stddev(int *buf, int size, float avg) {
	// Formula: RAD ( SUM{i,size}( (x[i] - avg)^2 ) / (size - 1) )
	double sum = 0;
	for (int i = 0; i < size; i++) {
		sum += pow(buf[i] - avg, 2);
	}

	return sqrt(sum / (size - 1));
}

void Utils::delay(unsigned int ms) {
	usleep(ms * 1000);
}

uint64_t Utils::hton64(byte* bignum) {
	uint64_t aux = 0;
	uint8_t *p = (uint8_t*)bignum;
	int i;

	/* we get the ntp in network byte order, so we must
	 * convert it to host byte order. */
	for (i = 0; i < 4; i++) {
		aux <<= 8;
		aux |= *p++;
	} /* for */
	return aux;
}

float Utils::reverseFloat(const float inFloat) {
	float retVal;
	char *floatToConvert = ( char* ) & inFloat;
	char *returnFloat = ( char* ) & retVal;

	// swap the bytes into a temporary buffer
	returnFloat[0] = floatToConvert[3];
	returnFloat[1] = floatToConvert[2];
	returnFloat[2] = floatToConvert[1];
	returnFloat[3] = floatToConvert[0];

	return retVal;
}

std::string Utils::trim(std::string& str, char c) {
	size_t first = str.find_first_not_of(c);
	size_t last = str.find_last_not_of(c);
	return str.substr(first, (last-first+1));
}

std::string Utils::doubleToString(double d) {
	std::ostringstream strs;
	strs << d;
	return strs.str();
}

std::string Utils::getInterfaceMAC() {
	struct ifconf ifc;
	char buf[1024];
	bool success = false;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		return std::string("");
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
		return std::string("");
	}

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	unsigned char mac_address[6];

	for (; it != end; ++it) {
#ifdef __linux__
		struct ifreq ifr;
		strcpy(ifr.ifr_name, it->ifr_name);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
			if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
					memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
					success = true;
					break;
				}
			}
		} else {
			return std::string("");
		}
#else
#if defined(OPENBSD) || defined(FREEBSD) ||defined(__APPLE__) || defined(__darwin__)
		ifaddrs* iflist;
		if (getifaddrs(&iflist) == 0) {
			for (ifaddrs* cur = iflist; cur; cur = cur->ifa_next) {
				if ((cur->ifa_addr->sa_family == AF_LINK) &&
					(strcmp(cur->ifa_name, it->ifr_name) == 0) && cur->ifa_addr) {
					sockaddr_dl* sdl = (sockaddr_dl*)cur->ifa_addr;
					memcpy(mac_address, LLADDR(sdl), sdl->sdl_alen);
					success = true;
					break;
				}
			}

			freeifaddrs(iflist);
		}
#else
#error No definition for getInterfaceMAC()
#endif
#endif
	}

	if (success) {
		char buf1[100];
		memset(buf1, 0, 50);
		snprintf(buf1, 50, "%02x%02x%02x%02x%02x%02x",
				 mac_address[0], mac_address[1], mac_address[2],
				 mac_address[3], mac_address[4], mac_address[5]
		);
		return std::string(buf1);
	} else {
		return std::string("");
	}
}

int Utils::setNonblocking(int fd) {
	int flags;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
		flags = 0;
	}
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
