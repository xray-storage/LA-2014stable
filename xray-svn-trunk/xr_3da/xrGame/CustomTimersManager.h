#pragma once

#include "script_export_space.h"
#include "CustomTimer.h"
//#include "alife_registry_wrappers.h"
#include "ui.h"
#include "HudManager.h"
#include "UIGameSP.h"

class CTimersManager
{
	typedef xr_vector<CTimerCustom>::iterator VECTOR_TIMERS_ITERATOR;
public:
					CTimersManager			(void);
	virtual			~CTimersManager			(void);

	void			Update					();

	void			save					(IWriter &memory_stream);
	void			load					(IReader &file_stream);

	bool			AddTimer				(CTimerCustom timer);
	void			RemoveTimer				(LPCSTR name);
	CTimerCustom*	GetTimerByName			(LPCSTR name);
	bool			TimerExist				(LPCSTR name);

	void			OnHud					(CTimerCustom *t,bool b);
	bool			IsAnyHUDTimerActive		() {return b_HUDTimerActive;};


private:
	xr_vector<CTimerCustom> objects;
	CTimerCustom			*hud_timer;
	CUIStatic				*ui_hud_timer;
	bool					b_HUDTimerActive;
	CTimerCustom*	SearchTimer				(LPCSTR name);

public:
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CTimersManager)
#undef script_type_list
#define script_type_list save_type_list(CTimersManager)