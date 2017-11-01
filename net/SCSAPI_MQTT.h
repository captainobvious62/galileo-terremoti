//
// Created by enrico on 12/03/17.
//

#ifndef GALILEO_TERREMOTI_SCSAPI_MQTT_H
#define GALILEO_TERREMOTI_SCSAPI_MQTT_H

#include <string>
#include <map>
#include "Tcp.h"
#include "../Seismometer.h"
#include "mqttclient.h"
#include "SCSAPI.h"

#define API_KEEPALIVE   1
#define API_QUAKE       2
#define API_TIMEREQ     3
#define API_TIMERESP    4
#define API_CFG         5
#define API_DISCONNECT  6
#define API_TEMPERATURE 7
#define API_REBOOT      8
#define API_PING        9

class SCSAPI_MQTT : public SCSAPI {
public:

	SCSAPI_MQTT();
	~SCSAPI_MQTT();

	bool init() override;

	/**
	 * Alive
	 * @return The config as string (as returned to server)
	 */
	void alive() override;

	/**
	 * Send alert to server
	 * @param db Accelerometer values
	 * @param threshold Threshold value
	 */
	void terremoto(RECORD *db) override;

	/**
	 * Packet processor
	 */
	void tick() override;

	/**
	 * Get time as UNIX timestamp (eg. seconds since Jan 1 1970 UTC)
	 * @return UNIX time
	 */
	unsigned long getUNIXTime() override;

	/**
	 * Request time sync
	 */
	void requestTimeUpdate() override;

	bool ping() override;

private:

	MQTT_Client *mqtt;
	MQTT_Subscribe *mydev;
	byte buffer[MAXBUFFERSIZE];
	unsigned long lastNTPTime;
	unsigned long lastNTPMillis;
	char *clientid;
	char *personalTopic;
};


#endif //GALILEO_TERREMOTI_SCSAPI_H
