#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-outputs", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "OBS ByteDance RTC Streaming";
}

extern struct obs_output_info createOutputInfo();
struct obs_output_info bytertc_output_info;

bool obs_module_load(void)
{
	bytertc_output_info = createOutputInfo();
	obs_register_output(&bytertc_output_info);
	module_load_settings();
	return true;
}

void obs_module_unload(void) {
	blog(LOG_INFO, "unload byteengine");
}
