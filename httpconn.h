#ifndef httpconn_h
#define httpconn_h 1

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
void *pthread_httpSend(void *ptr);
int tempseqid, tempsendingIter;
//debug
ssize_t byte_written;
struct SEQDB {
  struct RECORD *record;
  struct SEQDB *next;
};
//PTHREAD
pthread_t thread1;
int iret1;
IPAddress getFromString(char* ipAddr);


EthernetClient client;
byte inEvent = 0;
unsigned long seqid = 0;
unsigned long nextContact = 5000;
unsigned long nextContactMapped = 5;
boolean isSendingMapped;
boolean isLastMapped;
unsigned long stopClient = 0;
int seqDBfd;
unsigned long sendingIter = 0;
boolean isSending = false;

// get data from server to buffer line per line
int getLine(EthernetClient c, char* buffer, int maxsize, int toRead) { //???????
  int i;
  byte done = 0;
  memset(buffer, 0, maxsize);  // set the buffer to 0
  if (debugON) Serial.print("Header: ");
  for (i=0; i < maxsize-1 && done == 0; i++) {
    //while(!client.available() && client.connected());
    buffer[i] = c.read();
    if (debugON) Serial.print(buffer[i]);
    if (buffer[i] == '\r') i--;
    if (buffer[i] == '\n' || buffer[i] == -1) {  // if there is nothing more to read
      done = 1;
      buffer[i] = 0;
    }
    if (toRead == -1) {
    	// do nothing: it'll stop only if the buffer is emptied
    }
    else if (toRead > 1) {
      toRead--;
    }
    else {
      done = 1;
    }
  }

  if (debugON) Serial.println("");
  return i-1;
}

int getLine(EthernetClient c, char* buffer, int maxsize) {
  return getLine(c, buffer, maxsize, -1);
}

// convertv mac address to a string 
void hexToDecimal(char *mac_address ){
/*   for(int m=0; m < 6; m++) {// sending mac address
    if(mac[m] < 0x10) mac_address[m] = 0;
    sprintf(mac_address[m], "%hex", mac[m]);
  }   */
  Serial.println(mac_address);
}


int prepareFastBuffer(char* buf, struct RECORD *db, struct TDEF *td) {
	// ts, ms, pthresx, pthresy, pthresz, nthresx, nthresy, nthresz, deltax, deltay, deltaz
   if(debugON) Serial.print("mac testo: ");
   if(debugON) Serial.println(mac_string);
  return sprintf(buf, "tsstart=%u&deviceid=%s&lat=%.2f&lon=%.2f", db->ts, mac_string, lat,lon );
}

int prepareConfigBuffer(char* buf) {
	// ts, ms, pthresx, pthresy, pthresz, nthresx, nthresy, nthresz, deltax, deltay, deltaz
   if(debugON) Serial.print("mac testo: ");
   if(debugON) Serial.println(mac_string);
  return sprintf(buf, "deviceid=%s&lat=%.2f&lon=%.2f&version=%.2f&model=%s", mac_string, lat,lon, version, model );
}


int prepareFirstBuffer(char* buf, struct RECORD *db, struct TDEF *td) {
	// ts, ms, pthresx, pthresy, pthresz, nthresx, nthresy, nthresz, deltax, deltay, deltaz
  return sprintf(buf, "%ld;%ld;%f;%f;%f;%f;%f;%f;%ld;%ld;%ld",
                     db->ts, db->ms, td->pthresx, td->pthresy, td->pthresz, td->nthresx, td->nthresy, td->nthresz, db->valx, db->valy, db->valz);
}

int prepareBuffer(char* buf, struct RECORD *db) {
	// ts, ms, deltax, deltay, deltaz
  return sprintf(buf, "%ld;%ld;%ld;%ld;%ld\r\n", db->ts, db->ms, db->valx, db->valy, db->valz);
}

// open the file into the RAM of the device
int ramopen(int seqid, int sendingIter) {
	if (debugON) Serial.print("Opening file ");
	if (logON) log("Opening file ");
  // Send values
  char filename[100];
  sprintf(filename, "/media/ram/rel%d_%d.dat", seqid, sendingIter);
  if (debugON) Serial.println(filename);
  return open(filename, O_RDWR|O_CREAT);
}

// remove the file from the RAM of the device
void ramunlink(int seqid, int sendingIter) {
	if (debugON) {
		Serial.print("Removing file ret");
		Serial.print(seqid);Serial.print("_");
		Serial.print(sendingIter);
	}
	if (logON) log("Removing file ");
  // Send values
  char filename[100];
  sprintf(filename, "/media/ram/rel%d_%d.dat", seqid, sendingIter);
  if (debugON) Serial.println(filename);
  unlink(filename);
}

