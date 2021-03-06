/*
 **  V4L2 video capture example
 **
 **  This program can be used and distributed without restrictions.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>		// getopt_long()

#include <fcntl.h>		// for open()
#include <unistd.h>     // for close()
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <asm/types.h>		// for videodev2.h

#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <media/davinci/dm365_ccdc.h>
#include <media/davinci/vpfe_capture.h>
#include <media/davinci/imp_previewer.h>
#include <media/davinci/imp_resizer.h>
#include <media/davinci/dm365_ipipe.h>

#include <video/davincifb_ioctl.h>
#include <video/davinci_osd.h>

#define ROTATE 	0
#define FBDEV	0

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define CAPTURE_DEVICE	"/dev/video0"
#define VID0_DEVICE "/dev/video2"
#define VID1_DEVICE	"/dev/video3"
#define OSD0_DEVICE	"/dev/fb0"
#define OSD1_DEVICE	"/dev/fb2"
#define FBVID0_DEVICE		"/dev/fb1"
#define FBVID1_DEVICE		"/dev/fb3"

/* Function error codes */
#define SUCCESS			0
#define FAILURE			-1
/* Bits per pixel for video window */
#define YUV_422_BPP		16
#define BITMAP_BPP_8		8

#define round_32(width)		((((width) + 31) / 32) * 32 )

/* D1 screen dimensions */
#define VID0_WIDTH		384
#define VID0_HEIGHT		384
#define VID0_BPP		16
#define VID0_FRAME_SIZE		(VID0_WIDTH*VID0_HEIGHT)
#define VID0_VMODE		FB_VMODE_INTERLACED

#define VID1_WIDTH 		384
#define VID1_HEIGHT		384
#define VID1_BPP		16
#define VID1_FRAME_SIZE		(VID1_WIDTH*VID1_HEIGHT)
#define VID1_VMODE		FB_VMODE_INTERLACED

#define OSD0_BPP		4
#define	OSD0_WIDTH		(round_32(240*OSD0_BPP/8) * 8/OSD0_BPP)
#define	OSD0_HEIGHT		120
#define OSD0_FRAME_SIZE		(OSD0_WIDTH*OSD0_HEIGHT)
#define OSD0_VMODE		FB_VMODE_INTERLACED

#define OSD1_BPP		8
#define	OSD1_WIDTH		(round_32(240*OSD1_BPP/8) * 8/OSD1_BPP)
#define	OSD1_HEIGHT		120
#define OSD1_FRAME_SIZE		(OSD1_WIDTH*OSD1_HEIGHT)
#define OSD1_VMODE		FB_VMODE_INTERLACED

/* position */
#define	OSD0_XPOS		0
#define	OSD0_YPOS		0
#define	OSD1_XPOS		300
#define	OSD1_YPOS		250
#define	VID0_XPOS		0
#define	VID0_YPOS		0
#define	VID1_XPOS		20
#define	VID1_YPOS		100

/* Zoom Params */
#define	OSD0_HZOOM		0
#define	OSD0_VZOOM		0
#define	OSD1_HZOOM		0
#define	OSD1_VZOOM		0
#define	VID0_HZOOM		0
#define	VID0_VZOOM		0
#define	VID1_HZOOM		0
#define	VID1_VZOOM		0

/* OSD window blend factor */
#define OSD1_WINDOW_BF		0
#define OSD1_WINDOW_CK		0
#define OSD1_CK			0
#define OSD0_WINDOW_BF		3
#define OSD0_WINDOW_CK		0
#define OSD0_CK			0

#define VIDEO_NUM_BUFS		3
#define OSD_NUM_BUFS		2
#define RED_COLOR 		249
#define BLUE_COLOR 		140	//blue color
#define RAM_CLUT_IDX 		0xFF
#define BITMAP_COLOR		0x11

#define CURSOR_XPOS		100
#define CURSOR_YPOS		100
#define CURSOR_XRES		50
#define CURSOR_YRES		50
#define CURSOR_THICKNESS	1
#define CURSOR_COLOR		0xF9

#define ATTR_BLINK_INTERVAL	1
#define ATTR_BLEND_VAL 		0xaa

#define ATTRIB_MODE		"mode"
#define ATTRIB_OUTPUT		"output"

#define LOOP_COUNT		500

#define DEBUG
#ifdef DEBUG
#define DBGENTER  	printf("%s : Enter\n", __FUNCTION__);
#define DBGEXIT		printf("%s : Leave\n", __FUNCTION__);
#define PREV_DEBUG(x)	printf("DEBUG:%s:%s:%s\n",__FUNCTION__,__LINE__,x);
#else
#define DBGENTER
#define DBGEXIT
#define PREV_DEBUG(x)
#endif

struct buffer
{
    void *start;
    size_t length;
};

char *dev_name_prev = "/dev/davinci_previewer";
char *dev_name_rsz = "/dev/davinci_resizer";
#define YEE_TABLE_FILE "EE_Table.txt"
static short yee_table[MAX_SIZE_YEE_LUT];

struct vpbe_test_info {
	int vid0_width;
	int vid0_height;
	int vid0_bpp;
	int vid0_frame_size;
	int vid0_vmode;

	int vid1_width;
	int vid1_height;
	int vid1_bpp;
	int vid1_frame_size;
	int vid1_vmode;

	int osd0_bpp;
	int osd0_width;
	int osd0_height;
	int osd0_frame_size;
	int osd0_vmode;

	int osd1_bpp;
	int osd1_width;
	int osd1_height;
	int osd1_frame_size;
	int osd1_vmode;

	int osd0_xpos;
	int osd0_ypos;
	int osd1_xpos;
	int osd1_ypos;
	int vid0_xpos;
	int vid0_ypos;
	int vid1_xpos;
	int vid1_ypos;

	int osd0_hzoom;
	int osd0_vzoom;
	int osd1_hzoom;
	int osd1_vzoom;
	int vid0_hzoom;
	int vid0_vzoom;
	int vid1_hzoom;
	int vid1_vzoom;

	int osd1_window_bf;
	int osd1_window_ck;
	int osd1_ck;
	int osd0_window_bf;
	int osd0_window_ck;
	int osd0_ck;

	int osd0_coloridx;
	int osd1_coloridx;
	int ram_clut_idx;
	int bitmap_color;

	int cursor_xpos;
	int cursor_ypos;
	int cursor_xres;
	int cursor_yres;
	int cursor_thickness;
	int cursor_color;

	int attr_blink_interval;
	int attr_blend_val;
};

static struct vpbe_test_info test_data = {
	VID0_WIDTH,
	VID0_HEIGHT,
	VID0_BPP,
	VID0_FRAME_SIZE,
	VID0_VMODE,

	VID1_WIDTH,
	VID1_HEIGHT,
	VID1_BPP,
	VID1_FRAME_SIZE,
	VID1_VMODE,

	OSD0_BPP,
	OSD0_WIDTH,
	OSD0_HEIGHT,
	OSD0_FRAME_SIZE,
	OSD0_VMODE,

	OSD1_BPP,
	OSD1_WIDTH,
	OSD1_HEIGHT,
	OSD1_FRAME_SIZE,
	OSD1_VMODE,

	OSD0_XPOS,
	OSD0_YPOS,
	OSD1_XPOS,
	OSD1_YPOS,
	VID0_XPOS,
	VID0_YPOS,
	VID1_XPOS,
	VID1_YPOS,

	OSD0_HZOOM,
	OSD0_VZOOM,
	OSD1_HZOOM,
	OSD1_VZOOM,
	VID0_HZOOM,
	VID0_VZOOM,
	VID1_HZOOM,
	VID1_VZOOM,

	OSD1_WINDOW_BF,
	OSD1_WINDOW_CK,
	OSD1_CK,
	OSD0_WINDOW_BF,
	OSD0_WINDOW_CK,
	OSD0_CK,

	BLUE_COLOR,
	RED_COLOR,
	RAM_CLUT_IDX,
	BITMAP_COLOR,

	CURSOR_XPOS,
	CURSOR_YPOS,
	CURSOR_XRES,
	CURSOR_YRES,
	CURSOR_THICKNESS,
	CURSOR_COLOR,

	ATTR_BLINK_INTERVAL,
	ATTR_BLEND_VAL,
};


