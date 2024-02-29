#include "p2p_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include "P2PClic.h"
#include "camera_protocol.h"

#define USE_DEBUG_AV   0

#define CMDBUFFERSIZE (5 * 1024)

int g_login_flag = 0;
int g_userChn_flag = 0;

FILE *fp1;

int OnClientCreate(int nIndex, int result)
{
	printf("OnClientCreate %d-%d\n", nIndex, result);
	return 0;
}

int OnClientDelete(int nIndex, int result)
{
	printf("OnClientDelete %d-%d\n", nIndex, result);
	return 0;
}

int OnClientOpen(int nIndex, int status, unsigned int *m_nRelayServerID, int nRelayCount)
{
	if(status == 1)
	{
		printf("OnClientOpen %d-%s-%08x-%08x\n", nIndex, "Punch", m_nRelayServerID[0], m_nRelayServerID[1]);
	}
	else
	{
		printf("OnClientOpen %d-%s-%08x-%08x\n", nIndex, "Relay", m_nRelayServerID[0], m_nRelayServerID[1]);
	}
	return 0;
}

int OnClientClose(int nIndex, int status)
{
	if(status == 1)
	{
		printf("OnClientClose %d-%s\n", nIndex, "Punch");
	}
	else
	{
		printf("OnClientClose %d-%s\n", nIndex, "Relay");
	}
	return 0;
}


int OnClientLogin(int nIndex, int result, CAPABILITYINFO* capInfo)
{
	CAPABILITYCONTENT* content = (CAPABILITYCONTENT*)(capInfo + 1);
	if(result == 0)
	{
		g_login_flag = 1;
		printf("Client %d Login Success\n");
	}
	else
	{
		g_login_flag = 0;
		printf("Client %d Login Failed\n", nIndex);
	}
	return 0;
}

int OnClientSessionOpen(int nIndex, unsigned int nSessionID, int result)
{
	if (result != 0)
	{
		printf("OnClientSessionOpen failed %d-%d\n", nIndex, nSessionID);
		return 0;
	}
	printf("OnClientSessionOpen %d-%d\n", nIndex, nSessionID);

	if(nSessionID == SESSIONTYPE_USERDATA && result == 0)
	{
		printf("\n------>>>>>>user channel open success.\n\n");
		g_userChn_flag = 1;
	}

	return 0;
}

int OnClientSessionClose(int nIndex, unsigned int nSessionID)
{
	printf("OnClientSessionClose %d-%d\n", nIndex, nSessionID);
	return 0;
}

int OnClientSessionCreate(int nIndex, unsigned int nSessionID, int result)
{
	printf("OnClientSessionCreate %d-%d-%d\n", nIndex, nSessionID, result);

	if(nSessionID == SESSIONTYPE_USERDATA && result == 0)
	{
		printf("user channel create success.\n");
	}

	return 0;
}

int OnClientSessionDelete(int nIndex, unsigned int nSessionID, int result)
{
	printf("OnClientSessionDelete %d-%d-%d\n", nIndex, nSessionID, result);

	if(nSessionID == SESSIONTYPE_USERDATA && result == 0)
	{
		printf("user channel create delte.\n");
		g_userChn_flag = 0;
	}

	return 0;
}

