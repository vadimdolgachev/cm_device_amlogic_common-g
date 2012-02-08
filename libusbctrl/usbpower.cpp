/* main.c */  

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <dirent.h>

#define LOG_TAG "USBPower"

#include "cutils/log.h"
#include  "cutils/properties.h"

#define IDX_ATTR_FILENAME	"/sys/devices/platform/usb_phy_control/index"
#define POWER_ATTR_FILENAME	"/sys/devices/platform/usb_phy_control/por"

#define TOLOWER(x) ((x) | 0x20)

#define CONNECT_STR		"Bus Connected = 0x"
#define CONNECT_FILE_NAME	"/sys/devices/lm0/busconnected"

#define GOTGCTL_STR		"GOTGCTL = 0x"
#define GOTGCTL_FILE_NAME	"/sys/devices/lm0/gotgctl"

#define USB_IDX_A	0
#define USB_IDX_B	1
#define USB_IDX_MAX	2

#define USB_CMD_ON	0
#define USB_CMD_OFF	1
#define USB_CMD_IF	2
#define USB_CMD_MAX	3

#define USB_ID	16
#define USB_ID_HOST		0x0
#define USB_ID_DEVICE	0x1

#define USB_SES	18
#define USB_SES_VALID	0x3

char usb_index_str[USB_IDX_MAX][2]=
{
	"A","B"
};

char usb_state_str[USB_CMD_MAX][8]=
{
	"on","off","if"
};


static void usage(void)
{
    printf("usbpower USAGE:\n");
    printf("usbpower <portindex> <cmd> \n");
	printf("        index: A or B ; state: on/off/if(find device exist)\n");
    printf("for example: usbpower A	on\n");
    exit(1);
}

static int get_device_if(int idx)
{
	int ret = 0;
	int err = 0;
	char line[32];
	char filename[32];
	FILE *fp;
	unsigned int busconnect,gotgctl;

	strcpy(filename,GOTGCTL_FILE_NAME);
	filename[15] = idx + '0';

	if((fp = fopen(filename,"r"))) {
		if (fgets(line, 32, fp)) {
			if (!strncmp(line, GOTGCTL_STR, strlen(GOTGCTL_STR))) {
				sscanf(line+strlen(GOTGCTL_STR),"%x",&gotgctl);
			}
			else
			{
				SLOGE("gotgctl txt is error\n");
				err =1;
			}
		} else {
			SLOGE("Failed to read gotgctl\n");
			err=2;
		}

		fclose(fp);
	} else {
		//SLOGW("No usb device\n");
		err=3;
	}
	
	if(err)
		return 0;
	
	strcpy(filename,CONNECT_FILE_NAME);
	filename[15] = idx + '0';

	if((fp = fopen(filename,"r"))) {
		if (fgets(line, 32, fp)) {
			if (!strncmp(line, CONNECT_STR, strlen(CONNECT_STR))) {
				sscanf(line+strlen(CONNECT_STR),"%x",&busconnect);
      } else {
				SLOGE("busconnected txt is error\n");
				err=4;
			}
		} else {
			SLOGE("Failed to read busconnected\n");
			err=5;
		}

		fclose(fp);
	} else {
		//SLOGW("No usb device\n");
		err=6;
	}

	if(err)
		return 0;
		
	if(((gotgctl>>USB_ID)&0x1)==	USB_ID_HOST)
	{
		// this case,check busconnected
		if(busconnect==1)
			ret = 1;
	}
	else
	{
		// this case,check gotgctl USB_SES_VALID
		if(((gotgctl>>USB_SES)&0x3)== USB_SES_VALID)
			ret = 1;
	}
	return ret;
}

static int set_power_ctl(int idx,int cmd)
{
	int ret = 0;
	FILE *fp;

	if(cmd == USB_CMD_OFF)
	{
		ret = get_device_if(idx);
		if(ret == 1)
		{
			printf("Has devices on this port,can't power off!\n");
			return ret;
		}
	}

	if((fp = fopen(IDX_ATTR_FILENAME,"w"))){   
    	fwrite(usb_index_str[idx], 1, strlen(usb_index_str[idx]),fp);
    	fclose(fp);
	}
	else
	{
		ret = -1;
	}

	if(ret == 0)
	{
		if((fp = fopen(POWER_ATTR_FILENAME,"w"))){   
    		fwrite(usb_state_str[cmd], 1, strlen(usb_state_str[cmd]),fp);
    		fclose(fp);
		}
		else
		{
			ret = -2;
		}
	}

	return ret;
}

int main(int argc,  char * argv[])
{   
	int usagedisplay = 1;
	char * idxstr=NULL;
	char * statestr=NULL;
	int index,cmd;
	int ret=0;
	int i;

	if(argc<3)
	{
		goto USAGE;
	}
	else
	{
		idxstr = argv[1];
		statestr = argv[2];
	}

	for(i=0;i<USB_IDX_MAX;i++)
	{
		if(strcmp(idxstr, usb_index_str[i])==0)
		{
			index = i;
			break;
		}
	}

	if(i>=USB_IDX_MAX)
		goto USAGE;

	for(i=0;i<USB_CMD_MAX;i++)
	{
		if(strcmp(statestr, usb_state_str[i]) == 0)
		{
			cmd = i;
			break;
		}
	}

	if(i<USB_CMD_MAX)
		usagedisplay = 0;
USAGE:
	if (usagedisplay) {
        usage();
    }

	if((cmd==USB_CMD_ON)||(cmd==USB_CMD_OFF))
	{
		ret = set_power_ctl(index,cmd);
	}
	else if(cmd == USB_CMD_IF)
	{
		ret = get_device_if(index);
#if 0
		if(ret == 1 )
			printf("Has Device\n");
		else
			printf("No device\n");
#endif
	}

	return ret;
}