static int preview_fd;
static int resizer_fd;
static int capture_fd = -1;
static int vid0_fd;
static int linearization_en;
static int csc_en;
static int vldfc_en;
static int en_culling;
static unsigned int n_buffers = 0;
static unsigned long oper_mode_1;
static unsigned long user_mode_1;
static struct rsz_channel_config rsz_chan_config; // resizer channel config
static struct rsz_single_shot_config rsz_ss_config;
static struct rsz_continuous_config rsz_ctn_config;
static struct prev_channel_config prev_chan_config;
static struct prev_single_shot_config prev_ss_config;
static struct prev_continuous_config prev_ctn_config;
static struct v4l2_format fmt;
static struct v4l2_streamparm parm;
static struct v4l2_requestbuffers reqbuf;  // for display
static struct v4l2_requestbuffers req; // for capture
static struct buffer *buffers = NULL; // for capture
static struct buffer *vid0Buf; // for display
static struct fb_var_screeninfo vid1_varInfo, osd0_varInfo, osd1_varInfo,
    prev_vid1_var, prev_osd0_var;
static struct fb_fix_screeninfo vid0_fixInfo, vid1_fixInfo, osd0_fixInfo,
    osd1_fixInfo;
static int fd_vid0, fd_vid1, fd_osd0, fd_osd1;
static int vid0_size, vid1_size, osd0_size, osd1_size;
static char *vid0_display[VIDEO_NUM_BUFS];
static char *vid1_display[VIDEO_NUM_BUFS];
static char *osd0_display[OSD_NUM_BUFS];
static char *osd1_display[OSD_NUM_BUFS];

static void open_resizer(void);
static int parse_yee_table(void);
static void open_previewer(void);
static void open_capture(void);
static void capture_get_chip_id(void);
static void capture_query_capability(void);
static void capture_set_format(void);
static void capture_init_mmap(void);
static void capture_set_input(void);
static void ccdc_config_raw(void);
static void capture_start_streaming(void);
static void open_display_dev(void);
static void display_query_capability(void);
static void display_request_buffers(void);
static void display_set_format(void);
static void display_get_format(void);
static void display_init_mmap(void);
static void display_getinfo_control(void);
static void display_start_streaming(void);
static void start_loop(void);
static int open_all_windows(void);
static void close_all_windows(void);
static void init_vid1_device(void);
static void init_osd0_device(void);
static int mmap_vid1(void);
static int mmap_osd0(void);
static int disable_all_windows(void);
static int display_frame(char id, void *ptr_buffer);

int main(int argc, char **argv)
{
// open_device
    open_resizer();
    open_previewer();
    open_capture();
    open_all_windows();

// init_capture
    //capture_get_chip_id();
    capture_query_capability();
    capture_set_format();
    capture_init_mmap();
    capture_set_input();
    //ccdc_config_raw();
    capture_start_streaming();

// init_display
    open_display_dev();
    display_query_capability();
    display_request_buffers();
    display_set_format();
    display_get_format();
    display_init_mmap();
    //display_getinfo_control();
    display_start_streaming();

//	    init_vid1_device();
//	    //init_osd0_device();
//	    mmap_vid1();
//	    //mmap_osd0();
//	    disable_all_windows();
    start_loop();

    // TODO: release device and memory
    
    return 0;
}

static void open_resizer(void)
{
//    oper_mode_1 = 0;
    oper_mode_1 = IMP_MODE_CONTINUOUS;

//    printf("Configuring resizer in the chain mode\n");
//    printf("Opening resizer device, %s\n",dev_name_rsz);
    resizer_fd = open(dev_name_rsz, O_RDWR);

    if (ioctl(resizer_fd, RSZ_S_OPER_MODE, &oper_mode_1) < 0)
    {
    	perror("Error in setting default configuration for oper mode\n");
        close(resizer_fd);
    }
	
    if (ioctl(resizer_fd, RSZ_G_OPER_MODE, &user_mode_1) < 0)
    {
    	perror("Error in getting default configuration for oper mode\n");
        close(resizer_fd);
    }

    if (oper_mode_1 == user_mode_1)
    {
    //    printf("RESIZER: Operating mode changed successfully to Continuous");
    }
    else 
    {
        //printf("RESIZER: Couldn't change operation mode to Continuous");
        close(resizer_fd);
    }

    rsz_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    rsz_chan_config.chain = 1;
    rsz_chan_config.len = 0;
    rsz_chan_config.config = NULL; /* to set defaults in driver */
    if (ioctl(resizer_fd, RSZ_S_CONFIG, &rsz_chan_config) < 0) 
    {
    	perror("Error in setting default configuration for continuous mode\n");
        close(resizer_fd);
    }

    CLEAR (rsz_ctn_config);
    rsz_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    rsz_chan_config.chain = 1;
    rsz_chan_config.len = sizeof(struct rsz_continuous_config);
    rsz_chan_config.config = &rsz_ctn_config;
    if (ioctl(resizer_fd, RSZ_G_CONFIG, &rsz_chan_config) < 0) 
    {
    	perror("Error in getting default configuration for continuous mode\n");
        close(resizer_fd);
    }

    rsz_ctn_config.output1.enable = 1;
    rsz_ctn_config.output2.enable = 0;
    rsz_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    rsz_chan_config.chain = 1;
    rsz_chan_config.len = sizeof(struct rsz_continuous_config);
    rsz_chan_config.config = &rsz_ctn_config; /* to set defaults in driver */
    if (ioctl(resizer_fd, RSZ_S_CONFIG, &rsz_chan_config) < 0) {
    	perror("Error in setting default configuration for continuous mode\n");
        close(resizer_fd);
    }

#if 0

// form planar_resize.c
    /* getting physical address for mmap */
    ch1_bufd.buf_type = RSZ_BUF_IN;
    ch1_bufd.index = 0;
    if (ioctl(ch1_fd, RSZ_QUERYBUF, &ch1_bufd) == -1)
    {
        printf("query buffer failed \n");
        close(ch1_fd);
        exit(-1);
    }
    ch1_resize.in_buf.offset = ch1_bufd.offset;

    // from do_resize.c
    bzero(&convert,sizeof(convert));
//    bzero(output1_buffer.user_addr, output1_size);
//    memcpy(input_buffer.user_addr, in_buf, size);
    convert.in_buff.buf_type = IMP_BUF_IN;
    convert.in_buff.index = -1;
    //convert.in_buff.offset = (unsigned int)input_buffer.user_addr;
    convert.in_buff.size = 400*400;
    convert.out_buff1.buf_type = IMP_BUF_OUT1;
    convert.out_buff1.index = -1;
//    convert.out_buff1.offset = (unsigned int)output1_buffer.user_addr;
    convert.out_buff1.size = 800*800;

    if (ioctl(resizer_fd, RSZ_RESIZE, &convert) < 0) 
    {
        perror("Error in doing resize\n");
        ret = -1;
//        goto out;
    } 
#endif

}

static int parse_yee_table(void)
{
	int ret = -1, val, i;
	FILE *fp;

	fp = fopen(YEE_TABLE_FILE, "r");
	if (fp == NULL) {
		printf("Error in opening file %s\n", YEE_TABLE_FILE);
		goto out;
	}

	for (i = 0; i < MAX_SIZE_YEE_LUT; i++) {
		fscanf(fp, "%d", &val);
		printf("%d,", val);
		yee_table[i] = val & 0x1FF;
	}
	printf("\n");
	if (i != MAX_SIZE_YEE_LUT)
		goto clean_file;	
	ret = 0;
clean_file:
	fclose(fp);
out:
	return ret;
}


