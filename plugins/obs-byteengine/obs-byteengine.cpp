#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("byteengine", "en-US")

extern struct obs_output_info create_byteengine_output_info();
struct obs_output_info bytertc_output_info;


MODULE_EXPORT const char *obs_module_description(void)
{
	return "byteengine";
}



bool obs_module_load(void)
{
	bytertc_output_info = create_byteengine_output_info();
	obs_register_output(&bytertc_output_info);
	//module_load_settings();
	return true;
}

void obs_module_unload(void) {
}