//######################## realHttpSendValues() #############################
void realHttpSendValues() {
  close(seqDBfd);
  seqDBfd = -1;
  tempseqid = seqid;
  tempsendingIter = sendingIter;
  
  isLastMapped = true; 
  isSendingMapped = true;
  nextContactMapped = 0;
  
  sendingIter++;
  seqDBfd = ramopen(seqid, sendingIter);
  if ((debugON) && (seqDBfd == -1)) Serial.print("Error in ramopen: realHttpSendValues");
  if ((logON) && (seqDBfd == -1)) Serial.print("Error in ramopen: realHttpSendValues");
  //int pid = fork();
  // pthread_creation -->pthread_httpSend()
  iret1 = pthread_create( &thread1, NULL, pthread_httpSend, NULL);
  if (iret1) {
   if (debugON) Serial.print("Error - pthread_create() 1 return code: ");
   if (debugON) Serial.println(iret1);
   delay(10);
   exit(EXIT_FAILURE);
  }
}

//######################## httpSendValues() #############################
// send the accelerometer values that got over the threshold
void httpSendValues(struct RECORD *db, struct TDEF *td) {
  if (inEvent == 1) {  // if an "event" is running
    if (seqDBfd != -1) {  // if the file that stores the sequence has not yet been closed
      byte_written = write(seqDBfd, db, sizeof(struct RECORD));  // write the sequence onto the file into RAM memory
      if (debugON) {
        //Serial.print("byte written on seqDBfd pthread: ");
      	//Serial.println(byte_written);
        if (byte_written != sizeof(struct RECORD)) Serial.println("byte written MISMATCH! ");
      }
      if (logON && byte_written != sizeof(struct RECORD)) log("byte written MISMATCH! -- httpSendValues");
    }
    //Serial.println(isSending ? "isSending: TRUE!!!!" : "isSending:  FALSE");
    if (isSending) { // IF IS SENDING DATA
      //Serial.println("isSending: TRUE");
      // Check if finished
      if (!isSendingMapped) {
        isSending = false;
      	if (debugON) Serial.println("Child ended");
      	if (logON) log("Child ended");
        
        nextContact = nextContactMapped;
        if (debugON) Serial.print("nextContactMapped: ");
        if (debugON) Serial.println(nextContactMapped);
        debugUNIXTime(nextContactMapped);

        if (isLastMapped) {
          //Serial.println("isLAST MAPPED TRUEEEEEEE: ");
          inEvent = 0;
          //Serial.println("ORA ATTUALE: ");
          debugUNIXTime(getUNIXTime()); 
          //Serial.println("Finished");
          close(seqDBfd);
          seqDBfd = -1;
          ramunlink(seqid, sendingIter);
          if (debugON) Serial.println("Storing finished");
          if (logON) log("Storing finished");
        }
        else {
          if (debugON) Serial.println("isLAST MAPPED FALSE: ");
        }
      }
    }
    else if (getUNIXTime() >= nextContact) {   // IF IS THE RIGHT TIME, CONTACT SERVER
    	if (debugON) Serial.println("Child starting");
    	if (logON) log("Child starting");
      isSending = true;
      realHttpSendValues();
    }
  }
  else {
    // New Event ----------------------------------------------------------
  	if (debugON) Serial.print("New Event, values (X-Y-Z): ");
  	if (logON) log("New Event, values (X-Y-Z): ");
    printRecord(db); // Debug print recorded axis values
    if (debugON) Serial.println();

    if(client.connect(httpServer, 80)) {	// Connecting to server to retrieve "sequence id"
    	if (debugON) Serial.print("Requesting SEQID to:");
    	if (debugON) Serial.println(httpServer);

      char rBuffer[300];
      int rsize = prepareFirstBuffer(rBuffer, db, td);  // prepare the info for the new entry into the DB

      client.print("POST ");
      client.print(path_domain);
      client.print("/device.php?op=put&mac=");
      for(int m=0; m < 6; m++) {
        if(mac[m] < 0x10) client.print("0");
        client.print(mac[m], HEX);
      }
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(httpServer);
      client.println("Content-Type: text/plain");
      client.print("Content-Length: ");
      client.println(rsize);
      client.println("Connection: close");
      client.println("");
      client.print(rBuffer);
      unsigned long responseMill = millis();
      // Reading headers
      //while(!client.available()){;} // Attende che arrivino i dati ******************
      while(!client.available() && !(millis() - responseMill > timeoutResponse ) ){;}
      int s = getLine(client, rBuffer, 300);
      if(strncmp(rBuffer, "HTTP/1.1 200", 12) == 0) {
        int bodySize = 0;
        do {
          s = getLine(client, rBuffer, 300);
          if(strncmp(rBuffer, "Content-Length", 14) == 0) {
            char* separator = strchr(rBuffer, ':');
            if(*(separator+1) == ' ') {
              separator += 2;
            } else {
              separator++;
            }
            bodySize = atoi(separator);
          }
        } while(s > 0);
        // Content
        s = getLine(client, rBuffer, 300, bodySize);

        char* separator = strchr(rBuffer, ';');  // ?
        *separator = 0;
        seqid = atol(rBuffer);  // get the sequence ID
        nextContact = atol(separator+1) + getUNIXTime();  // ?
        inEvent = 1;  // Set ON the Event 
        if (debugON) Serial.print("SEQID:");
        if (debugON) Serial.println(seqid);
        if (debugON) Serial.print("tempo offset per nextContact:");
        if (debugON) Serial.println(atol(separator+1));
        if (logON) log("SEQID:");
        if (logON) log(rBuffer);
        if (debugON){ Serial.print("Next Contact scheduled for new EVENT: ");}
        debugUNIXTime(nextContact);

        sendingIter = 0;
        seqDBfd = ramopen(seqid, sendingIter);
        if ((debugON) && (seqDBfd ==-1)) Serial.println("Error in ramopen: httpSendValues");
        if (logON && (seqDBfd ==-1)) log("Error in ramopen: httpSendValues");
      } else {
      	if (debugON) Serial.print("Error in reply: ");
      	if (debugON) Serial.println(rBuffer);
      	if (logON) log("Error in reply: ");
      	if (logON) log(rBuffer);
      }
      client.stop();
    }else{
      client.stop();
      if(debugON) Serial.println("Connection error");
      if(logON)log("connessione fallita");
      resetEthernet = true;
    }
    //free(db);
    if(debugON) Serial.println("exiting from - httpSendValues");
  }
}