static void open_previewer(void)
{
    oper_mode_1 = IMP_MODE_CONTINUOUS; // same as resizer

    preview_fd = open(dev_name_prev, O_RDWR);

    if(preview_fd <= 0)
    {
        printf("Cannot open previewer device\n");
        close(preview_fd);
    }
	
    if (ioctl(preview_fd, PREV_S_OPER_MODE, &oper_mode_1) < 0) 
    {
        perror("Can't set operation mode\n");
        close(preview_fd);
    }
	
	if (ioctl(preview_fd, PREV_G_OPER_MODE, &user_mode_1) < 0) 
    {
        perror("Can't get operation mode\n");
        close(preview_fd);
    }

    if (oper_mode_1 == user_mode_1) 
    {   
        //printf("Operating mode changed successfully to continuous in previewer");
    }
	else 
    {
        //printf("failed to set mode to continuous in previewer\n");
        close(preview_fd);
    }

    prev_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    prev_chan_config.len = 0;
    prev_chan_config.config = NULL; /* to set defaults in driver */
    if (ioctl(preview_fd, PREV_S_CONFIG, &prev_chan_config) < 0) 
    {
        perror("Error in setting default configuration\n");
        close(preview_fd);
    }

    CLEAR (prev_ctn_config);
    prev_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    prev_chan_config.len = sizeof(struct prev_continuous_config);
    prev_chan_config.config = &prev_ctn_config;
    if (ioctl(preview_fd, PREV_G_CONFIG, &prev_chan_config) < 0) 
    {
        perror("Error in getting configuration from driver\n");
        close(preview_fd);
    }

    prev_chan_config.oper_mode = IMP_MODE_CONTINUOUS;
    prev_chan_config.len = sizeof(struct prev_continuous_config);
    prev_chan_config.config = &prev_ctn_config;
        
#if 0
        /*
            Gb B
            R Gr
        */
            prev_ctn_config.input.colp_elep= IPIPE_RED;
            prev_ctn_config.input.colp_elop= IPIPE_GREEN_RED;
            prev_ctn_config.input.colp_olep= IPIPE_GREEN_BLUE;
            prev_ctn_config.input.colp_olop= IPIPE_BLUE;
#endif
    
    
#if 0
        /* 
            B Gb
            Gr R
        */
            prev_ctn_config.input.colp_elep= IPIPE_GREEN_RED;
            prev_ctn_config.input.colp_elop= IPIPE_RED;
            prev_ctn_config.input.colp_olep= IPIPE_BLUE;
            prev_ctn_config.input.colp_olop= IPIPE_GREEN_BLUE;
#endif
    
#if 1
        /*
            Gr R
            B Gb
        */
                prev_ctn_config.input.colp_elep= IPIPE_BLUE;
                prev_ctn_config.input.colp_elop= IPIPE_GREEN_BLUE;
                prev_ctn_config.input.colp_olep= IPIPE_GREEN_RED;
                prev_ctn_config.input.colp_olop= IPIPE_RED;
#endif
    
#if 0
        /*
            R Gr
            Gb B
        */
    
                prev_ctn_config.input.colp_elep= IPIPE_GREEN_BLUE;
                prev_ctn_config.input.colp_elop= IPIPE_BLUE;
                prev_ctn_config.input.colp_olep= IPIPE_RED;
                prev_ctn_config.input.colp_olop= IPIPE_GREEN_RED;
#endif


	if (ioctl(preview_fd, PREV_S_CONFIG, &prev_chan_config) < 0) 
    {
		perror("Error in setting default configuration\n");
        close(preview_fd);
	}

    struct prev_cap cap;
    struct prev_module_param mod_param;
    struct prev_wb wb;
	struct prev_lum_adj lum_adj;
	struct prev_gamma gamma;
    struct prev_yee yee;
    
    int ret;

    cap.index=0;
    while (1) 
    {
        ret = ioctl(preview_fd , PREV_ENUM_CAP, &cap);
        if (ret < 0)
        {
            break;
        }
        
        // find the defaults for this module

        strcpy(mod_param.version,cap.version);
        mod_param.module_id = cap.module_id;
        // try set parameter for this module
        if(cap.module_id == PREV_WB)
        {
            printf("cap.module_id == PREV_WB\n");
            bzero((void *)&wb, sizeof (struct prev_wb));
            wb.gain_r.integer = 1;
            wb.gain_r.decimal = 0;
            wb.gain_gr.integer = 1;
            wb.gain_gr.decimal = 0;
            wb.gain_gb.integer = 1;
            wb.gain_gb.decimal = 0;
            wb.gain_b.integer = 1;
            wb.gain_b.decimal = 0;
            wb.ofst_r = 0;
            wb.ofst_gb = 0;
            wb.ofst_b = 0;
            mod_param.len = sizeof(struct prev_wb);
            mod_param.param = &wb;
		} 
        else if(cap.module_id == PREV_LUM_ADJ) 
        {
            printf("cap.module_id == PREV_LUM_ADJ\n");
            bzero((void *)&lum_adj, sizeof (struct prev_lum_adj));
			lum_adj.brightness = 0;
			lum_adj.contrast = 20; 
			mod_param.len = sizeof (struct prev_lum_adj);
			mod_param.param = &lum_adj;
        } 
        else if (cap.module_id == PREV_GAMMA) 
        {
            printf("Setting gamma for %s\n", cap.module_name);
            bzero((void *)&gamma, sizeof (struct prev_gamma));
            gamma.bypass_r = 1;
            gamma.bypass_b = 1;
            gamma.bypass_g = 1;
            gamma.tbl_sel = IPIPE_GAMMA_TBL_RAM;
            gamma.tbl_size = IPIPE_GAMMA_TBL_SZ_512;
            mod_param.len = sizeof (struct prev_gamma);
            mod_param.param = &gamma;
        }
        else if (cap.module_id == PREV_YEE)
        {
            printf("Setting Edge Enhancement for %s\n", cap.module_name);
            bzero((void *)&yee, sizeof (struct prev_yee));
            bzero((void *)&yee_table, sizeof (struct prev_yee));
            yee.en = 1;
            //yee.en_halo_red = 1;
            yee.en_halo_red = 0;
            //yee.merge_meth = IPIPE_YEE_ABS_MAX;
            yee.merge_meth = IPIPE_YEE_EE_ES;
            yee.hpf_shft = 6; // 5, 10
            //yee.hpf_coef_00 = 8;
            //yee.hpf_coef_01 = 2;
            //yee.hpf_coef_02 = -2;
            //yee.hpf_coef_10 = 2;
            //yee.hpf_coef_11 = 0;
            //yee.hpf_coef_12 = -1;
            //yee.hpf_coef_20 = -2;
            //yee.hpf_coef_21 = -1;
            //yee.hpf_coef_22 = 0;
            yee.hpf_coef_00 = 84,
            yee.hpf_coef_01 = (-8 & 0x3FF),
            yee.hpf_coef_02 = (-4 & 0x3FF),
            yee.hpf_coef_10 = (-8 & 0x3FF),
            yee.hpf_coef_11 = (-4 & 0x3FF),
            yee.hpf_coef_12 = (-2 & 0x3FF),
            yee.hpf_coef_20 = (-4 & 0x3FF),
            yee.hpf_coef_21 = (-2 & 0x3FF),
            yee.hpf_coef_22 = (-1 & 0x3FF),
            yee.yee_thr = 20; //12
            yee.es_gain = 128;
            yee.es_thr1 = 768;
            yee.es_thr2 = 32;
            yee.es_gain_grad = 32;
            yee.es_ofst_grad = 0;
            if(parse_yee_table() <0)
            {
                printf("read yee table error.\n");
            }
            yee.table = yee_table;
            
            mod_param.len = sizeof (struct prev_yee);
            mod_param.param = &yee;           
        }
        else
        {
            // using defaults
            printf("Setting default for %s\n", cap.module_name);
            mod_param.param = NULL;
        }
        
        if (ioctl(preview_fd, PREV_S_PARAM, &mod_param) < 0)
        {
            printf("Error in Setting %s params from driver\n", cap.module_name);
            close(preview_fd);
            //exit (EXIT_FAILURE);
        }


        cap.index++;
    }
}

static void open_capture(void)
{
    capture_fd = open(CAPTURE_DEVICE, O_RDWR | O_NONBLOCK, 0);

    if (-1 == capture_fd)
    {
        fprintf (stderr, "Cannot open '%s': %d, %s\n", CAPTURE_DEVICE, errno, strerror (errno));
        exit (EXIT_FAILURE);
    }
}

static void capture_get_chip_id(void)
{
    struct v4l2_dbg_chip_ident chip;

    CLEAR (chip);
    if (ioctl(capture_fd, VIDIOC_DBG_G_CHIP_IDENT, &chip))
    {
        printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
        //return FAIL;
    }
    else
    {
        printf("sensor chip is %s\n", chip.match.name);
    }
}

