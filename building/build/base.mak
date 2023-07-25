

#++++++++++++++++++++++++++++++++++++++++++++++++++++
#	BUILDING Product Define
#++++++++++++++++++++++++++++++++++++++++++++++++++++
BUILD_ROOT          :=          $(shell pwd)
LUX_COMMON_PATH     :=          $(BUILD_ROOT)/common
LUX_HAL_PATH        :=          $(BUILD_ROOT)/hal
LUX_IMP_PATH        :=          $(BUILD_ROOT)/imp
LUX_SYSUTILS_PATH   :=          $(BUILD_ROOT)/sysutils
LUX_MODULES_PATH    :=          $(BUILD_ROOT)/modules
LUX_SAMPLE_PATH     :=          $(BUILD_ROOT)/sample

include $(LUX_COMMON_PATH)/inc.mak
include $(LUX_HAL_PATH)/inc.mak
include $(LUX_IMP_PATH)/inc.mak
include $(LUX_SYSUTILS_PATH)/inc.mak
include $(LUX_MODULES_PATH)/inc.mak
include $(LUX_SAMPLE_PATH)/inc.mak

#test if the path is  is positive
#$(warning "test--------------- $(LUX_COMMON_PATH) -----------------")