int OnClientSessionData(int nIndex, unsigned int nSessionID, char* data, int len)
{
	switch(nSessionID)
	{
	case SESSIONTYPE_CMD:
		{
			P2PCMDHEADER* header = (P2PCMDHEADER*)data;
			USERCAMERAINFORSP *userCameraInfo;
			if(header->cmdID == UCMDQUERYCAMERAINFORSP)
			{
				userCameraInfo = (USERCAMERAINFORSP*)(data + sizeof(P2PCMDHEADER));
			
				printf("%s-%s-%s\n", userCameraInfo->szManufacture, 
					userCameraInfo->szFirmVersion, userCameraInfo->szDeviceType);

			}
			else if(header->cmdID == NETRSP_JSON)
			{
				char* szJsonData = (char*)(data + sizeof(P2PCMDHEADER));
				printf("recv json : %s\n", szJsonData);

				// analysis recvied json data here !!!
			}
		}
		break;
	case SESSIONTYPE_LIVE:
		{
			
			P2PFRAMEHEADER* header = (P2PFRAMEHEADER*)data;
			if(header->frameType == P2P_FRAMETYPE_VIDEO)
			{
				int nHeadLen = sizeof(P2PFRAMEHEADER) + sizeof(P2PVIDEOFRAME);
				char* frame = data + nHeadLen;
			#if USE_DEBUG_AV
				printf("OnClientVideoData %d-%d-%d\n", nIndex, frame[4] & 0x1F, len - nHeadLen);
			#endif
			}
			else
			{
				int nHeadLen = sizeof(P2PFRAMEHEADER) + sizeof(P2PAUDIOFRAME);
				char* frame = data + nHeadLen;
			#if USE_DEBUG_AV
				printf("OnClientAudioData %d-%d\n", nIndex, len - nHeadLen);
			#endif
			}

		}
		break;
	case SESSIONTYPE_AUDIO:
		{
			P2PFRAMEHEADER* header = (P2PFRAMEHEADER*)data;
			if(header->frameType == P2P_FRAMETYPE_AUDIO)
			{
				int nHeadLen = sizeof(P2PFRAMEHEADER) + sizeof(P2PAUDIOFRAME);
				char* frame = data + nHeadLen;
				printf("OnClientAudioData %d-%d\n", nIndex, len - nHeadLen);
			}
		}
		break;

	case SESSIONTYPE_USERDATA:
		{	
			P2PCMDHEADER * header = (P2PCMDHEADER *)data;			
			UserData_t * p_user_data = (UserData_t *)(data + sizeof(P2PCMDHEADER));
			//printf("get user data : %s\n", (char *)(data + sizeof(P2PCMDHEADER)));

			printf("SESSIONTYPE_USERDATA, cmdID:%d, userData:%d\n", header->cmdID, header->userData);

			if(p_user_data->data_type == P2P_USER_DATA_STRING || p_user_data->data_type == P2P_USER_DATA_RADAR)
			{
				fp1 = fopen("breathing.txt", "w");
				char *breathing = (char *)malloc(sizeof(char)*p_user_data->user_data_len);
				
				for (int i=0; i<p_user_data->user_data_len; i++)
					breathing[i] = p_user_data->user_data[i];
				
				for (int i=0; i<p_user_data->user_data_len; i++)
					fprintf(fp1, "%c", breathing[i]);
				printf("get user radar sensor, len:%d, %s\n", p_user_data->user_data_len, breathing);

				free(breathing);
				fclose(fp1);
				//printf("get user string, len:%d, %s\n", p_user_data->user_data_len, p_user_data->user_data);
			}
			else   // raw data
			{
				printf("get user raw data, len:%d : type:%d, sub type:%d, %s\n", p_user_data->user_data_len, p_user_data->data_type, p_user_data->sub_type, &(p_user_data->user_data[20 * 1000 - 5]));	
				SK_IRCAM_getBodyTemp(p_user_data->user_data, p_user_data->user_data_len);
			}
			
			

		}
		break;
	}


	return 0;
}

int OnClientReleaseData(HDATA hData, int nUserData, int result)
{
	
	return 0;
}

void *p2p_memory_alloc(void* context, char* szTag, int nSize)
{
	static int m_nSize = 0;
	m_nSize += nSize;
	printf("p2p_memory_alloc %s-%d\n", szTag, nSize);
	return malloc(nSize);
}

int p2p_client_initial(char* szDeviceID)
{
	P2PInitial(p2p_memory_alloc, 0);
	
	CLIMEMORYPARAM cliMemoryParam;
	CLICLIENTMEMORYPARAM cliClientMemoryParam;
	UDPMEMORYPARAM cliUdpMemoryParam;

	cliMemoryParam.m_nRequestBufferSize = CMDBUFFERSIZE;
	cliMemoryParam.m_nCMDBufferSize = CMDBUFFERSIZE;
	cliMemoryParam.m_nSpeakBufferSize = CMDBUFFERSIZE;
	cliMemoryParam.m_nUserBufferSize = CMDBUFFERSIZE;

	cliClientMemoryParam.nMaxClientCount = 10;

	cliClientMemoryParam.nCMDSessionCount = 2;
	cliClientMemoryParam.nCMDGroupSize = 1;
	cliClientMemoryParam.nCMDReadBufferSize = 100;
	cliClientMemoryParam.nCMDSendBufferSize = 100;

	cliClientMemoryParam.nAVSessionCount = 2;
	cliClientMemoryParam.nAVGroupSize = 100;
	cliClientMemoryParam.nAVReadBufferSize = 1024;
	cliClientMemoryParam.nAudioGroupSize = 1;
	cliClientMemoryParam.nAudioReadBufferSize = 100;

	cliClientMemoryParam.nSpeakSessionCount = 2;
	cliClientMemoryParam.nSpeakSendBufferSize = 40;

	cliClientMemoryParam.nUserSessionCount = 2;
	cliClientMemoryParam.nUserSendBufferSize = 40;
	cliClientMemoryParam.nUserReadBufferSize = 40;
	cliClientMemoryParam.nUserGroupSize = 40;

	cliUdpMemoryParam.m_nUDPSendBufferSize = 20 * 1024;
	cliUdpMemoryParam.m_nUDPReadBufferSize = 20 * 1024;
	cliUdpMemoryParam.m_nUDPPacketSize = 2 * 1024;

	P2PCliSetMemory(MEMORYTYPE_CLI, (char*)&cliMemoryParam);
	P2PCliSetMemory(MEMORYTYPE_CLI_SESSION, (char*)&cliClientMemoryParam);
	P2PCliSetMemory(MEMORYTYPE_CLI_UDP, (char*)&cliUdpMemoryParam);

	CP2PCliCallback p2pCallback;
	p2pCallback.m_clientCreateCB = OnClientCreate;
	p2pCallback.m_clientDeleteCB = OnClientDelete;
	p2pCallback.m_clientOpenCB = OnClientOpen;
	p2pCallback.m_clientCloseCB = OnClientClose;
	p2pCallback.m_clientLoginCB = OnClientLogin;
	p2pCallback.m_sessionCreateCB = OnClientSessionCreate;
	p2pCallback.m_sessionDeleteCB = OnClientSessionDelete;
	p2pCallback.m_sessionOpenCB = OnClientSessionOpen;
	p2pCallback.m_sessionCloseCB = OnClientSessionClose;
	p2pCallback.m_sessionDataCB = OnClientSessionData;
	p2pCallback.m_releaseDataCB = OnClientReleaseData;
	P2PCliStart(szDeviceID, 0, 1024 * 1024, 0, &p2pCallback);
	return 0;
}

