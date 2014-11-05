#include <string.h>
#include "jsn260.h"
#include "Debug.h"

JSN260 *JSN260::instance;

JSN260::JSN260(Stream *serial)
{
	instance = this;
	this->serial = serial;
	setTimeout(DEFAULT_WAIT_RESPONSE_TIME);
	associated = false;
	error_count = 0;
}

JSN260::JSN260(Stream &serial)
{
	instance = this;
	this->serial = &serial;
	setTimeout(DEFAULT_WAIT_RESPONSE_TIME);
	associated = false;
}

int JSN260::available()
{
	return serial->available();
}

int JSN260::read()
{
	return serial->read();
}

int JSN260::peek()
{
	return serial->peek();
}

void JSN260::flush()
{
	return serial->flush();
}

size_t JSN260::write(uint8_t c)
{
	return serial->write(c);
}

size_t JSN260::write(const uint8_t *buffer, size_t size)
{
	return serial->write(buffer, size);
}

void JSN260::reset()
{
	sendCommand("at+reset\r", "[OK]");
	delay(1000);
}
void JSN260::prompt() {
	sendCommand("at\r", "[OK]");
	delay(1000);
}
boolean JSN260::save()
{
	if (sendCommand("at+save\r", "[OK]")) {
		return true;
	}
	return false;
}

boolean JSN260::leave()
{
	if (sendCommand("at+wd\r", "[OK]")) {
		associated = false;
		return true;
	}
	return false;
}

boolean JSN260::staticIP(const char *ip, const char *mask, const char *gateway)
{
    boolean result = false;
    char cmd[MAX_CMD_LEN];
//    reset(); 

	sendCommand("at+ndhcp=0\r", "[OK]"); 
	delay(10);
    sendCommand("at+nset=");
    snprintf(cmd, MAX_CMD_LEN, "%s,%s,%s\r", ip, mask, gateway);
    sendCommand(cmd, "[OK]");
    return true;
}

boolean JSN260::connect(const char *host, uint16_t port, const char *protocol)
{
	char cmd[MAX_CMD_LEN];

	if (!strcmp(protocol, "TCP")) {
		snprintf(cmd, sizeof(cmd), "at+nauto=0,1,%s,%d\r", host, port);
	}
	else if (!strcmp(protocol, "UDP")) {
		snprintf(cmd, sizeof(cmd), "at+nauto=0,0,%s,%d\r", host, port);
	}

	sendCommand(cmd,"[OK]");
	delay(10);
	if (sendCommand("ata\r", "CONNECT")) {
		clear();
		return true;
	}
	return false;
}

boolean JSN260::connect(uint16_t localport, const char *protocol)
{
	char cmd[MAX_CMD_LEN];
	
	if (!strcmp(protocol, "TCP")) {
		snprintf(cmd, sizeof(cmd), "at+nauto=1,1,0.0.0.0,%d\r", localport);
	}
	else if (!strcmp(protocol, "UDP")) {
		snprintf(cmd, sizeof(cmd), "at+nauto=1,0,0.0.0.0,%d\r", localport);
	}
	
	sendCommand(cmd,"[OK]");
	delay(10);
	if (sendCommand("ata\r", "CONNECT")) {
		clear();
		return true;
	}	
	return true;
}

boolean JSN260::join(const char *ssid, const char *phrase, const char *auth)
{
	char cmd[MAX_CMD_LEN];
	boolean result = false;
	sendCommand("at\r", "[OK]"); 
	delay(10);
	// ssid
	snprintf(cmd, MAX_CMD_LEN, "at+wauto=0,%s\r", ssid);
	sendCommand(cmd,"[OK]");

	//key
	if (!strcmp(auth, "WPA2") || !strcmp(auth, "WPA")) {
		snprintf(cmd, MAX_CMD_LEN, "at+wwpa=%s\r", phrase);
		sendCommand(cmd,"[OK]");
	}
	else if (!strcmp(auth, "WEP")) {
		snprintf(cmd, MAX_CMD_LEN, "at+wwep=%s\r", phrase);
		sendCommand(cmd,"[OK]");
	}

	return true;
}
 
int JSN260::send(const uint8_t *data, int len, int timeout)
{
    int write_bytes = 0;
    boolean write_error = false;
    unsigned long start_millis;

    if (data == NULL) {
        return 0;
    }
    while (write_bytes < len) {
        if (write(data[write_bytes]) == 1) {
            write_bytes++;
            write_error = false;
        } else {         // failed to write, set timeout
            if (write_error) {
                if ((millis() - start_millis) > timeout) {
                    DBG("Send data. Timeout!\r\n");
                    break;
                }
            } else {
                write_error = true;
                start_millis = millis();
            }
        }
    }

    return write_bytes;
}

int JSN260::send(const char *data, int timeout)
{
    send((uint8_t *)data, strlen(data), timeout);
}

boolean JSN260::ask(const char *q, const char *a, int timeout)
{
    unsigned long start;
    unsigned long end;
    int q_len = strlen(q);
    send((uint8_t *)q, q_len, timeout);

    if (a != NULL) {
        setTimeout(timeout);
        start = millis();
        boolean found = find((char *)a);
        if (!found) {
            end = millis();
            if ((end - start) < timeout) {
                DBG("\r\n");
                DBG(q);
                DBG("\r\nTry to find: ");
                DBG(a);
                DBG("\r\nTimeout: ");
                DBG(timeout);
                DBG("\r\nStart time: ");
                DBG(start);
                DBG("\r\nEnd time: ");
                DBG(end);
                DBG("\r\n***** Probably ot enough memory *****\r\n");
            } else {
                DBG("Timeout! ");
            }

            return false;
        }
    }

    return true;
}

boolean JSN260::sendCommand(const char *cmd, const char *ack, int timeout)
{
    DBG("CMD: ");
    DBG(cmd);
    DBG("\r\n");
    clear();
 
    if (!ask(cmd, ack, timeout)) {
        DBG("Failed to run: ");
        DBG(cmd);
        DBG("\r\n");
        error_count++;
        return false;
    }
    error_count = 0;
    return true;
}

void JSN260::clear()
{
    char r;
    while (receive((uint8_t *)&r, 1, 10) == 1) {
    }
}

 
int JSN260::receive(uint8_t *buf, int len, int timeout)
{
    int read_bytes = 0;
    int ret;
    unsigned long end_millis;

    while (read_bytes < len) {
        end_millis = millis() + timeout;
        do {
            ret = read();
            if (ret >= 0) {
                break;
            }
        } while (millis() < end_millis);

        if (ret < 0) {
            return read_bytes;
        }
        buf[read_bytes] = (char)ret;
        read_bytes++;
    }

    return read_bytes;
}
