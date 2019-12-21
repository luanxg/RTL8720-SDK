ALC CODEC FW UPGRADE

Description:
Get file from TFTP server and upgrade the audio dsp firmwatre.

Configuration:
Modify TFTP_HOST_IP_ADDR and ALC_DSP_FIRMWARE_NAME for your firmware in example_alc_dsp_fw_upgrade.c based on your TFTP server.

[platform_opts.h]
	#define CONFIG_EXAMPLE_ALC_DSP_FW_UPGRADE    1

[Supported List]
	Supported :
	    Ameba-1
	Source code not in project:
	    Ameba-z, Ameba-pro