static void capture_query_capability(void)
{
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmtdesc;

    CLEAR (cap);
    if (ioctl(capture_fd, VIDIOC_QUERYCAP, &cap))
    {
        printf("VIDIOC_QUERYCAP failed. %s is no V4L2 device\n", CAPTURE_DEVICE);
        //return FAIL;
    }
    else
    {
        printf("%s is a V4L2 device\n", CAPTURE_DEVICE);
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        {
	    fprintf (stderr, "%s is no video capture device\n", CAPTURE_DEVICE);
            exit (EXIT_FAILURE);
        }
#if 0
        else
        {
            printf("%s is a video capture device\n", CAPTURE_DEVICE);
        }

        if (!(cap.capabilities & V4L2_CAP_READWRITE))
        {
            printf("It can not be read\n");
        }
        else
        {
            printf("It can be read\n");
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            printf("It can not streaming\n");
        }
        else
        {
            printf("It can streaming\n");
        }
#endif
    }

    // TODO: VIDIOC_CROPCAP

    CLEAR (fmtdesc);
    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; 
//    printf("Supportformat:\n");
    while(ioctl(capture_fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1) 
    {
//        printf("\tfmt_desc.index = %d\n", fmtdesc.index);
//        printf(format,args...)("\tfmt_desc.type = %d\n", fmtdesc.type);
//        printf("\tfmt_desc.description = %s\n", fmtdesc.description);
//        printf("\tfmt_desc.pixelformat = %x\n", fmtdesc.pixelformat); 
//        //printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description); 
        fmtdesc.index++; 
    }

    // TODO: VIDIOC_TRY_FMT

}

static void capture_set_format(void)
{
    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(capture_fd, VIDIOC_G_FMT, &fmt);

    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 384;
    fmt.fmt.pix.height = 384;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR8;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    //fmt.fmt.pix.bytesperline=2560;
    printf("pixelformat = %x\n",fmt.fmt.pix.pixelformat);

    if (ioctl(capture_fd, VIDIOC_S_FMT, &fmt))
    {
        printf("VIDIOC_S_FMT failed.\n");
        //return FAIL;
    }

    // frame-rate
    CLEAR (parm);
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 60;
    parm.parm.capture.capturemode = 0;

    if (-1 == ioctl(capture_fd, VIDIOC_S_PARM, &parm))
    {
        //printf("VIDIOC_S_PARM failed ");
    }
//    else
//    {
//        printf("VIDIOC_S_PARM SUCCESS!\n");
//    }

    if (-1 == ioctl(capture_fd, VIDIOC_G_PARM, &parm))
    {
        printf("VIDIOC_G_PARM failed ");
    }
    else
    {
        printf("streamparm:\n\tnumerator =%d\n\tdenominator=%d\n\tcapturemode=%d\n\n",
        parm.parm.capture.timeperframe.numerator,
        parm.parm.capture.timeperframe.denominator,
        parm.parm.capture.capturemode);
    }

}

static void capture_init_mmap(void)
{
    static struct v4l2_buffer buf;

    CLEAR (req);
    req.count = 3;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(capture_fd, VIDIOC_REQBUFS, &req))
    {
        printf("VIDIOC_REQBUFS failed.\n");
        //return FAIL;
    }
    else
    {
        printf("success: request buffers for capture\n");
    }

    // 2 is minimum buffers
    if (req.count < 2)
    {
      fprintf (stderr, "Insufficient buffer memory on %s\n", CAPTURE_DEVICE);
      exit (EXIT_FAILURE);
    }

    // request buffer
    buffers = calloc (req.count, sizeof (*buffers));

    if (!buffers)
    {
        //fprintf (stderr, "Out of memory\n");
        printf("fail: Out of memory for capture\n");
        exit (EXIT_FAILURE);
    }
//    else
//    {
//        printf("success in init_capture_buffers calloc\n");
//    }

    // query buffer status
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
//        printf("process n_buffers %d\n", n_buffers);
        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == ioctl (capture_fd, VIDIOC_QUERYBUF, &buf))
        {
            printf("fail: in init_capture_buffers: VIDIOC_QUERYBUF\n");
            //fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));
            exit (EXIT_FAILURE);
        }
        else
        {
            printf("success: query buffers for capture\n");
//	            printf("buffer.length = %d\n",buf.length);
//	            printf("buffer.bytesused = %d\n",buf.bytesused);
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap (NULL /* start anywhere */ ,
				       buf.length,
				       PROT_READ | PROT_WRITE /* required */ ,
				       MAP_SHARED /* recommended */ ,
				       capture_fd, buf.m.offset);
        printf("buffer:%d phy:%x mmap:%p length:%d\n", 
            buf.index, 
            buf.m.offset, 
            buffers[n_buffers].start, 
            buf.length);

        if (MAP_FAILED == buffers[n_buffers].start)
        {
            printf("failed in init_capture_buffers: mmap\n");
            exit (EXIT_FAILURE);
        }
//        else
//        {
//            printf("success in init_capture_buffers mmap\n");
//        }
    }

}

static void capture_set_input(void)
{
    struct v4l2_input input;
    int ret;

    input.type = V4L2_INPUT_TYPE_CAMERA;
    input.index = 0;

    // query input device
    while ((ret = ioctl(capture_fd,VIDIOC_ENUMINPUT, &input) == 0))
    {
        printf("input.name = %s at input.index = %d\n", input.name, input.index);
        input.index++;
    }

    if (ret < 0) 
    {
        printf("Couldn't find the capture input\n");
        //return -1;
    }

    // setup input device
    input.index = 0;
    //printf("Calling capture VIDIOC_S_INPUT with index = %d\n", input.index);
    if (-1 == ioctl (capture_fd, VIDIOC_S_INPUT, &input.index))
    {
        printf("fail: setup capture input.index = %d\n", input.index);
        //exit (EXIT_FAILURE);
    }
    else
    {
        printf("success: setup capture input.index = %d\n", input.index);
    }

//    int temp_input;
//
//    if (-1 == ioctl (capture_fd, VIDIOC_G_INPUT, &temp_input))
//    {
//        perror("Error:InitDevice:ioctl:VIDIOC_G_INPUT\n");
//        //exit (EXIT_FAILURE);
//    }
//    else
//    {
//        printf("ioctl:VIDIOC_G_INPUT input index %d SUCCESS!\n", temp_input);
//    }
//
//    if (temp_input == input.index)
//    {
//        printf ("InitDevice:ioctl:VIDIOC_G_INPUT, selected input, %s\n", input.name);
//    }
//    else
//    {
//        printf ("Error: InitDevice:ioctl:VIDIOC_G_INPUT,");
//        printf("Couldn't select %s input\n", input.name);
//        //exit (EXIT_FAILURE);
//    }
//
//    printf("Following standards available at the input\n");
//    struct v4l2_standard standard;
//    standard.index = 0;
//    while (0 == (ret = ioctl (capture_fd, VIDIOC_ENUMSTD, &standard)))
//    {
//        printf("standard.index = %d\n", standard.index);
//        printf("\tstandard.id = %llx\n", standard.id);
//        printf("\tstandard.frameperiod.numerator = %d\n", standard.frameperiod.numerator);
//        printf("\tstandard.frameperiod.denominator = %d\n", standard.frameperiod.denominator);
//        printf("\tstandard.framelines = %d\n", standard.framelines);
//        standard.index++;
//    }
//    if(ret!=0)
//    {
//        printf("\tstandard not found!\n");
//    }
}