// send the accelerometer values that got over the threshold
void httpSendAlert2(struct RECORD *db, struct TDEF *td) {
  // New Event ----------------------------------------------------------
  if (debugON){ 
    Serial.print("New Event, values (X-Y-Z): ");
    printRecord(db); // Debug print recorded axis values
    Serial.println();
  }
  if (logON) log("New Event, values (X-Y-Z): ");
  char rBuffer[300];
  bool sent = false;
  bool received = false;
  unsigned long prevSend = 0;
  int connection_status = 0;
  byte ntimes = 3; // numbers of connection tryies
  while((!sent || !received) && (millis() - prevSend > 1500) && ntimes > 0){// testare 
    ntimes--;
    // Connecting to server to retrieve "sequence id"
    if(!sent && (connection_status != 1)){
      connection_status = client.connect(httpServer, 80);
    }
    if(connection_status) { // if connection enstabilished	
      if(!sent){
        
        if (debugON){
          Serial.print("Connecting to:");
          Serial.println(httpServer);
        }
        
        int rsize = prepareFastBuffer(rBuffer, db, td);  // prepare the info for the new entry to be send to DB
        // sendig request to server
        client.print("POST ");
        client.print(path_domain);
        //client.print("/device.php?op=put1&mac="); // 
        client.print("/terremoto.php");
/*      for(int m=0; m < 6; m++) {// sending mac address
          if(mac[m] < 0x10) client.print("0");
          client.print(mac[m], HEX);
        } */
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(httpServer);
        /* client.println("Content-Type: text/plain"); */
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-Length: ");
        client.println(rsize);
        client.println("Connection: close"); // ???
        client.println("");
        client.println(rBuffer);
        if(debugON) Serial.print("sending Buffer: "); 
        if(debugON) Serial.println(rBuffer); 
        sent = true;
      }
    
      unsigned long responseMill = millis();
      // Attende che arrivino i dati con timeout nella risposta ***************
      while(!client.available() && (millis() - responseMill < timeoutResponse ) ){;}
      if (millis() - responseMill > timeoutResponse) Serial.println("TIMEOUT SERVER CONNECTION");
      if(client.available()){ // gestire il caso in cui la connessione con il server avviene ma i dati non arrivano
      // il problema sussiste nel fatto che vengono inviati di nuovo i dati al server
       // client has sent a response
        // Reading headers
        int s = getLine(client, rBuffer, 300);
        if(strncmp(rBuffer, "HTTP/1.1 200", 12) == 0) { // risposta ok dal server
          int bodySize = 0;
          do { // read from client response
            s = getLine(client, rBuffer, 300);
            if(strncmp(rBuffer, "Content-Length", 14) == 0) {
              char* separator = strchr(rBuffer, ':');
              if(*(separator+1) == ' ') {
                separator += 2;
              } else {
                separator++;
              }
              bodySize = atoi(separator); // get body size response
              //break; // stop 
            }
          } while(s > 0); // get data till new line
          // Content
          s = getLine(client, rBuffer, 300, bodySize); // get content size 
          //nextContact = atol(separator+1) + getUNIXTime();  // TIME FOR SENDING COLLECTED DATA
          nextContact = (atol(rBuffer) *1000UL);  // get next time to send new data
          inEvent = 1;  // Set ON the Event 
          milldelayTimeEvent = millis(); // timestamp in millis for Event Interval
          if (debugON){
            Serial.print("tempo offset per nextContact:");
            Serial.println(nextContact);
          }
          if (debugON){ Serial.print("Next Contact scheduled for new EVENT: ");}
          debugUNIXTime(nextContact);

          received = true;
          //sendingIter = 0;
  /*      seqDBfd = ramopen(seqid, sendingIter);
          if ((debugON) && (seqDBfd ==-1)) Serial.println("Error in ramopen: httpSendValues");
          if (logON && (seqDBfd ==-1)) log("Error in ramopen: httpSendValues"); */
        } else { // connetion response != 200
          if (debugON){ 
            Serial.print("Error in reply: ");
            Serial.println(rBuffer);
          }
          if (logON){ 
            log("Error in reply: ");
            log(rBuffer);
          }
          sent = false;
        }
     }else{ // client not responding - data not available
      if(logON){log("Client not available on: sendAlert2");}
      if(debugON){Serial.println("Client not available on: sendAlert2");}
     }
     //client.stop();
    }else{ // connection to server Failed!!!
      // client.stop();
      if(debugON) Serial.println("Connection error on sendAlert2");
      if(logON)log("connessione fallita");
      //resetEthernet = true; check if is there an Internet Connection!!!!!!!!!!!
    }
    prevSend = millis();
  if(received || (connection_status != 1)){ // if data received or connection failed close socket
    client.stop();
  }
 }
  //free(db);
  if(debugON) Serial.println("exiting from - httpSendAlert2");
}


