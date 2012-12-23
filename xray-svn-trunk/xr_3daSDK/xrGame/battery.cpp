#include "stdafx.h"
#include "battery.h"
#include "PhysicsShell.h"
#include "Actor.h"

CBattery::CBattery()
{
}

CBattery::~CBattery()
{
}

BOOL CBattery::net_Spawn(CSE_Abstract* DC) 
{
	return		(inherited::net_Spawn(DC));
}

void CBattery::Load(LPCSTR section) 
{
	inherited::Load(section);
}

void CBattery::net_Destroy() 
{
	inherited::net_Destroy();
}

void CBattery::shedule_Update(u32 dt) 
{
	inherited::shedule_Update(dt);

}

void CBattery::UpdateCL() 
{
	inherited::UpdateCL();
}


void CBattery::OnH_A_Chield() 
{
	inherited::OnH_A_Chield		();
}

void CBattery::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);
}

void CBattery::renderable_Render() 
{
	inherited::renderable_Render();
}

void CBattery::UseBy (CEntityAlive* entity_alive)
{
	inherited::UseBy(entity_alive);
	CInventoryOwner *IO				= smart_cast<CInventoryOwner*>(entity_alive);
	CActor			*actor			= NULL;
	
	R_ASSERT						(IO);

	actor							= smart_cast<CActor*>(IO);
	R_ASSERT						(actor);
	actor->RechargeTorchBattery		();
}
