#ifndef __TUYA_MD_DEMO_H_
#define __TUYA_MD_DEMO_H_
#include <stdio.h>
#include <tuya_cloud_error_code.h>

void *thread_md_proc(void *arg);
OPERATE_RET TUYA_APP_Enable_AI_Detect();

/*设置报警区域初始化值*/
void IPC_APP_SetAlarmZone_InitValue(void);

#endif