// send the accelerometer values that got over the threshold
void httpTestConfig() {
  // New Event ----------------------------------------------------------
  if (debugON){ 
    Serial.println("Asking for config: ");
   
  }
  char rBuffer[300];
  bool sent = false;
  bool received = false;
  unsigned long prevSend = 0;
  int connection_status = 0;
  byte ntimes = 3; // numbers of connection tryies
  while((!sent || !received) && (millis() - prevSend > 1500) && ntimes > 0){// testare 
    ntimes--;
    // Connecting to server to retrieve "sequence id"
    if(!sent && (connection_status != 1)){
      connection_status = client.connect(httpServer, 80);
    }
    if(connection_status) { // if connection enstabilished	
      if(!sent){
        
        if (debugON){
          Serial.print("Connecting to:");
          Serial.println(httpServer);
        }
        
        int rsize = prepareConfigBuffer(rBuffer);  // prepare the info for the new entry to be send to DB
        // sendig request to server
        client.print("POST ");
        client.print(path_domain);
        //client.print("/device.php?op=put1&mac="); // 
        client.print("/alive.php");
/*      for(int m=0; m < 6; m++) {// sending mac address
          if(mac[m] < 0x10) client.print("0");
          client.print(mac[m], HEX);
        } */
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(httpServer);
        /* client.println("Content-Type: text/plain"); */
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-Length: ");
        client.println(rsize);
        client.println("Connection: close"); // ???
        client.println("");
        client.println(rBuffer);
        if(debugON) Serial.print("sending Buffer: "); 
        if(debugON) Serial.println(rBuffer); 
        sent = true;
      }
    
      unsigned long responseMill = millis();
      // Attende che arrivino i dati con timeout nella risposta ***************
      while(!client.available() && (millis() - responseMill < timeoutResponse ) ){;}
      if (millis() - responseMill > timeoutResponse) Serial.println("TIMEOUT SERVER CONNECTION");
      if(client.available()){ // gestire il caso in cui la connessione con il server avviene ma i dati non arrivano
      // il problema sussiste nel fatto che vengono inviati di nuovo i dati al server
       // client has sent a response
        // Reading headers
        int s = getLine(client, rBuffer, 300);
        if(strncmp(rBuffer, "HTTP/1.1 200", 12) == 0) { // risposta ok dal server
          int bodySize = 0;
          do { // read from client response
            s = getLine(client, rBuffer, 300);
            if(strncmp(rBuffer, "Content-Length", 14) == 0) {
              char* separator = strchr(rBuffer, ':');
              if(*(separator+1) == ' ') {
                separator += 2;
              } else {
                separator++;
              }
              bodySize = atoi(separator); // get body size response
              //break; // stop 
            }
          } while(s > 0); // get data till new line
          // Content
          s = getLine(client, rBuffer, 300, bodySize); // get content size 
          //nextContact = atol(separator+1) + getUNIXTime();  // TIME FOR SENDING COLLECTED DATA
          //nextContact = (atol(rBuffer) *1000UL);  // get next time to send new data
          if(strncmp(rBuffer, "server", 6) == 0){
            char* separator = strchr(rBuffer, ':');
            if(*(separator+1) == ' ') {
                separator += 2;
            } 
            //httpServer = strncpy(
            
          }
          
          inEvent = 1;  // Set ON the Event 
          milldelayTimeEvent = millis(); // timestamp in millis for Event Interval
          if (debugON){
            Serial.print("tempo offset per nextContact:");
            Serial.println(nextContact);
          }
          if (debugON){ Serial.print("Next Contact scheduled for new EVENT: ");}
          debugUNIXTime(nextContact);

          received = true;
          //sendingIter = 0;
  /*      seqDBfd = ramopen(seqid, sendingIter);
          if ((debugON) && (seqDBfd ==-1)) Serial.println("Error in ramopen: httpSendValues");
          if (logON && (seqDBfd ==-1)) log("Error in ramopen: httpSendValues"); */
        } else { // connetion response != 200
          if (debugON){ 
            Serial.print("Error in reply: ");
            Serial.println(rBuffer);
          }
          if (logON){ 
            log("Error in reply: ");
            log(rBuffer);
          }
          sent = false;
        }
     }else{ // client not responding - data not available
      if(logON){log("Client not available on: sendAlert2");}
      if(debugON){Serial.println("Client not available on: sendAlert2");}
     }
     //client.stop();
    }else{ // connection to server Failed!!!
      // client.stop();
      if(debugON) Serial.println("Connection error on sendAlert2");
      if(logON)log("connessione fallita");
      //resetEthernet = true; check if is there an Internet Connection!!!!!!!!!!!
    }
    prevSend = millis();
  if(received || (connection_status != 1)){ // if data received or connection failed close socket
    client.stop();
  }
 }
  //free(db);
  if(debugON) Serial.println("exiting from - httpSendAlert2");
}

