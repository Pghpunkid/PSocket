/*
	PSocket 1.0 by Pghpunkid (c) 2011
	Include File
	GNU Public License
*/

//Defines
#define MAX_SOCKETS	200				//This value is hardcoded into the plugin, this is for use with variables.

//Errors
#define ERROR_NOTCONNECTED -2		//User is not connected.
#define ERROR_STARTSTOP_DONE -1		//Server is already Started/Stopped depending on the function.
#define ERROR_MISS_PARAMETER 0		//Missing a parameter.
#define ERROR_NONE 1				//Function completed with no errors. (Start, Stop, SendData, SendDataToAll, Close)

//Callbacks
forward PSocket_OnConnect(id,ip[]);
forward PSocket_OnDisconnect(id,ip[]);
forward PSocket_OnReceiveData(id,data[]);

//Functions
native PSocket_Start(port);
native PSocket_Stop();
native PSocket_SendData(id, data[]);	//id = id given on connect/disconnect
native PSocket_SendDataToAll(data[]);	
native PSocket_IsConnected(id);			//id = id given on connect/disconnect
native PSocket_Close(id);				//id = id given on connect/disconnect
native PSocket_GetIP(id,ip[],len);		//id = id given on connect/disconnect
native PSocket_GetUserCount();
