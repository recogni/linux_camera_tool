
#include "../includes/shortcuts.h"

#include "../src/uvc_extension_unit_ctrl.h"
#include "../src/extend_cam_ctrl.h"
#include "../src/ui_control.h"
#include "../src/cam_property.h"
#include "../src/v4l2_devices.h"

int v4l2_dev; /* global variable, file descriptor for camera device */
int fw_rev; /* global variable, firmware revision for the camera */

/* main function */
int main(int argc, char ** argv)
{
	struct device dev;
	char dev_name[64] = "/dev/video0";
	char *ret_dev_name = enum_v4l2_device(dev_name);
	v4l2_dev = open_v4l2_device(ret_dev_name, &dev);

	if (v4l2_dev < 0)
	{
		printf("open camera %s failed,err code:%d\n\r", dev_name, v4l2_dev);
		return 0;
	}

	system("v4l2-ctl --list-formats-ext | grep Size | awk '{print $1 $3}'");
	/* run a v4l2-ctl --list-formats-ext to see the resolution */
	//video_set_format(&dev, 1344, 972, V4L2_PIX_FMT_YUYV);
	//set_frame_rate(v4l2_dev, 15);
	
	get_frame_rate(v4l2_dev);
	
	/* try to get all the static camera info before fork */
	fw_rev = read_cam_uuid_hwfw_rev(v4l2_dev);
	check_dev_cap(&dev);
	video_get_format(&dev);
	video_alloc_buffers(&dev, 1);
	
	//sensor_reg_read(v4l2_dev, 0x55d7);
	//generic_I2C_read(v4l2_dev, 0x02, 8, 0x20, 0x0210);

	


	/* Activate streaming */
	start_camera(&dev);
	pid_t pid;
	pid = fork();
	if (pid == 0) {				/* child process */
		init_control_gui(argc, argv);
	} else if (pid > 0) { 		/* parent process */
		streaming_loop(&dev);
		/* Deactivate streaming */
		stop_Camera(&dev);
		video_free_buffers(&dev);
	} else {
		fprintf(stderr, "ERROR:fork() failed\n");
	}
	

/* individual camera tests, detail info is in uvc_extension_unit_ctrl.h */
#ifdef AP0202_WRITE_REG_ON_THE_FLY
	unsigned int i;
	for (i = 0; i < sizeof(ChangConfig) / sizeof(reg_seq); i++)
		generic_I2C_write(v4l2_dev, 0x82, ChangConfig[i].reg_data_width,
						  AP020X_I2C_ADDR, ChangConfig[i].reg_addr,
						  (unsigned char *)&(ChangConfig[i].reg_val));

	for (i = 0; i < sizeof(ChangConfig) / sizeof(reg_seq); i++)
	{
		generic_I2C_read(v4l2_dev, 0x02, ChangConfig[i].reg_data_width,
						 AP020X_I2C_ADDR, ChangConfig[i].reg_addr);
	}

	generic_I2C_read(v4l2_dev, 0x02, 1, MAX9295_SER_I2C_ADDR, 0x0000);
	generic_I2C_read(v4l2_dev, 0x02, 1, MAX9296_DESER_I2C_ADDR, 0x0000);
#endif

#ifdef AP0202_WRITE_REG_IN_FLASH
	load_register_setting_from_configuration(v4l2_dev,
		SIZE(ChangConfigFromFlash), ChangConfigFromFlash);

	sleep(1);
	//generic_I2C_read(v4l2_dev, 0x02, 2, AP020X_I2C_ADDR, 0x0058);
	sensor_reg_write(v4l2_dev, 0x5080, 0x00);
	sensor_reg_read(v4l2_dev, 0x4308);
	sensor_reg_read(v4l2_dev, 0x5080);
#endif

#ifdef OS05A20_PTS_QUERY
	set_pts(v4l2_dev, 0);
	get_pts(v4l2_dev);
#endif

#ifdef AR0231_MIPI_TESTING
	unsigned int i;
	for (i = 0; i < sizeof(AR0231_MIPI_REG_TESTING) / sizeof(reg_seq); i++)
	{
		//choose either one of the function below for register read
		generic_I2C_read(v4l2_dev, 0x02, AR0231_MIPI_REG_TESTING[i].reg_data_width,
						 AR0231_I2C_ADDR, AR0231_MIPI_REG_TESTING[i].reg_addr);

		sensor_reg_read(v4l2_dev, AR0231_MIPI_REG_TESTING[i].reg_addr);
	}
#endif

#ifdef IMX334_MONO_MIPI_TESTING
	unsigned int i;
	for (i = 0; i < sizeof(IMX334_MIPI_REG_TESTING) / sizeof(reg_seq); i++)
	{
		//TODO: choose either one of the function below for register read
		generic_I2C_read(v4l2_dev, 0x02, IMX334_MIPI_REG_TESTING[i].reg_data_width,
						 IMX334_I2C_ADDR, IMX334_MIPI_REG_TESTING[i].reg_addr);

		sensor_reg_read(v4l2_dev, IMX334_MIPI_REG_TESTING[i].reg_addr);
	}
	set_gain(v4l2_dev, 5);
	get_gain(v4l2_dev);
#endif
	
	
	close(v4l2_dev);
	return 0;
}