// ask config to server - New Da finire
boolean getConfigNew() {
  boolean ret = false;
  if (client.connect(httpServer, 80)) {
  	if (debugON) Serial.print("Requesting CONFIG to: ");
  	if (debugON) Serial.println(httpServer);
    char rBuffer[300];
    int rsize = prepareConfigBuffer(rBuffer);  // prepare the info for the new entry to be send to DB
    // sendig request to server
    client.print("POST ");
    client.print(path_domain);
    client.print("/alive.php");
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(httpServer);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(rsize);
    client.println("Connection: close"); // ???
    client.println("");
    client.println(rBuffer);
    //client.println("");
    if(debugON) Serial.print("sending Buffer: "); 
    if(debugON) Serial.println(rBuffer); 
    
    delay(100); // ATTENDERE ARRIVO RISPOSTA!!!
    while (!client.available()) {;}  // Attendere che il client risponda
    Serial.println("Dati arrivati - getConfigUpdates");
    memset( rBuffer, 0, 300);
    // Reading headers
    int s = getLine(client, rBuffer, 300);

    if (strncmp(rBuffer, "HTTP/1.1 200", 12) == 0) {  // if it is an HTTP 200 response
      int bodySize = 0;
      do {
        s = getLine(client, rBuffer, 300);
        if (strncmp(rBuffer, "Content-Length", 14) == 0) {
          char* separator = strchr(rBuffer, ':');
          if (*(separator+1) == ' ') {
            separator += 2;
          }
          else {
            separator++;
          }
          bodySize = atoi(separator);
        }
      } while(s > 0);
      // Content
      
      do {
        s = getLine(client, rBuffer, 300);
        if (s > 0) {
          char* separator = strchr(rBuffer, ':');
          *separator = 0;
          char* argument = separator+1;
          if (strncmp(rBuffer, "server", 6) == 0) {
            free(httpServer);  // ?
            httpServer = (char*)malloc(strlen(argument)*sizeof(char));
            if(httpServer!=NULL){
              strcpy(httpServer, argument);
            }else{
              if (logON) log("Malloc FAILED - getConfigUpdates");
              if (debugON) Serial.println("Malloc FAILED - getConfigUpdates");
            }
          }
          else if(strncmp(rBuffer, "ntpserver", 9) == 0) {
            timeServer = getFromString(argument);
          }
          else if(strncmp(rBuffer, "script", 6) == 0) {
            separator = strchr(rBuffer, ':');
            separator++;
            char scriptTest[100] ;
            strncpy(scriptTest, separator, strlen(rBuffer));
            Serial.println("Script");
            Serial.println(separator);
          }
        }
      } while(s > 0);
      ret = true;
    }
    else {
    	if (debugON) Serial.print("Error in reply: ");
    	if (debugON) Serial.println(rBuffer);
    }
    client.stop();
  }else{
      client.stop();
      if(debugON) Serial.println("Connection error");
      if(logON)log("connessione fallita");
      resetEthernet = true;
  }
  return ret;
}


