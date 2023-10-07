/*
// LINKS //

	Remember to link to : wsock32.lib

*/

// INCLUDE FILES //

// OS Dependent headers //
#ifdef WIN32
#include <winsock2.h>
#include <string>
#else
#define Sleep usleep
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "XPThreads.h"

const int INVALID_SOCKET = 0;
const int SOCKET_ERROR = -1;
XPThreads* MainThread;
#endif

// Normal headers //
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <list>

// SDK Headers //
#include "../SDK/plugin.h"


// DEFINITIONS //
#define	MAX_CLIENTS 200	// Define the maximum number of clients we can receive
#define	BUFFER_SIZE 512 // Define the buffer size of the messages

// STRUCTURES //
struct _client
{
	bool		connected;
	sockaddr_in	address;
#ifdef WIN32
	SOCKET		socket;
#else
	int			socket;
#endif
	fd_set		socket_data;
#ifdef WIN32
	int			address_length;
#else
	socklen_t	address_length;
#endif
	int			id;
	char		template_name[15];
	char		siegepos[45];
};

// GLOBAL VARIABLES //
int PORT;
bool serv_started=false;
bool looping=false;
sockaddr_in	server_address;
sockaddr	server_socket_address;
#ifdef WIN32
SOCKET		server_socket;
#else
int			server_socket;
#endif
_client		client[MAX_CLIENTS];
int			clients_connected = 0;
void *vThreadHandle, *vThreadHandle2;
void **ppPluginData;
extern void *pAMXFunctions;
char* CMid(char *string,int start, int end);
AMX *pAMX;


// FUNCTION DECLARATIONS //

bool	accept_client ( _client *current_client );
int		disconnect_client ( _client *current_client );
//void	echo_message ( char *message );
void	end_server();
void	midcopy ( char* input, char* output, int start_pos, int stop_pos );
int		receive_client ( _client *current_client, char *buffer, int size );
void	receive_data();
void	start_server();
int		PSocket_Disconnect(int conn, char ip[]);
int		PSocket_Receive(int conn, char data[]);
int		PSocket_Connect(int conn, char ip[]);
int		accept_connections ();
void	start_server(int s_port);
int		send_data ( _client *current_client, char *buffer, int size );
int		disconnect_client ( _client *current_client );
int		strpos ( char *haystack, char *needle );

#if defined WIN32
int mainfunc();
#else
void* mainfunc(void* params);
#endif

// FUNCTION DEFINITIONS //

typedef void (*logprintf_t)(char* format, ...);

logprintf_t logprintf;


// SERVER FUNCTIONS ##############################################################################################################

// Data Recieve ##################################################################################################################
int receive_client ( _client *current_client, char buffer[], int size )
{
	if ( FD_ISSET ( current_client->socket, &current_client->socket_data ) )
	{
		// Store the return data of what we have sent
		current_client->address_length = recv ( current_client->socket, buffer, size, 0 );

		if ( current_client->address_length == 0 )
		{ 
			// Data error on client
			disconnect_client ( current_client );
			return ( false );
		}
		return ( true );
	}
	return ( false );
}

void receive_data()
{
	char buffer[BUFFER_SIZE];
	for ( int j = 0; j < MAX_CLIENTS; j++ )
	{
		if (client[j].connected)
		{
			int result;
			result = receive_client ( &client[j], buffer, BUFFER_SIZE);
			if (result)
			{
				if (strlen(buffer) != 0)
				{
					int result = strpos(buffer, "\r\n");
					if (result != -1) 
					{
						char* buffer3 = strtok(buffer,"\r\n");

						buffer[strlen(buffer3)] = '\0';
						//logprintf("data: %s",buffer);

						PSocket_Receive(j,buffer);
					}
				}
			}
			
		}
		buffer[0] = '\0';
	}
}

// Main Process ##################################################################################################################
#if defined WIN32
int mainfunc()
#else
void* mainfunc(void* params)
#endif
{
	logprintf("[PSocket] PSocket listening on port %d.",PORT);
	if (serv_started == false) 
#ifdef WIN32
		return -1;
#else
		return (void*)-1;
#endif

	// Loop forever
	looping = true;

	while ( looping )
	{
		accept_connections();

		receive_data();

		// Sleep so the server dont eat cpu/memory
		#ifdef WIN32
			Sleep(250);
		#else
			usleep(250000); //Linux uses usleep, units are microseconds. (1000000 = 1 second)
		#endif

	}
#ifdef WIN32
	CloseHandle(vThreadHandle);
#else
	delete MainThread;
#endif
	return 0;

}