// ref to configCCDCraw() of capture_prev_rsz_onthe_fly_bayer.c 
static void ccdc_config_raw(void)
{
    struct ccdc_config_params_raw raw_params;

    // Change these values for testing Gain - Offsets
    struct ccdc_float_16 r = {0, 511};
    struct ccdc_float_16 gr = {0, 511};
    struct ccdc_float_16 gb = {0, 511};
    struct ccdc_float_16 b = {0, 511};
    struct ccdc_float_8 csc_coef_val = { 1, 0 };
    int i;

    CLEAR(raw_params);

    // First get the default parameters
    if (-1 == ioctl(capture_fd, VPFE_CMD_G_CCDC_RAW_PARAMS, &raw_params)) 
    {
        printf("fail: get the default ccdc parameters\n");
        //exit (EXIT_FAILURE);
    }

    raw_params.compress.alg = CCDC_NO_COMPRESSION;
    raw_params.gain_offset.gain.r_ye = r; 
    raw_params.gain_offset.gain.gr_cy = gr;
    raw_params.gain_offset.gain.gb_g = gb;
    raw_params.gain_offset.gain.b_mg = b;
    raw_params.gain_offset.gain_sdram_en = 1;
    raw_params.gain_offset.gain_ipipe_en = 1;
    raw_params.gain_offset.offset = 0;
    raw_params.gain_offset.offset_sdram_en = 1;

    // To test linearization, set this to 1, and update the
    // linearization table with correct data
    if (linearization_en)
    {
        raw_params.linearize.en = 1;
        raw_params.linearize.corr_shft = CCDC_1BIT_SHIFT;
        raw_params.linearize.scale_fact.integer = 0;
        raw_params.linearize.scale_fact.decimal = 10;
        for (i = 0; i < CCDC_LINEAR_TAB_SIZE; i++)
        {
            raw_params.linearize.table[i] = i;	
        }
    }
    else
    {
        raw_params.linearize.en = 0;
    }


	// CSC
    if (csc_en)
    {
        raw_params.df_csc.df_or_csc = 0;
        raw_params.df_csc.csc.en = 1;
        // I am hardcoding this here. But this should
        // really match with that of the capture standard
        raw_params.df_csc.start_pix = 1;
        raw_params.df_csc.num_pixels = 384;
        raw_params.df_csc.start_line = 1;
        raw_params.df_csc.num_lines = 384;
        // These are unit test values. For real case, use
        // correct values in this table
        raw_params.df_csc.csc.coeff[0] = csc_coef_val;
        raw_params.df_csc.csc.coeff[1].decimal = 1;
        raw_params.df_csc.csc.coeff[2].decimal = 2;
        raw_params.df_csc.csc.coeff[3].decimal = 3;
        raw_params.df_csc.csc.coeff[4].decimal = 4;
        raw_params.df_csc.csc.coeff[5].decimal = 5;
        raw_params.df_csc.csc.coeff[6].decimal = 6;
        raw_params.df_csc.csc.coeff[7].decimal = 7;
        raw_params.df_csc.csc.coeff[8].decimal = 8;
        raw_params.df_csc.csc.coeff[9].decimal = 9;
        raw_params.df_csc.csc.coeff[10].decimal = 10;
        raw_params.df_csc.csc.coeff[11].decimal = 11;
        raw_params.df_csc.csc.coeff[12].decimal = 12;
        raw_params.df_csc.csc.coeff[13].decimal = 13;
        raw_params.df_csc.csc.coeff[14].decimal = 14;
        raw_params.df_csc.csc.coeff[15].decimal = 15;	
    }
    else
    {
        raw_params.df_csc.df_or_csc = 0;
        raw_params.df_csc.csc.en = 0;
    }

	// vertical line defect correction
    if (vldfc_en)
    {
        raw_params.dfc.en = 1;
        // correction method
        raw_params.dfc.corr_mode = CCDC_VDFC_HORZ_INTERPOL_IF_SAT;
        // not pixels upper than the defect corrected
        raw_params.dfc.corr_whole_line = 1;
        raw_params.dfc.def_level_shift = CCDC_VDFC_SHIFT_2;
        raw_params.dfc.def_sat_level = 20;
        raw_params.dfc.num_vdefects = 7;
        for (i = 0; i < raw_params.dfc.num_vdefects; i++)
        {
            raw_params.dfc.table[i].pos_vert = i;
            raw_params.dfc.table[i].pos_horz = i + 1;
            raw_params.dfc.table[i].level_at_pos = i + 5;
            raw_params.dfc.table[i].level_up_pixels = i + 6;
            raw_params.dfc.table[i].level_low_pixels = i + 7;
        }
        printf("DFC enabled\n");
    }
else
{
    raw_params.dfc.en = 0;
}

    if(en_culling)
    {
        printf("Culling enabled\n");
        raw_params.culling.hcpat_odd  = 0xaa;
        raw_params.culling.hcpat_even = 0xaa;
        raw_params.culling.vcpat = 0x55;
        raw_params.culling.en_lpf = 1;
    }
    else
    {
        raw_params.culling.hcpat_odd  = 0xFF;
        raw_params.culling.hcpat_even = 0xFF;
        raw_params.culling.vcpat = 0xFF;
    }

    raw_params.col_pat_field0.olop = CCDC_GREEN_BLUE;
    raw_params.col_pat_field0.olep = CCDC_BLUE;
    raw_params.col_pat_field0.elop = CCDC_RED;
    raw_params.col_pat_field0.elep = CCDC_GREEN_RED;
    raw_params.col_pat_field1.olop = CCDC_GREEN_BLUE;
    raw_params.col_pat_field1.olep = CCDC_BLUE;
    raw_params.col_pat_field1.elop = CCDC_RED;
    raw_params.col_pat_field1.elep = CCDC_GREEN_RED;
    raw_params.data_size = CCDC_12_BITS;
    raw_params.data_shift = CCDC_NO_SHIFT;

    if (-1 == ioctl(capture_fd, VPFE_CMD_S_CCDC_RAW_PARAMS, &raw_params))
    {
        printf("fail: set the ccdc parameters\n");
        //exit (EXIT_FAILURE);
    }
    else
    {
        printf("success: set the ccdc parameters, size = %d, address = %p\n", 
            sizeof(raw_params),
            &raw_params);
    }
}

static void capture_start_streaming(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i)
    {
        struct v4l2_buffer buf;

        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == ioctl(capture_fd, VIDIOC_QBUF, &buf))
        {
            printf("fail: test capture stream queue buffer %d\n", i);
            //exit (EXIT_FAILURE);
        }
        else
        {
            printf("success: test capture stream queue buffer %d\n", i);
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(capture_fd, VIDIOC_STREAMON, &type))
    {
        printf("fail: capture stream on\n");
        //exit (EXIT_FAILURE);
    }
    else
    {
        printf("success: capture stream on\n\n");
    }
}

static void open_display_dev(void)
{
//
// 1. Open display channel
    //printf("1. Opening VID0 device\n");
    vid0_fd = open(VID0_DEVICE, O_RDWR);
    
    if (-1 == vid0_fd) 
    {
        printf("fail: open VID0 display device\n");
        //exit (EXIT_FAILURE);
    }
    else
    {
        printf("seccess: open VID0 display device\n");
    }

}

static void display_query_capability(void)
{
    int i = 0, ret = 0;
    struct v4l2_capability capability;

    ret = ioctl(vid0_fd, VIDIOC_QUERYCAP, &capability);
    if (ret < 0)
    {
        printf("fail: video out query capability\n");
        //exit (EXIT_FAILURE);
    }
    else
    {
        printf("seccess: video out query capability\n");
        if (capability.capabilities & V4L2_CAP_VIDEO_OUTPUT)
        {
            printf("\tDisplay capability is supported\n");
        }
        if (capability.capabilities & V4L2_CAP_STREAMING)
        {
            printf("\tStreaming is supported\n");
        }
    }

//	    // query display format
//	    struct v4l2_fmtdesc format;
//	    while(1)
//	    {
//	        format.index = i;
//	        format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
//	        ret = ioctl(vid0_fd, VIDIOC_ENUM_FMT, &format);
//	        if (ret < 0)
//	            break;
//	        printf("\tFormat description = %s\n", format.description);
//	        
//	        if (format.type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
//	            printf("\tFormat type is Video Display\n");
//	            
//	        if (format.pixelformat == V4L2_PIX_FMT_UYVY)
//	            printf("\tPixelformat is V4L2_PIX_FMT_UYVY\n");
//	            
//	        i++;
//	    }
   
}

static void display_request_buffers(void)
{
    int ret = 0;

    //printf("2. Test request for buffers\n");
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    reqbuf.count = 3;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(vid0_fd, VIDIOC_REQBUFS, &reqbuf);

    if (ret < 0) 
    {
        printf("fail: request buffers for display\n");
        //exit (EXIT_FAILURE);
    }
    else
    {
        printf("success: request buffers for display\n");
    }
    
}