int p2p_client_clean()
{
	
	return 0;
}

int p2p_client_set_local_addr(char* szLocalIP)
{
	P2PCliSetLocalAddr((char*)szLocalIP, 6800);
	return 0;
}

int p2p_client_start(char* szServerIP, int nServerPort, char* szRemoteDeviceID, char* szRemoteDevicePassword)
{
	P2PNETADDRESS netRemoteAddr;
	memset(&netRemoteAddr, 0, sizeof(P2PNETADDRESS));
	P2PNETADDRESS netServerAddr;
	netServerAddr.family = P2P_NET_FAMILY_IPV4;
	strcpy(netServerAddr.ip, szServerIP);
	netServerAddr.port = nServerPort;
	printf("p2p_client_start %s-%d-%s-%s\n", szServerIP, nServerPort, szRemoteDeviceID, szRemoteDevicePassword);
	P2PCliCreateClient(0, &netRemoteAddr, &netServerAddr, 0, "", szRemoteDeviceID, "admin", szRemoteDevicePassword, 6000, 6000, 0);
	return 0;
}

int p2p_client_stop()
{
	P2PCliDeleteClient(0);
	return 0;
}

int p2p_client_start_live()
{
	P2PCliCreateSession(0, SESSIONTYPE_LIVE, MEMORY_MODE_PASSIVE, 0, 0);
	return 0;
}

int p2p_client_stop_live()
{
	P2PCliDeleteSession(0, SESSIONTYPE_LIVE);
	return 0;
}

int p2p_client_start_userChannel(void)
{
	P2PCliCreateSession(0, SESSIONTYPE_USERDATA, MEMORY_MODE_PASSIVE, 0, 0);
	return 0;
}

int p2p_client_stop_userChannel(void)
{
	P2PCliDeleteSession(0, SESSIONTYPE_USERDATA);
	return 0;
}

int p2p_client_get_device_info()
{
	P2PCMDHEADER header;
	header.cmdID = UCMDQUERYCAMERAINFOREQ;
	header.userData = 100;
	P2PCliClientSend(0, SESSIONTYPE_CMD, (char*)&header, sizeof(P2PCMDHEADER), 0, 0);
	return 0;
}

int p2p_client_send_json(char* szJsonData)
{
	P2PCMDHEADER header;
	header.cmdID = NETREQ_JSON;
	header.userData = 100;
	P2PCliClientSend(0, SESSIONTYPE_CMD, (char*)&header, sizeof(P2PCMDHEADER), (char*)szJsonData, strlen(szJsonData) + 1);
	return 0;
}

int p2p_client_test_userChannel(void)
{
	int res, len;

	char m_buffer[1024];
	sprintf(m_buffer, "{\"type\":\"start_speak\","
						"\"user_name\":\"%s\","
						"\"user_pass\":\"%s\"}", "admin", "admin");

	len = strlen(m_buffer) + 1;

	char* dataNew;
#ifdef USEGLOBALMEMORY
    dataNew = (char*)AllocUserDataBuffer(len);
#else
    dataNew = (char*)malloc(len);
#endif
    if(dataNew == 0)
    {
        printf("failed to alloc dataNew!\n");
        return 0;
    }
    memcpy(dataNew, m_buffer, len);

	printf("user data send, %s, %d\n", __FUNCTION__, __LINE__);


	P2PCMDHEADER header;
	header.cmdID = NETREQ_JSON;
	header.userData = 100;
/*
int P2PCliSessionSend(int nIndex,
					  unsigned int nSessionID,
					  HDATA hData,
					  int nUserData, 
					  char* header,
					  int nHeaderLen,
					  char* data,
					  int nDataLen);
*/
	res = P2PCliSessionSend(0, SESSIONTYPE_USERDATA, m_buffer, 100,(char*)&header, sizeof(P2PCMDHEADER),
							(char*)m_buffer, strlen(m_buffer) + 1);  
	if(res <= 0)
	{
#ifdef USEGLOBALMEMORY
		ReleaseUserDataBuffer(dataNew);
#else
		free(dataNew);
#endif
		return -1;
	}	

	return 0;			
}