// send the accelerometer values that got over the threshold
void httpSendAlert(struct RECORD *db, struct TDEF *td) {
  // New Event ----------------------------------------------------------
  if (debugON){ 
    Serial.print("New Event, values (X-Y-Z): ");
    printRecord(db); // Debug print recorded axis values
    Serial.println();
  }
  if (logON) log("New Event, values (X-Y-Z): ");
  char rBuffer[300];
  bool sent = false;
  bool received = false;
  unsigned long prevSend = 0;
  int connection_status = 0;
  byte ntimes = 3; // numbers of connection tryies
  while((!sent || !received) && (millis() - prevSend > 1500) && ntimes > 0){// testare 
    ntimes--;
    // Connecting to server to retrieve "sequence id"
    if(!sent && (connection_status != 1)){
      connection_status = client.connect(httpServer, 80);
    }
    if(connection_status) { // if connection enstabilished	
      if(!sent){
        
        if (debugON){
          Serial.print("Requesting SEQID to:");
          Serial.println(httpServer);
        }
        
        int rsize = prepareFirstBuffer(rBuffer, db, td);  // prepare the info for the new entry to be send to DB
        // sendig request to server
        client.print("POST ");
        client.print(path_domain);
        //client.print("/device.php?op=put1&mac="); // 
        client.print("/device.php?op=put&mac=");
        for(int m=0; m < 6; m++) {// sending mac address
          if(mac[m] < 0x10) client.print("0");
          client.print(mac[m], HEX);
        }
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(httpServer);
        client.println("Content-Type: text/plain");
        client.print("Content-Length: ");
        client.println(rsize);
        client.println("Connection: close"); // ???
        client.println("");
        client.print(rBuffer);
        sent = true;
      }
    
      unsigned long responseMill = millis();
      // Attende che arrivino i dati con timeout nella risposta ***************
      while(!client.available() && (millis() - responseMill < timeoutResponse ) ){;}
      if(client.available()){ // gestire il caso in cui la connessione con il server avviene ma i dati non arrivano
      // il problema sussiste nel fatto che vengono inviati di nuovo i dati al server
       // client has sent a response
        // Reading headers
        int s = getLine(client, rBuffer, 300);
        if(strncmp(rBuffer, "HTTP/1.1 200", 12) == 0) { // risposta ok dal server
          int bodySize = 0;
          do { // read from client response
            s = getLine(client, rBuffer, 300);
            if(strncmp(rBuffer, "Content-Length", 14) == 0) {
              char* separator = strchr(rBuffer, ':');
              if(*(separator+1) == ' ') {
                separator += 2;
              } else {
                separator++;
              }
              bodySize = atoi(separator); // get body size response
              //break; // stop 
            }
          } while(s > 0); // get data till new line
          // Content
          s = getLine(client, rBuffer, 300, bodySize); // get content size 
          char* separator = strchr(rBuffer, ';');  // ?
          //*separator = 0;
          seqid = atol(rBuffer);  // get the sequence ID
          //nextContact = atol(separator+1) + getUNIXTime();  // TIME FOR SENDING COLLECTED DATA
          nextContact = (atol(separator+1) *1000UL);  // get next time to send new data
          inEvent = 1;  // Set ON the Event 
          milldelayTimeEvent = millis(); // timestamp in millis for Event Interval
          if (debugON){
            Serial.print("SEQID:");
            Serial.println(seqid);
            Serial.print("tempo offset per nextContact:");
            Serial.println(atol(separator+1));
            Serial.println(nextContact);
          }
          if (logON){ 
            log("SEQID:");
            log(rBuffer);
          }
          if (debugON){ Serial.print("Next Contact scheduled for new EVENT: ");}
          debugUNIXTime(nextContact);

          received = true;
          //sendingIter = 0;
  /*      seqDBfd = ramopen(seqid, sendingIter);
          if ((debugON) && (seqDBfd ==-1)) Serial.println("Error in ramopen: httpSendValues");
          if (logON && (seqDBfd ==-1)) log("Error in ramopen: httpSendValues"); */
        } else { // connetion response != 200
          if (debugON){ 
            Serial.print("Error in reply: ");
            Serial.println(rBuffer);
          }
          if (logON){ 
            log("Error in reply: ");
            log(rBuffer);
          }
          sent = false;
        }
     }else{ // client not responding - data not available
      if(logON){log("Client not available on: getMacAddressFromServer");}
      if(debugON){Serial.println("Client not available on: getMacAddressFromServer");}
     }
     //client.stop();
    }else{ // connection to server Failed!!!
      // client.stop();
      if(debugON) Serial.println("Connection error");
      if(logON)log("connessione fallita");
      //resetEthernet = true; check if is there an Internet Connection!!!!!!!!!!!
    }
    prevSend = millis();
  if(received || (connection_status != 1)){ // if data received or connection failed close socket
    client.stop();
  }
 }
  //free(db);
  if(debugON) Serial.println("exiting from - httpSendAlert");
}


