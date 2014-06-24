#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "ThinkGearStreamParser.h"

#define PIN 0000

int prevValue = -1;
char command[50];

/* Callback Mindwave bluetooth parser*/
void handleDataValueFunc( unsigned char extendedCodeLevel,unsigned char code,unsigned char valueLength,const unsigned char *value,void *customData ) {
	int i,newValue;
	if( extendedCodeLevel == 0 ) {
		switch( code ) {
			case( PARSER_CODE_ATTENTION ):
				newValue = value[0] & 0xFF;
				printf( "Attention Level: %d\n", value[0] & 0xFF );
				if(newValue != prevValue){
					sprintf (command, "pactl set-sink-volume 1 %d%%",newValue);
					system(command);
				}
				prevValue = value[0] & 0xFF;
				break;
			case( PARSER_CODE_MEDITATION ):
				printf( "Meditation Level: %d\n", value[0] & 0xFF );
				break;
			default:
				printf( "EXCODE level: %d CODE: 0x%02X vLength: %d\n",
				extendedCodeLevel, code, valueLength );
				printf( "Data value(s):" );
				for( i=0; i<valueLength; i++ )
					printf( " %02X", value[i] & 0xFF );
				printf( "\n" );
		}
	}
}

int main(int argc, char **argv){
	inquiry_info *ii = NULL;
	int max_rsp, num_rsp;
	int dev_id, sock, len, flags;
	int i,pos;
	char addr[19] = { 0 };
	char **addrs;
	char name[248] = { 0 };
	struct sockaddr_rc addr_sc = { 0 };
	int s, status;
	char dest[18] = "01:23:45:67:89:AB";
	unsigned char *rcv;
	int bytes_rcv;
	int parserInited = 1;
	ThinkGearStreamParser parser;
	
	parserInited = THINKGEAR_initParser( &parser, PARSER_TYPE_PACKETS,handleDataValueFunc, NULL );
	
	dev_id = hci_get_route(NULL);
	sock = hci_open_dev( dev_id );
	if (dev_id < 0 || sock < 0) {
		perror("opening socket");
		exit(1);
	}
	
	len  = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
	
	num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
	if( num_rsp <= 0 ){
		perror("hci_inquiry");
		exit(0);
	}
	
	addrs = (char**)malloc(num_rsp*sizeof(char*));
	
	if(addrs==NULL){
		perror("malloc");
		exit(2);
	}
	
	for (i = 0; i < num_rsp; i++) {
		ba2str(&(ii+i)->bdaddr, addr);
		memset(name, 0, sizeof(name));
		if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
			strcpy(name, "[unknown]");
		addrs[i] = (char*)malloc(19*sizeof(char));
		memcpy(addrs[i], addr, 19);
		printf("%d) %s  %s\n", i, addrs[i], name);
	}
	
	printf("Select device: ");
	scanf("%d",&pos);
	if( pos >= num_rsp || pos < 0){
		printf("not available\n");
		return 0;
	}
	
	printf("%s selected\n",addrs[pos]);
	strcpy(dest,addrs[pos]);
	free( ii );
	for(i = 0; i < num_rsp; i++)
		free(addrs[i]);
	free(addrs);
	close( sock );
	
	s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	
	addr_sc.rc_family = AF_BLUETOOTH;
	addr_sc.rc_channel = (uint8_t) 1;
	str2ba( dest, &addr_sc.rc_bdaddr );
	
	status = connect(s, (struct sockaddr *)&addr_sc, sizeof(addr_sc));
	printf("%d status\n",status);
	rcv = (unsigned char*)malloc(1024*sizeof(unsigned char));
	if( status == 0 ) {
		printf("Waiting for data...\n");
		while( bytes_rcv = read(s, rcv, sizeof(rcv))) {
			if(bytes_rcv > 0){
				for(i=0; i<bytes_rcv; ++i){
					THINKGEAR_parseByte(&parser, rcv[i]);
				}
			}
		}
	}

	if( status < 0 )
		perror("info:");
	free(rcv);
	close(s);
	return 0;
}