static void display_set_format(void)
{
    int ret = 0;
    struct v4l2_format setfmt;
    
    CLEAR(setfmt);
    
    setfmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    //setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    //setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
//    setfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
    setfmt.fmt.pix.width = 384;
    setfmt.fmt.pix.height = 384;
    //setfmt.fmt.pix.width = 768;
    //setfmt.fmt.pix.height = 1024;
    //setfmt.fmt.win.w.height=384;
    //setfmt.fmt.win.w.left=100;
    //setfmt.fmt.win.w.top=100;
    //setfmt.fmt.win.w.width=384;
    setfmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    ret = ioctl(vid0_fd, VIDIOC_S_FMT, &setfmt);
    if (ret < 0) 
    {
        //perror("display VIDIOC_S_FMT err\n");
        printf("fail: set format for display\n");

        // TODO: open it when all done
        //close(fd_vid0);

        //exit (EXIT_FAILURE);
    } 
    else
    {
        printf("success: set format for display\n");
    }

    struct v4l2_crop crop;
    CLEAR(crop);

    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    crop.c.height=384;
    crop.c.width=384;
    crop.c.top=10;
    crop.c.left=10;

    ret = ioctl(vid0_fd, VIDIOC_S_CROP, &crop);
    if (ret < 0) 
    {
        //perror("display VIDIOC_S_FMT err\n");
        printf("fail: set crop for display\n");

        // TODO: open it when all done
        //close(fd_vid0);

        //exit (EXIT_FAILURE);
    } 
    else
    {
        printf("success: set crop for display\n");
    }
}

static void display_get_format(void)
{
    //printf("3. Test GetFormat\n");
    
    int ret = 0;
    int disppitch, dispheight, dispwidth;
    struct v4l2_format fmt;

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ret = ioctl(vid0_fd, VIDIOC_G_FMT, &fmt);

    if (ret < 0) 
    {
        printf("fail: get format for display\n");
        //exit (EXIT_FAILURE);
    } 
    else
    {
        printf("success: get format for display\n");

        dispheight = fmt.fmt.pix.height;
        disppitch = fmt.fmt.pix.bytesperline;
        dispwidth = fmt.fmt.pix.width;
        printf("\tdispheight = %d\n\tdisppitch = %d\n\tdispwidth = %d\n", dispheight, disppitch, dispwidth);
        printf("\timagesize = %d\n", fmt.fmt.pix.sizeimage);
    }
}

static void display_init_mmap(void)
{
    //printf("4. Test querying of buffers and mapping them\n");

    int i=0, ret = 0;

    struct v4l2_buffer buf;
    vid0Buf = (struct buffer *)calloc(reqbuf.count, sizeof(struct buffer));

    if (!vid0Buf)
    {
        printf("fail: Out of memory for display\n");
        //fprintf (stderr, "Out of memory\n");
        exit (EXIT_FAILURE);
    }

    for (i = 0; i < reqbuf.count; i++)
    {
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(vid0_fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) 
        {
            printf("fail: query buffer for display\n");
            //exit (EXIT_FAILURE);
        } 
        else
        {
            printf("success: query buffer for display\n");
        }

        vid0Buf[i].length = buf.length; 
        vid0Buf[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, vid0_fd, buf.m.offset);
        printf("buffer:%d phy:%x mmap:%p length:%d\n",
            buf.index,
            buf.m.offset,
            vid0Buf[i].start,
            buf.length);

        if (MAP_FAILED == vid0Buf[i].start)
        {
            printf("fail: mmap buffer for display\n");
        }
        else
        {
            printf("success: mmap buffer for display\n");
        }

        //memset(vid0Buf[i].start, 0x80, buf.length);
		memset(vid0Buf[i].start, 0x00, buf.length);
    }
}

static void display_getinfo_control(void)
{
    int ret;
    struct v4l2_control ctrl_r;
    CLEAR(ctrl_r);
    ret = ioctl(vid0_fd, VIDIOC_G_CTRL, &ctrl_r);
    if (ret < 0) 
    {
        printf("\tError: Querying control info failed: VID0\n");
    }
    else
    {
        printf("\tctrl_r.id = %d, ctrl_r.value = %d\n", ctrl_r.id, ctrl_r.value);
        printf("\tQuerying control info SUCCESS: VID0\n");
    }
}

static void display_start_streaming(void)
{
    int i = 0, ret;
    enum v4l2_buf_type type;

// Queue all the buffers for the initial running
    //printf("6. Test enqueuing of buffers - ");
    for (i = 0; i < reqbuf.count; i++)
    {
        struct v4l2_buffer buf;
        
        CLEAR (buf);
        
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        ret = ioctl(vid0_fd, VIDIOC_QBUF, &buf);

        if(ret < 0)
        {
            printf("fail: test display stream queue buffer %d\n", i);
            //exit (EXIT_FAILURE);
        }
        else
        {
            printf("success: test display stream queue buffer %d\n", i);
        }
    }

    //printf("7. Test STREAM_ON\n");
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ret = ioctl(vid0_fd, VIDIOC_STREAMON, &type);
	if (ret < 0)
	{
        printf("fail: display stream on\n");
        //exit (EXIT_FAILURE);
    }
    else
    {
        printf("success: display stream on\n");
    }

//    while(1);
}

/*******************************************************************************
 *	Does a DEQUEUE and gets/returns the address of the
 *	dequeued buffer
 */
static void *get_display_buffer(int vid_win)
{
	int ret;
	struct v4l2_buffer buf;
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(vid_win, VIDIOC_DQBUF, &buf);
	if (ret < 0) {
		perror("VIDIOC_DQBUF\n");
		return NULL;
	}
	return vid0Buf[buf.index].start;
}

/*******************************************************************************
 *	Takes the address, finds the appropriate index
 *	of the buffer, and QUEUEs the buffer to display
 *	If this part is done in the main loop,
 *	there is no need of this conversionof address
 *	to index as both are available.
 */
static int put_display_buffer(int vid_win, void *addr)
{
	struct v4l2_buffer buf;
	int i, index = 0;
	int ret;
	if (addr == NULL)
		return -1;
	memset(&buf, 0, sizeof(buf));

	for (i = 0; i < 3; i++) {
		if (addr == vid0Buf[i].start) {
			index = i;
			break;
		}
	}
	buf.m.offset = (unsigned long)addr;
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;
//printf("in put_display_buffer();\n");
	ret = ioctl(vid_win, VIDIOC_QBUF, &buf);
	return ret;
}

/*******************************************************************************
 *      Stops Streaming
 */
static int stop_capture(int vid_win)
{
	int ret;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(vid_win, VIDIOC_STREAMOFF, &type);
	return ret;
}

