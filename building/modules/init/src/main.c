#include <stdio.h>
#include <lux_product_init.h>
#include <lux_product_common.h>
#include <lux_product_led.h>
#include <lux_imp.h>
#include <lux_sysutils.h>
#include <lux_product_tuya.h>
#include <lux_sample.h>

int main(int argc, char *argv[])
{
  init();
  LUXSHARE_product_common();
  LUXSHARE_product_led();
  LUXSHARE_IMP();
  LUXSHARE_SYSUTILS();
  LUXSHARE_tuya();
  LUXSHARE_sample();
  LUSHARE_test_sample_Encode_video(argc, &argv[0]);
  return 0;

}
