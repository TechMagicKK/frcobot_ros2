#ifndef VERSION_CONTROL_H_
#define VERSION_CONTROL_H_

//版本号规则：V(VERSION_MAJOR).(VESION_MINOR).(VERSION_MINOR2)
#define VERSION_MAJOR 3
#define VERSION_MINOR 0
#define VERSION_MINOR2 0

//用于记录对应机械臂软件版本号
#define VERSION_ROBOT_MARJOR 3
#define VERSION_ROBOT_MINOR 8
#define VERSION_ROBOT_MINOR2 4


//用于记录fairino_msgs版本号
/*
V3.0.3 将error_code拆分成main_error_code和sub_error_code并实现反馈
V3.0.4 新增扩展轴状态数组
*/
#define VERSION_MSG_MARJOR 3
#define VERSION_MSG_MINOR 0
#define VERSION_MSG_MINOR2 4

//日志语言选择 / Log language: enable exactly one
//#define CHN_VERSION
#define ENG_VERSION

//FR_MSG(中文, English): 按上面选择的语言返回日志文本 / pick the log text for the selected language
#ifdef CHN_VERSION
#define FR_MSG(zh, en) zh
#else
#define FR_MSG(zh, en) en
#endif
#endif
