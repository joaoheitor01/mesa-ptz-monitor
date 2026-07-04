/*
 * Mesa PTZ Monitor — plugin nativo para OBS Studio
 * Registra o dock "Mesa PTZ" no menu Docks do OBS.
 * Requer OBS Studio 30 ou mais recente.
 */

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "mesa-ptz-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("mesa-ptz-monitor", "en-US")

bool obs_module_load(void)
{
	obs_frontend_add_dock_by_id("mesa_ptz_monitor", "Mesa PTZ",
				    new MesaPtzDock());
	blog(LOG_INFO, "[mesa-ptz-monitor] plugin carregado");
	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[mesa-ptz-monitor] plugin descarregado");
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Painel de visualizacao da mesa controladora PTZ (Arduino + Python)";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "Mesa PTZ Monitor";
}