// Start Server ##################################################################################################################
void start_server(int s_port)
{ 
	int res;
	PORT=s_port;

#ifdef WIN32
	WSADATA wsaData;
#endif
	int i = 1;

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons ( PORT);

	memcpy ( &server_socket_address, &server_address, sizeof ( sockaddr_in ) );

#ifdef WIN32
	res = WSAStartup ( MAKEWORD ( 1, 1 ), &wsaData );
	if ( res != 0 ) 
	{
		//cout << "WSADATA ERROR : Error while attempting to initialize winsock." << endl;
	}
#endif

	server_socket = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if ( server_socket == INVALID_SOCKET )
	{
		logprintf("[PSocket] Invalid Socket.");
	}
	else if ( server_socket == SOCKET_ERROR )
	{
		logprintf("[PSocket] Socket Error.");
	}
	else
	{
		logprintf("[PSocket] Socket Established.");
	}
	setsockopt ( server_socket, SOL_SOCKET, SO_REUSEADDR, ( char * ) &i, sizeof ( i ) );
	res = bind ( server_socket, &server_socket_address, sizeof ( server_socket_address ) );
	res = listen ( server_socket, 8 );
	logprintf("[PSocket] Starting on port %d... ",s_port);
#ifdef WIN32
	unsigned long b = 1;
	ioctlsocket ( server_socket, FIONBIO, &b );
#else
	int oldfdArgs = fcntl(server_socket, F_GETFL, 0);
	fcntl(server_socket, F_SETFL, oldfdArgs | O_NONBLOCK);
#endif
	clients_connected=0;
	for ( int j = 0; j < MAX_CLIENTS; j++ ) 
	{ 
		client[j].id = -1;
		client[j].connected = false;
	}
	serv_started = true;
	int threadId = 0;
#ifdef WIN32
	vThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&mainfunc, 0, 0, (LPDWORD)&threadId);
#else
	MainThread = new XPThreads(mainfunc);
	MainThread->Run();
#endif
}


// Stop Server ###################################################################################################################
void end_server()
{
	for ( int j = 0; j < MAX_CLIENTS, j++;) 
	{
		disconnect_client ( &client[j] ); 
	}
	looping = false;
#ifdef WIN32
	closesocket ( server_socket );
#else
	close( server_socket );
#endif

#ifdef WIN32
	WSACleanup();
#endif
	serv_started = false;
	logprintf("[PSocket] Server Shutdown.");
}

// Disconnect Function ###########################################################################################################
int disconnect_client ( _client *current_client )
{
	if ( current_client->connected == true )
	{
#ifdef WIN32
		closesocket ( current_client->socket );
#else
		close( current_client->socket );
#endif
	}
	current_client->connected = false;
	current_client->address_length = -1;
	clients_connected--;
	char *client_ip_address = inet_ntoa ( current_client->address.sin_addr );
	int id = current_client->id;
	PSocket_Disconnect(id,client_ip_address);
	current_client->id = -1;
	//logprintf("[PSocket] Client from %s has disconnected.",client_ip_address);
	return ( 1 );
}

int fdisconnect_client ( _client *current_client )
{
	if ( current_client->connected == true )
	{ 
#ifdef WIN32
		closesocket ( current_client->socket );
#else
		close( current_client->socket );
#endif
	}
	current_client->connected = false;
	current_client->address_length = -1;
	clients_connected--;
	char *client_ip_address = inet_ntoa ( current_client->address.sin_addr );
	int id = current_client->id;
	PSocket_Disconnect(id,client_ip_address);
	current_client->id = -1;
	//logprintf("[PSocket] Client from %s has been disconnected from server.",client_ip_address);
	return ( 1 );
}

// Send Data to client ###########################################################################################################
int send_data ( _client *current_client, char *buffer, int size )
{
	char *newLine = "\r\n";
	char *retVal = new char[strlen(buffer)+strlen(newLine)+1];
	strcat(retVal,buffer);
	strcat(retVal,newLine);

	current_client->address_length = send ( current_client->socket, retVal, size, 0 );
	buffer[0]='\0';
	if ( ( current_client->address_length == SOCKET_ERROR ) || ( current_client->address_length == 0 ) )
	{
		disconnect_client ( current_client );
		return ( false );
	}
	else
		return ( true );
}