static void start_loop(void)
{
    unsigned int w,h;

	int ret, quit=0;
	struct v4l2_buffer buf;
	static int captFrmCnt = 0, printfn = 0, stress_test = 1, start_loopCnt = 20;
	unsigned char *displaybuffer = NULL;
	unsigned char temp;
	int i,j,k;
	char *ptrPlanar = NULL;
	char *dst = NULL;
    unsigned char * psrc       = NULL;
    unsigned char * pdst[2]   = {NULL, NULL};
	void *src, *dest;
    int dummy;
	
//	    FILE * fin_fd;
//	    FILE * fout_fd;
//	
//	    if((fin_fd = fopen("input_frame.YUV", "wb"))==NULL)
//	    {
//	        printf("fail: open input_frame.YUV\n");
//	    }
//	    else
//	    {
//	        printf("success: open input_frame.YUV\n");
//	    }
//	    
//	    if((fout_fd = fopen("output_frame.YUV", "wb"))==NULL)
//	    {
//	        printf("fail: open output_frame.YUV\n");
//	    }
//	    else
//	    {
//	        printf("success: open output_frame.YUV\n");
//	    }

	//while (!quit)
	while(1)
	{
        // Wait for vertical sync
        if (ioctl(fd_vid1, FBIO_WAITFORVSYNC, &dummy) < -1)
        {
            printf("Failed FBIO_WAITFORVSYNC\n");
			if (EAGAIN == errno)
			{
                printf("disp_again\n");
                continue;
            }
            //return -1;
        }
//	        else
//	        {
//	            printf("Wait for vertical sync\n");
//	        }
        
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		// determine ready buffer
		if (-1 == ioctl(capture_fd, VIDIOC_DQBUF, &buf))
		{
			if (EAGAIN == errno)
				continue;
			printf("StartCameraCaputre:ioctl:VIDIOC_DQBUF\n");
			//return -1;
		}

		/******************* V4L2 display ********************/
		displaybuffer = get_display_buffer(vid0_fd);
		if (NULL == displaybuffer) {
			printf("Error in getting the  display buffer:VID1\n");
			//return ret;
		}
		
		src = buffers[buf.index].start;
		
		dst     = (char *)calloc(294912, sizeof(char));

      
//	//        for (i = 0; i < 768; i += 4)
//	//        {
//	            pdst[0] = dst;          // '1'
//	            pdst[1] = dst + 768;    // '5' 
//	            psrc    = src + 294144; // '13'
//	        
//	        
//	 //       printf("Oops!\n");
//	                for (j = 0; j < 384; ++j)
//	                {
//	                    k = j % 2;
//	                    // copy 4 bytes
//	                    *((unsigned int *)pdst[k]) = *((unsigned int *)psrc);
//	                    
//	//	                    // 
//		                    if(1 == k)
//		                    {
//	//	                        temp = *(pdst[0] - 1);
//	//	                        *(pdst[0] - 1) = *(pdst[1] + 1);
//	//	                        *(pdst[1] + 1) = temp;
//	                            psrc += 768+4;
//		                    }
//	                    
//	                    pdst[k] += 1536;
//	                    psrc    -= 768; // '9'
//	                }
//	        
//	            //dst   += 1536;
//	            //src += 4;
//	
//	
//	            pdst[0] = dst + 4;          // '1'
//	            pdst[1] = dst + 768 + 4;    // '5' 
//	            psrc    = src + 294144 - 1536; // '13'
//	            for (j = 0; j < 384; ++j)
//	                {
//	                    k = j % 2;
//	                    // copy 4 bytes
//	                    *((unsigned int *)pdst[k]) = *((unsigned int *)psrc);
//	                    
//	//	                    // 
//		                    if(1 == k)
//		                    {
//	//	                        temp = *(pdst[0] - 1);
//	//	                        *(pdst[0] - 1) = *(pdst[1] + 1);
//	//	                        *(pdst[1] + 1) = temp;
//	                            psrc += 768+4;
//		                    }
//	                    
//	                    pdst[k] += 1536;
//	                    psrc    -= 768; // '9'
//	                }
//	
//	            pdst[0] = dst + 4 + 4;          // '1'
//	            pdst[1] = dst + 768 + 4 + 4;    // '5' 
//	            psrc    = src + 294144 - 1536 - 1536; // '13'
//	            for (j = 0; j < 384; ++j)
//	                {
//	                    k = j % 2;
//	                    // copy 4 bytes
//	                    *((unsigned int *)pdst[k]) = *((unsigned int *)psrc);
//	                    
//	//	                    // 
//		                    if(1 == k)
//		                    {
//	//	                        temp = *(pdst[0] - 1);
//	//	                        *(pdst[0] - 1) = *(pdst[1] + 1);
//	//	                        *(pdst[1] + 1) = temp;
//	                            psrc += 768+4;
//		                    }
//	                    
//	                    pdst[k] += 1536;
//	                    psrc    -= 768; // '9'
//	                }
//	
//	            pdst[0] = dst + 4 + 4 + 4;          // '1'
//	            pdst[1] = dst + 768 + 4 + 4 + 4;    // '5' 
//	            psrc    = src + 294144 - 1536 - 1536 - 1536; // '13'
//	            for (j = 0; j < 384; ++j)
//	                {
//	                    k = j % 2;
//	                    // copy 4 bytes
//	                    *((unsigned int *)pdst[k]) = *((unsigned int *)psrc);
//	                    
//	//	                    // 
//		                    if(1 == k)
//		                    {
//	//	                        temp = *(pdst[0] - 1);
//	//	                        *(pdst[0] - 1) = *(pdst[1] + 1);
//	//	                        *(pdst[1] + 1) = temp;
//	                            psrc += 768+4;
//		                    }
//	                    
//	                    pdst[k] += 1536;
//	                    psrc    -= 768; // '9'
//	                }
//	
//	            pdst[0] = dst + 4 + 4 + 4 + 4;          // '1'
//	            pdst[1] = dst + 768 + 4 + 4 + 4 + 4;    // '5' 
//	            psrc    = src + 294144 - 1536 - 1536 - 1536 - 1536; // '13'
//	            for (j = 0; j < 384; ++j)
//	                {
//	                    k = j % 2;
//	                    // copy 4 bytes
//	                    *((unsigned int *)pdst[k]) = *((unsigned int *)psrc);
//	                    
//	//	                    // 
//		                    if(1 == k)
//		                    {
//	//	                        temp = *(pdst[0] - 1);
//	//	                        *(pdst[0] - 1) = *(pdst[1] + 1);
//	//	                        *(pdst[1] + 1) = temp;
//	                            psrc += 768+4;
//		                    }
//	                    
//	                    pdst[k] += 1536;
//	                    psrc    -= 768; // '9'
//	                }
//	            printf("Oops!\n");
//	        //}


#if(ROTATE)
        char y_tmp_u,y_tmp_v;
        for (i = 0; i < 768; i += 4)
        {
            pdst[0] = dst + i;
            pdst[1] = dst + 768 + i;
            psrc    = src + 294144 - 384*i;
            for (j = 0; j < 384; j++)
            {
                k = j % 2;
                // copy 4 bytes
                *((unsigned int *)pdst[k]) = *((unsigned int *)psrc);
                if(1 == k)
                {
                    *((unsigned char *)(pdst[k]+1)) = y_tmp_v;
                    psrc += 768+4;
                }
                else
                {
                    y_tmp_v = *((unsigned char *)(psrc+3));
                    psrc -= 768; // '9'
                    y_tmp_u = *((unsigned char *)(psrc+1));
                    *((unsigned char *)(pdst[k]+3)) = y_tmp_u;
                }
                pdst[k] += 1536;
            }
//            printf("Oops!\n");
        }
//        memcpy(dst, src, 294912);
        memcpy(src, dst, 294912);
#endif /* ROTATE */

		dest = displaybuffer;

        /* Display image onto requested video window */
        for(i=0 ; i < 768; i++) 
        {
            memcpy(dest, src, 384*2);
            src += 384*2;
            dest += 384*2;
        }

        free(dst);

//	        if(quit)
//	        {
//	            fwrite((void *)buffers[buf.index].start,1,294912,fin_fd);
//	            fwrite((void *)vid0Buf[buf.index].start,1,1572864,fout_fd);
//	            return;
//	        }
        
//	        printf("Oops!\n");
//	        
//	//	        for(h=0 ; h < 720; i++) 
//	//	        {
//	//	            for(w = 0; w<1024;w++)
//	//	            {s
//	//	                memcpy(dest, src, 1);
//	//	                src++;
//	//	                dest++;
//	//	            }
//	//	        }
//		
//	        for(i=0 ; i < 768; i++) 
//	        {
//	            memset(dest, 0x00, 512*2);
//	            dest += 512*2;
//	            memcpy(dest, src, 384*2);
//	            src += 384*2;
//	            dest += 512*2;
//	        }
//	      

//	    printf("Oops!!\n");

		ret = put_display_buffer(vid0_fd, displaybuffer);
    //printf("put_display_buffer\n");
		if (ret < 0) {
			printf("Error in putting the display buffer\n");
			//return ret;
		}
		/***************** END V4L2 display ******************/
#if(FBDEV)
        display_frame(VID1, src);
#endif
		if (printfn)
			printf("time:%lu    frame:%u\n", (unsigned long)time(NULL),
		       		captFrmCnt++);

		/* requeue the buffer */
		if (-1 == ioctl(capture_fd, VIDIOC_QBUF, &buf))
			printf("StartCameraCaputre:ioctl:VIDIOC_QBUF\n");
		if (stress_test) {
			start_loopCnt--;
			if (start_loopCnt == 0) {
				start_loopCnt = 50;
				//break;
				quit =1;
			}
		}
	}
	ret = stop_capture(capture_fd);
	if (ret < 0)
		printf("Error in VIDIOC_STREAMOFF:capture\n");
	//return ret;
}


static int open_all_windows(void)
{
	if ((fd_vid0 = open(FBVID0_DEVICE, O_RDWR)) < 0)
		goto open_all_exit;
	if ((fd_vid1 = open(FBVID1_DEVICE, O_RDWR)) < 0)
		goto open_all_exit;
	if ((fd_osd0 = open(OSD0_DEVICE, O_RDWR)) < 0)
		goto open_all_exit;
	if ((fd_osd1 = open(OSD1_DEVICE, O_RDWR)) < 0)
		goto open_all_exit;
	return SUCCESS;
      open_all_exit:
	close_all_windows();
	return FAILURE;
}

static void close_all_windows(void)
{
	if (fd_vid0)
		close(fd_vid0);
	if (fd_vid1)
		close(fd_vid1);
	if (fd_osd0)
		close(fd_osd0);
	if (fd_osd1)
		close(fd_osd1);
}

