#include "p2p_client.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"

extern int g_login_flag;
extern int g_userChn_flag;

int main(int argc, char **argv)
{
	char strJson[512];
	int flag_login = 0, user_flag = 0;
	unsigned int count = 0;

	p2p_client_initial("00023323");
	p2p_client_set_local_addr("192.168.1.113");

	// china server
	//p2p_client_start("120.79.60.230", 6000, "00018775", "inputpwd");
	//p2p_client_start("120.79.60.230", 6000, "00018776", "admin");

	// japan server	
	//p2p_client_start("15.152.48.55", 6000, "000493E9", "admin");
	p2p_client_start("15.152.48.55", 6000, argv[1], "admin");
	//p2p_client_start("15.152.48.55", 6000, "000493FD", "admin");
	
	
	p2p_client_start_live();

	while(1)
	{
		if(g_login_flag)
		{
			if(flag_login)
			{
				count++;
			}

			if(!flag_login)
			{
				flag_login = 1;				
				
				sprintf(strJson, "{\"cmd\":\"%s\",\"cid\":%d,\"channel\":%d}", "get_device_info", 1, 0);
				p2p_client_send_json(strJson);
				
				sleep(1);

				sprintf(strJson, "{\"cmd\":\"%s\",\"cid\":%d,\"channel\":%d}", "get_sensor_value", 1, 0);
				p2p_client_send_json(strJson);

				p2p_client_start_userChannel();
			}
			
			if(flag_login && (count % 3 == 0))
			{
				sprintf(strJson, "{\"cmd\":\"%s\",\"cid\":%d,\"channel\":%d}", "get_sensor_value", 1, 0);
				p2p_client_send_json(strJson);				
			}
			
			if(flag_login && count == 3)
			{
				printf("get_camera_pwd\n");
				
				sprintf(strJson, "{\"cmd\":\"%s\",\"inputpwd\":\"%s\",\"check\":%d}", "get_camera_pwd", "admin", 0);
				p2p_client_send_json(strJson);
			}

			if(g_userChn_flag && !user_flag && count == 20)
			{
				user_flag = 1;
				
				printf("p2p_client_test_userChannel....\n");
				p2p_client_test_userChannel();
			}
		}

		sleep(1);
	}

	p2p_client_stop_userChannel();

	p2p_client_clean();
	return 0;
}