// Actually connecting the client ################################################################################################
bool accept_client ( _client *current_client )
{
	current_client->address_length = sizeof ( sockaddr );
	current_client->socket = accept ( server_socket, ( sockaddr * ) &current_client->address, &current_client->address_length );
#ifdef WIN32
	unsigned long b = 1;
	ioctlsocket ( current_client->socket, FIONBIO, &b );
#else
	int oldfdArgs = fcntl(current_client->socket, F_GETFL, 0);
	fcntl(current_client->socket, F_SETFL, oldfdArgs | O_NONBLOCK);
#endif
	if ( current_client->socket == 0 )
	{
		return ( false );
	}
	else if ( current_client->socket == SOCKET_ERROR )
	{
		return ( false );
	}
	else
	{
		current_client->connected = true;
		FD_ZERO ( &current_client->socket_data );
		FD_SET ( current_client->socket, &current_client->socket_data );
		return ( true );
	}

	return ( false );
}

// Once they connect #############################################################################################################
int accept_connections()
{
	if ( clients_connected < MAX_CLIENTS )
	{
		for ( int j = 0; j < MAX_CLIENTS; j++ )
		{
			if ( !client[j].connected )
			{
				if ( accept_client ( &client[j] ) )
				{
					clients_connected++;
					client[j].id = j;
					char *client_ip_address = inet_ntoa ( client[j].address.sin_addr );
					PSocket_Connect(j,client_ip_address);
					//logprintf("[PSocket] Client from %s has connected.",client_ip_address);
				}
			}
		}
	}
	return 1;
}

// SA-MP CALLBACKS ###############################################################################################################

// When a data is sent to server. ################################################################################################
int PSocket_Receive(int id, char *data)
{
    int idx; cell ret, amx_Address,*phys_addr;
	if (strlen(data) == 0)
        return 0;
	int amxerr = amx_FindPublic(pAMX, "PSocket_OnReceiveData", &idx);
    if (amxerr == AMX_ERR_NONE)
	{
		amx_PushString(pAMX,&amx_Address,&phys_addr,data,0,0);
		amx_Push(pAMX, id);
		amx_Exec(pAMX, NULL, idx);
		return 1;
	}
	return 0;
}

// When a connection is made. ####################################################################################################
int PSocket_Connect(int id, char ip[])
{
    int idx; cell ret, amx_Address,*phys_addr; 
	if (strlen(ip) == 0)
        return 0;
	int amxerr = amx_FindPublic(pAMX, "PSocket_OnConnect", &idx);
    if (amxerr == AMX_ERR_NONE)
	{
		amx_PushString(pAMX,&amx_Address,&phys_addr,ip,0,0);
		amx_Push(pAMX, id);
		amx_Exec(pAMX, NULL, idx);
		return 1;
	}
	return 0;
}

// When a connection is lost. ####################################################################################################
int PSocket_Disconnect(int id,char ip[])
{
    int idx; cell ret, amx_Address,*phys_addr;
	if (strlen(ip) == 0)
        return 0;
	int amxerr = amx_FindPublic(pAMX, "PSocket_OnDisconnect", &idx);
    if (amxerr == AMX_ERR_NONE)
	{
		amx_PushString(pAMX,&amx_Address,&phys_addr,ip,0,0);
		amx_Push(pAMX, id);
		amx_Exec(pAMX, NULL, idx);
		return 1;
	}
	return 0;
}

// OTHER FUNCTIONS ###############################################################################################################
void midcopy ( char* input, char* output, int start_pos, int stop_pos )
{
	int index = 0;

	for ( int i = start_pos; i < stop_pos; i++ )
	{
		output[index] = input[i];
		index++;
	}

	output[index] = 0;
}

int strpos(char *haystack, char *needle)
{
   char *p = strstr(haystack, needle);
   if (p)
      return p - haystack;
   return -1;   // Not found = -1.
}