static void init_vid1_device(void)
{
    vpbe_window_position_t pos;
    if (ioctl(fd_vid1, FBIOGET_FSCREENINFO, &vid1_fixInfo) < 0)
    {
		printf("\nFailed FBIOGET_FSCREENINFO vid1");
		//return FAILURE;
	}

	if (ioctl(fd_vid1, FBIOGET_VSCREENINFO, &vid1_varInfo) < 0)
	{
		printf("\nFailed FBIOGET_VSCREENINFO\n");
		//return FAILURE;
	}

    vid1_varInfo.xres = test_data.vid1_width;
	vid1_varInfo.yres = test_data.vid1_height;
	vid1_varInfo.vmode = test_data.vid1_vmode;
	vid1_varInfo.bits_per_pixel = test_data.vid1_bpp;

	vid1_varInfo.yres_virtual = vid1_varInfo.yres * VIDEO_NUM_BUFS;

	// Set vid1 window format
	if (ioctl(fd_vid1, FBIOPUT_VSCREENINFO, &vid1_varInfo) < 0)
	{
		printf("\nFailed FBIOPUT_VSCREENINFO for vid1\n");
		//return FAILURE;
	}
	else
	{
	    printf("\nSuccess FBIOPUT_VSCREENINFO for vid1\n");
	}

    // Set window position
	pos.xpos = test_data.vid1_xpos;
	pos.ypos = test_data.vid1_ypos;

    //printf("", );
	if (ioctl(fd_vid1, FBIO_SETPOS, &pos) < 0)
	{
	    printf("\npos.xpos=%d, pos.ypos=%d", pos.xpos, pos.ypos);
		printf("\nFailed  FBIO_SETPOS");
		//return FAILURE;
	}

	// Enable the window
	if (ioctl(fd_vid1, FBIOBLANK, 0))
	{
		printf("\nError enabling VID1\n");
		//return FAILURE;
	}
}


static void init_osd0_device(void)
{
    vpbe_window_position_t pos;
    
    if (ioctl(fd_osd0, FBIOGET_FSCREENINFO, &osd0_fixInfo) < 0)
    {
        printf("\nFailed FBIOGET_FSCREENINFO osd0");
        //return FAILURE;
    }

    // Get Existing var_screeninfo for osd0 window
	if (ioctl(fd_osd0, FBIOGET_VSCREENINFO, &osd0_varInfo) < 0)
	{
		printf("\nFailed FBIOGET_VSCREENINFO");
		//return FAILURE;
	}

    // Modify the resolution and bpp as required
	osd0_varInfo.xres = test_data.osd0_width;
	osd0_varInfo.yres = test_data.osd0_height;
	osd0_varInfo.bits_per_pixel = test_data.osd0_bpp;
	osd0_varInfo.vmode = test_data.osd0_vmode;

	// Change the virtual Y-resolution for buffer flipping (2 buffers)
	osd0_varInfo.yres_virtual = osd0_varInfo.yres * OSD_NUM_BUFS;

	// Set osd0 window format
	if (ioctl(fd_osd0, FBIOPUT_VSCREENINFO, &osd0_varInfo) < 0)
	{
		printf("\nFailed FBIOPUT_VSCREENINFO for osd0");
		//return FAILURE;
	}

    // Set window position
	pos.xpos = test_data.osd0_xpos;
	pos.ypos = test_data.osd0_ypos;

	if (ioctl(fd_osd0, FBIO_SETPOSX, test_data.osd0_xpos) < 0)
	{
		printf("\nFailed  FBIO_SETPOSX");
		//return FAILURE;
	}
	if (ioctl(fd_osd0, FBIO_SETPOSY, test_data.osd0_ypos) < 0)
	{
		printf("\nFailed  FBIO_SETPOSY");
		//return FAILURE;
	}

	// Enable the window
	if (ioctl(fd_osd0, FBIOBLANK, 0))
	{
		printf("Error enabling OSD0\n");
		//return FAILURE;
	}
}

static int mmap_vid1(void)
{
	int i;
	vid1_size = vid1_fixInfo.line_length * vid1_varInfo.yres;
	/* Map the video0 buffers to user space */
	vid1_display[0] = (char *)mmap(NULL, vid1_size * VIDEO_NUM_BUFS,
				       PROT_READ | PROT_WRITE, MAP_SHARED,
				       fd_vid1, 0);

	if (vid1_display[0] == MAP_FAILED) {
		printf("\nFailed mmap on %s", FBVID1_DEVICE);
		return FAILURE;
	}

	for (i = 0; i < VIDEO_NUM_BUFS - 1; i++) {
		vid1_display[i + 1] = vid1_display[i] + vid1_size;
		printf("Display buffer %d mapped to address %#lx\n", i + 1,
		       (unsigned long)vid1_display[i + 1]);
	}
	return SUCCESS;
}

static int mmap_osd0(void)
{
	int i;
	osd0_size = osd0_fixInfo.line_length * osd0_varInfo.yres;

	/* Map the osd0 buffers to user space */
	osd0_display[0] = (char *)mmap(NULL, osd0_size * OSD_NUM_BUFS,
				       PROT_READ | PROT_WRITE, MAP_SHARED,
				       fd_osd0, 0);

	if (osd0_display[0] == MAP_FAILED) {
		printf("\nFailed mmap on %s", OSD0_DEVICE);
		return FAILURE;
	}

	for (i = 0; i < OSD_NUM_BUFS - 1; i++) {
		osd0_display[i + 1] = osd0_display[i] + osd0_size;
		printf("Display buffer %d mapped to address %#lx\n", i + 1,
		       (unsigned long)osd0_display[i + 1]);
	}
	return SUCCESS;
}

static int disable_all_windows(void)
{
	int fd;

	// Disbale OSD0
	fd = open(OSD0_DEVICE, O_RDWR);
	if (!fd) {
		printf("Error: cannot open OSD0\n");
		return -1;
	}
	if (ioctl(fd, FBIOBLANK, 1)) {
		printf("Error disabling the window OSD0\n");
		return -2;
	}
	close(fd);

	// Disbale OSD1
	fd = open(OSD1_DEVICE, O_RDWR);
	if (!fd) {
		printf("Error: cannot open OSD1\n");
		return -1;
	}
	if (ioctl(fd, FBIOBLANK, 1)) {
		printf("Error disabling the window OSD0\n");
		return -2;
	}
	close(fd);

	// Disbale VID0
	fd = open(FBVID0_DEVICE, O_RDWR);
	if (!fd) {
		printf("Error: cannot open VID0\n");
		return -1;
	}
	if (ioctl(fd, FBIOBLANK, 1)) {
		printf("Error disabling the window VID0\n");
		return -2;
	}
	close(fd);

	// Disbale VID1
	fd = open(FBVID1_DEVICE, O_RDWR);
	if (!fd) {
		printf("Error: cannot open VID1\n");
		return -1;
	}
	if (ioctl(fd, FBIOBLANK, 1)) {
		printf("Error disabling the window VID1\n");
		return -2;
	}
	close(fd);

	return SUCCESS;
}

static int display_frame(char id, void *ptr_buffer)
{
	static unsigned int nDisplayIdx = 0;
	static unsigned int nWorkingIndex = 1;
	int y;
	int yres;
	char *dst;
	char *src;
	int fd;
	unsigned int line_length;

	if (id == VID0) {
		yres = test_data.vid0_height;
		dst = vid0_display[nWorkingIndex];
		if (dst == NULL)
			return -1;
		fd = fd_vid0;
		line_length = vid0_fixInfo.line_length;
	}
	if (id == VID1) {
		yres = test_data.vid1_height;
		dst = vid1_display[nWorkingIndex];
		if (dst == NULL)
			return -1;
		fd = fd_vid1;
		line_length = vid1_fixInfo.line_length;
	}
	src = ptr_buffer;
	
//		for (y = 0; y < VID1_HEIGHT; y++) {
//			memcpy(dst, src, (VID1_WIDTH * 2));
//			dst += line_length;
//			src += VID1_WIDTH * 2;//(360 * 2);
//		}
    memcpy(dst, src, 294912);
	
	nWorkingIndex = (nWorkingIndex + 1) % VIDEO_NUM_BUFS;
	nDisplayIdx = (nDisplayIdx + 1) % VIDEO_NUM_BUFS;
//		if ((flip_video_buffers(fd, nDisplayIdx)) < 0)
//			return -1;
	return 0;
}