//######################## *pthread_httpSend() #############################
//worker thread send on http*****************************
void *pthread_httpSend(void *ptr) {  // if its the child process
	int fd = ramopen(tempseqid, tempsendingIter);  // store the file descriptor for the child file
	if (fd == -1) {
		if(debugON){
			Serial.print("Error in ramopen fd: PTHREAD");
			Serial.println(fd);
		}
		if (logON) log("Error in ramopen fd: PTHREAD ");
	}

	int size = lseek(fd, 0, SEEK_END);  // get the size of the file
	lseek(fd, 0, SEEK_SET);  // set the pointer to the beginning of the file

	char *sendBuffer = (char*) malloc(0);
	int offset = 0;
	int r = 0;
	unsigned int totalValues = 0;
	boolean isLast = true;
	do {  // for as long as there is something to read
		struct RECORD *rec = (struct RECORD*)malloc(sizeof(struct RECORD));
		if (rec != NULL){
			r = read(fd, rec, sizeof(struct RECORD));    //READING EVENTS FILE ON RAM
			if(r > 0) {
				if(rec->overThreshold){
					isLast = false;
				}else{
					isLast = true;
				}
				totalValues++;

				char *rBuffer = (char *)malloc(300 * sizeof(char));
				if (rBuffer != NULL){
					int ls = prepareBuffer(rBuffer, rec);  // get the length of the string and store the string into the buffer
					sendBuffer = (char*) realloc(sendBuffer, offset+ls);
					memcpy(sendBuffer + offset, rBuffer, ls);
					offset += ls;
					free(rBuffer);
				}else{
					if (logON) log("MALLOC FAILED rBuffer - pthread_httpSend");
					if (debugON) Serial.println("MALLOC FAILED rBuffer - pthread_httpSend");
				}
			}
			free(rec);
		}else{
			if (logON) log("MALLOC FAILED - pthread_httpSend");
			if (debugON) Serial.println("MALLOC FAILED - pthread_httpSend");
			r = 0;// stop reading memory error
		}
	}while(r > 0);  // for as long as there is an Event to read

	if (debugON) {
		Serial.print("OFFSET STRINGHE: ");
		Serial.println(offset);
		Serial.println(isLast?"TRUE":"FALSE");
	}
	sendBuffer = (char*) realloc(sendBuffer, offset+1);
	sendBuffer[offset] = 0; //TERMINATE STRING

	if (client.connect(httpServer, 80) && totalValues > 0) {
		if (debugON) {
				Serial.print("Current time on Pthread: ");
				debugUNIXTime(nextContact);
		}
		if (debugON) {
			Serial.print("Sending ");
			Serial.print(totalValues);
			Serial.print(" values to:");
			Serial.println(httpServer);
		}

		client.print("POST ");
		client.print(path_domain);
		client.print("/device.php?op=put&mac=");
		for(int m=0; m < 6; m++) {
			if(mac[m] < 0x10) client.print("0");
			client.print(mac[m], HEX);
		}
		client.print("&seqid=");
		client.print(seqid);
		if(isLast) client.print("&last=1");
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(httpServer);
		client.println("Connection: close");
		client.print("Content-Length: ");
		client.println(offset);
		client.println("Content-Type: text/plain");
		client.println("");

		client.println(sendBuffer);  // SENDING BUFFER
    unsigned long responseMill = millis();
    // WAIT FOR SERVER RESPONCE
    while(!client.available() && !(millis() - responseMill > timeoutResponse ) ){;}
		char rBuffer[300];
		// Reading headers -----------SERVER RESPONCE--------------------
		int s = getLine(client, rBuffer, 300);
		if(strncmp(rBuffer, "HTTP/1.1 200", 12) != 0) {
			if (debugON) Serial.print("error in reply: ");
			if (debugON) Serial.println(rBuffer);
		}
		else {
			int bodySize = 0;
			do {
				s = getLine(client, rBuffer, 300);
				if(strncmp(rBuffer, "Content-Length", 14) == 0) {
					char* separator = strchr(rBuffer, ':');
					if(*(separator+1) == ' ') {
						separator += 2;
					} else {
						separator++;
					}
					bodySize = atoi(separator);
				}
			} while(s > 0);
			// Content
			s = getLine(client, rBuffer, 300, bodySize);
			nextContactMapped = atol(rBuffer) + getUNIXTime();
			isLastMapped = isLast;

			if (debugON) Serial.println("done");
			if (isLast) {  // debug only
				//inEvent = 0;
				if (debugON) Serial.println("No more relevant values, ending now- IS LAST: TRUE");
				if (logON) log("No more relevant values, ending now");
			}
			else {
				if (debugON) {
					Serial.print("Next Contact scheduled for: ");
					debugUNIXTime(nextContact);
				 }
			}
		}
		if (debugON) Serial.println("closing connection... ");
		client.stop();
	}else{
      client.stop();
      if(debugON) Serial.println("Connection error");
      if(logON)log("connessione fallita");
      resetEthernet = true;
  }

	free(sendBuffer);
	if (debugON) Serial.println("closed, freeing memory... ");
	// RM file
	close(fd);
	ramunlink(tempseqid, tempsendingIter);

	isSendingMapped = false;
	if (debugON) {
		Serial.println("RAM UNLINK: ");
		Serial.println("isSendingMapped:################ false");
	}
	if (debugON){
		Serial.println("PTHREAD DONE");
	}

	pthread_exit(NULL);
}

