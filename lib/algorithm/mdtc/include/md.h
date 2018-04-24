/*
 * md.h
 *
 *  Created on: 2014/1/8
 *      Author: chris
 */

#ifndef MD_HPP_
#define MD_HPP_

#ifdef __cplusplus
extern "C" {
#endif

#define AIT_MD_LIB_VER	(20160926)


typedef struct MD_params_in_s
{
	//(0: disable, 1: enable)
	unsigned char enable;
	//(0 ~ 99)
    unsigned char size_perct_thd_min;
    //(1 ~ 100), must be larger than size_perct_thd_min
    unsigned char size_perct_thd_max;
    //(10, 20, 30, ..., 100), 100 is the most sensitive
    unsigned char sensitivity;
    //(1000 ~ 30000)
    unsigned short learn_rate;
} MD_params_in_t;

typedef struct MD_params_out_s
{
    unsigned char md_result;
    unsigned int obj_cnt;
} MD_params_out_t;

typedef struct MD_block_info_s
{
	unsigned short st_x;
	unsigned short st_y;
	unsigned short end_x;
	unsigned short end_y;
} MD_block_info_t;


typedef struct MD_init_s
{
	unsigned char* working_buf_ptr;
	int working_buf_len;
	unsigned short width;
	unsigned short height;
	union mux_t {
	    unsigned char color;
	    unsigned char roi_num ;
	} mux;
} MD_init_t;

union win_s {
    unsigned char win[2] ;//w_num;
    //unsigned char w_div;
    short    roi_id;
}  ;

typedef struct MD_detect_window_s
{
	unsigned short lt_x; //left top X
	unsigned short lt_y; //left top Y
	unsigned short rb_x; //right bottom X
	unsigned short rb_y; //right bottom Y
	union win_s    win_roi;
	//unsigned char  w_div;//divide how many window in horizotal
	//unsigned char  h_div;//divide how many window in vertical
} MD_detect_window_t;


typedef struct MD_window_parameter_in_s
{
    union win_s win_roi ;
	//unsigned char h_num; 
	MD_params_in_t param;
} MD_window_parameter_in_t;

typedef struct MD_window_parameter_out_s
{
	union win_s win_roi ;
	//unsigned char h_num; 
	MD_params_out_t param;
} MD_window_parameter_out_t;

typedef struct MD_buffer_info_s
{
	unsigned short width;
	unsigned short height;
	unsigned char color;
	unsigned char w_div;
	unsigned char h_div;
	unsigned long return_size;		
} MD_buffer_info_t;

typedef struct MD_suspend_s
{
	unsigned char md_suspend_enable;
	unsigned char md_suspend_duration;
	unsigned char md_suspend_threshold;
} MD_suspend_t;

typedef struct MD_motion_info_s
{
	unsigned short obj_cnt ;
	unsigned short obj_axis[256] ;
	unsigned short obj_cnt_sum ;
} MD_motion_info_t ;


typedef struct MD_proc_info_s
{
    unsigned char *frame ;
    MD_motion_info_t result ;      
} MD_proc_info_t ;

extern unsigned int MD_GetLibVersion(unsigned int* ver);
extern int MD_init(unsigned char* working_buf_ptr, int working_buf_len, unsigned short width, unsigned short height, unsigned char color);
extern int MD_run(const unsigned char *_ucImage);
extern int MD_set_detect_window(unsigned short lt_x, unsigned short lt_y, unsigned short rb_x, unsigned short rb_y, unsigned char w_div, unsigned char h_div);
extern int MD_get_detect_window_size(unsigned short* st_x, unsigned short* st_y, unsigned short* div_w, unsigned short* div_h);
extern int MD_set_window_params_in(unsigned char w_num, unsigned char h_num, const MD_params_in_t* param);
extern int MD_get_window_params_in(unsigned char w_num, unsigned char h_num, MD_params_in_t* param);
extern int MD_get_window_params_out(unsigned char w_num, unsigned char h_num, MD_params_out_t* param);
extern int MD_get_buffer_info(unsigned short width, unsigned short height, unsigned char color, unsigned char w_div, unsigned char h_div);
extern int MD_set_region_info(unsigned char num, MD_block_info_t* blk_info);
extern int MD_pixel_downsample(unsigned char pix_num);
extern void MD_printk(char *fmt, ...);
extern void MD_set_time_ms(unsigned int time_diff);
extern void MD_get_Y_mean(unsigned int* mean);

extern unsigned int IvaMD_GetLibVersion(unsigned int* ver);
extern int IvaMD_Init(unsigned char* working_buf_ptr, int working_buf_len, unsigned short width, unsigned short height, unsigned char color);
extern int IvaMD_Run(const unsigned char* _ucImage);
extern int IvaMD_SetDetectWindow(unsigned short lt_x, unsigned short lt_y, unsigned short rb_x, unsigned short rb_y, unsigned char w_div, unsigned char h_div);
extern int IvaMD_GetDetectWindowSize(unsigned short* st_x, unsigned short* st_y, unsigned short* div_w, unsigned short* div_h);
extern int IvaMD_SetWindowParamsIn(unsigned char w_num, unsigned char h_num, const MD_params_in_t* param);
extern int IvaMD_GetWindowParamsIn(unsigned char w_num, unsigned char h_num, MD_params_in_t* param);
extern int IvaMD_GetWindowParamsOut(unsigned char w_num, unsigned char h_num, MD_params_out_t* param);
extern int IvaMD_GetBufferInfo(unsigned short width, unsigned short height, unsigned char color, unsigned char w_div, unsigned char h_div);
//extern void IvaMD_Release();
extern int IvaMD_SetRegionInfo(unsigned char num, MD_block_info_t* blk_info);
extern int IvaMD_PixelDownsample(unsigned char pix_num);
extern void IvaMD_SetTimeMs(unsigned int time_diff);
extern void IvaMD_GetYMean(unsigned int* mean);

void EstablishMD(void);
#ifdef __cplusplus
}
#endif

#endif /* MD_HPP_ */