// AMX CALLS #####################################################################################################################
static cell AMX_NATIVE_CALL n_getusercount(AMX *amx, cell *params)
{
	return clients_connected;
}
static cell AMX_NATIVE_CALL n_getip(AMX *amx, cell *params)
{
	cell* addr = NULL;
	if(params[0] != 12)
	{
		logprintf("PSocket_GetIP: Expecting 3 params, but found %d.",params[0] / 12);
		return 0;
	}
	int p;
	p = params[1];
	if (client[p].connected)
	{	
		char *client_ip_address = inet_ntoa ( client[p].address.sin_addr );
		amx_GetAddr(amx, params[2], &addr);
		amx_SetString(addr, client_ip_address, NULL, NULL, params[3]);
	}
	else
		return -2;
	return 1;
}

static cell AMX_NATIVE_CALL n_start_server( AMX* amx, cell* params )
{
	if(params[0] != 4)
	{
		logprintf("PSocket_Start: Expecting 1 param, but found %d",params[0] / 4);
		return 0;
	}
	if(serv_started == true)
		return -1;
	int s_port = params[1];
	start_server(s_port);
	pAMX = amx;
	return 1;
}

static cell AMX_NATIVE_CALL n_end_server( AMX* amx, cell* params )
{
	if(serv_started == false)
		return -1;
	end_server();
	return 1;
}


static cell AMX_NATIVE_CALL n_close( AMX* amx, cell* params )
{
	if(serv_started == false)
		return -1;
	if(params[0] != 4)
	{
		logprintf("PSocket_Close: Expecting 1 param, but found %d",params[0] / 4);
		return 0;
	}
	int p;
	p = params[1];
	if (client[p].connected)
	{
		fdisconnect_client(&client[p]);
	}
	else
		return -2;
	return 1;
}

static cell AMX_NATIVE_CALL n_send_message_all( AMX* amx, cell* params )
{
	if(serv_started == false)
		return -1;
	if(params[0] != 4)
	{
		logprintf("PSocket_SendDataToAll: Expecting 1 param, but found %d",params[0] / 4);
		return 0;
	}
 	char* rawdata_pa = NULL;
 	amx_StrParam(amx, params[1], rawdata_pa);
	if (strlen(rawdata_pa) == 0)
		return 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if (client[i].connected )
		{
			int len_pa = strlen(rawdata_pa);
			send_data(&client[i],rawdata_pa,len_pa);
		}
	}
	return 1;
}

static cell AMX_NATIVE_CALL n_send_message( AMX* amx, cell* params )
{
	if(serv_started == false)
		return -1;
	if(params[0] != 8)
	{
		logprintf("PSocket_SendData: Expecting 2 params, but found %d",params[0] / 8);
		return 0;
	}
	int p;
	p = params[1];
 	char* rawdata_pa = NULL;
	amx_StrParam(amx, params[2], rawdata_pa);
	if (strlen(rawdata_pa) == 0)
		return 0;
	if (client[p].connected )
	{
		int len_pa = strlen(rawdata_pa);
		send_data(&client[p],rawdata_pa,len_pa);
	}
	else
		return -2;
	return 1;
}

static cell AMX_NATIVE_CALL n_connected( AMX* amx, cell* params )
{
	if(serv_started == false)
		return -1;
	if(params[0] != 4)
	{
		logprintf("PSocket_IsConnected: Expecting 1 param, but found %d",params[0] / 4);
		return 0;
	}
	int p = params[1];
	if (client[p].connected)
		return 1;
	return -2;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load( void **ppData )
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
	logprintf( "PSocket 1.1x Plugin loaded." );
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload( )
{
	end_server();
	logprintf( "PSocket 1.0 Plugin unloaded." );
}

AMX_NATIVE_INFO MyProjectNatives[ ] =
{
	{"PSocket_Start",			n_start_server},
	{"PSocket_Stop",			n_end_server},
	{"PSocket_SendData",		n_send_message},
	{"PSocket_SendDataToAll",	n_send_message_all},
	{"PSocket_IsConnected",		n_connected},
	{"PSocket_Close",			n_close},
	{"PSocket_GetIP",			n_getip},
	{"PSocket_GetUserCount",	n_getusercount},
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad( AMX *amx )
{
	return amx_Register( amx, MyProjectNatives, -1 );
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload( AMX *amx )
{
	return AMX_ERR_NONE;
}