// store the given MAC address to a FILE into the SD card
void storeMacAddressToSD(char *validMAC) {
	macToFile = fopen(macAddressFilePath, "w");
	if (macToFile == NULL) {
		printf("Error opening file!\n");
		exit(1);
	}

	fprintf(macToFile, "%s", validMAC);
	fclose(macToFile);
}

// ask the server for a valid MAC address
void getMacAddressFromServer() {
	if (client.connect(httpServer, 80)) {
		client.print("GET ");
		client.print("/terremoti/galileo");
		client.print("/getMacAddress.php");

		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(httpServer);
		client.println("Connection: close");
		client.println("");
    unsigned long responseMill = millis();

		delay(100);
		char rBuffer[300];
		//while(!client.available()){;} // wait for data
    while(!client.available() && !(millis() - responseMill > timeoutResponse ) ){;}
    if(client.available()){
      int s = getLine(client, rBuffer, 300);
      Serial.print("rBuffer: ");
      Serial.println(rBuffer);
      if(strncmp(rBuffer, "HTTP/1.1 200", 12) == 0) {
        int bodySize = 0;
        do {
          s = getLine(client, rBuffer, 300);
          if(strncmp(rBuffer, "Content-Length", 14) == 0) {
            char* separator = strchr(rBuffer, ':');
            if(*(separator+1) == ' ') {
              separator += 2;
            } else {
              separator++;
            }
            bodySize = atoi(separator);
          }
        } while(s > 0);

        // Content
        s = getLine(client, rBuffer, 300, bodySize);
        if (s > 0) {
          Serial.print("MAC address: ");
          Serial.println(rBuffer);
          storeMacAddressToSD(rBuffer);  // store the MAC address
        }
        else {
          Serial.println("Error getting the content");
        }

      }
      else {
        Serial.println("Connection error");
      }
    }else{
      if(logON){log("Client not available on: getMacAddressFromServer");}
      if(debugON){Serial.println("Client not available on: getMacAddressFromServer");}
    }
		//client.stop();
	}else{
      //client.stop();
      if(debugON) Serial.println("Connection error");
      if(logON)log("connessione fallita");
  }
  client.stop();
}



// given a string made of pair of characters in HEX base, convert them in decimal base - OK
uint8_t* HEXtoDecimal(const char *in, size_t len, uint8_t *out) {
	unsigned int i, t, hn, ln;
	char *ptr;
	char tmp[2];
	long decimal;
	for (t = 0, i = 0; i < len; i+=2, ++t) {
		tmp[0] = in[i];
		tmp[1] = in[i+1];
		decimal = strtol(tmp, &ptr, 16);

		out[t] = decimal;
    if(decimal < 1) Serial.print(out[t],HEX);
		Serial.print(out[t],HEX);
		Serial.print(":");
	}

	return out;
}

// convert MAC address from char* to byte[]
void convertMACFromStringToByte() {
	macToFile = fopen(macAddressFilePath, "r");
	if (macToFile == NULL) {
		printf("Error opening file!\n");
		exit(1);
	}else{

    char validMAC[13];
    fread(validMAC, 12, 1, macToFile);
    validMAC[13] = '\0';
    HEXtoDecimal(validMAC, strlen(validMAC), mac);
    }
  
	fclose(macToFile);

}


void byteMacToString(byte mac_address[]){
  char macstr[18];
  snprintf(macstr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
  Serial.print("MAC ADDRESS TO STRING: ");
  Serial.println((char * )macstr);
  
}
 
#endif
