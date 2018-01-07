#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>

#include <game/server/gamecontroller.h>
#include <game/server/gamecontext.h>

#include <game/generated/protocol.h>

#include "luabinding.h"
#include "lua.h"

using namespace luabridge;

void CLua::RegisterLuaCallbacks()
{
	lua_State *L = m_pLuaState;

	lua_register(L, "print", CLuaBinding::Print);
	lua_register(L, "throw", CLuaBinding::Throw); // adds an exception, but doesn't jump out like 'error' does
	lua_register(L, "script_name", CLuaBinding::ScriptName);

	getGlobalNamespace(L)

		// global types
		.beginClass< vector2_base<float> >("vec2")
			.addConstructor <void (*) (float, float)> ()
			.addFunction("__add", &vector2_base<float>::operator+)
			.addFunction("__sub", &vector2_base<float>::operator-)
			.addFunction("__mul", &vector2_base<float>::operator*)
			.addFunction("__div", &vector2_base<float>::operator/)
			.addFunction("__eq", &vector2_base<float>::operator==)
			.addFunction("__tostring", &vector2_base<float>::tostring)
			.addData("x", &vector2_base<float>::x)
			.addData("y", &vector2_base<float>::y)
			.addData("u", &vector2_base<float>::u)
			.addData("v", &vector2_base<float>::v)
		.endClass()
		.beginClass< vector3_base<float> >("vec3")
			.addConstructor <void (*) (float, float, float)> ()
			.addFunction("__add", &vector3_base<float>::operator+)
			.addFunction("__sub", &vector3_base<float>::operator-)
			.addFunction("__mul", &vector3_base<float>::operator*)
			.addFunction("__div", &vector3_base<float>::operator/)
			.addFunction("__eq", &vector3_base<float>::operator==)
			.addFunction("__tostring", &vector3_base<float>::tostring)
			.addData("x", &vector3_base<float>::x)
			.addData("y", &vector3_base<float>::y)
			.addData("z", &vector3_base<float>::z)
			.addData("r", &vector3_base<float>::r)
			.addData("g", &vector3_base<float>::g)
			.addData("b", &vector3_base<float>::b)
			.addData("h", &vector3_base<float>::h)
			.addData("s", &vector3_base<float>::s)
			.addData("l", &vector3_base<float>::l)
			.addData("v", &vector3_base<float>::v)
		.endClass()
		.beginClass< vector4_base<float> >("vec4")
			.addConstructor <void (*) (float, float, float, float)> ()
			.addFunction("__add", &vector4_base<float>::operator+)
			.addFunction("__sub", &vector4_base<float>::sub)
			.addFunction("__mul", &vector4_base<float>::mul)
			.addFunction("__div", &vector4_base<float>::div)
			.addFunction("__eq", &vector4_base<float>::operator==)
			.addFunction("__tostring", &vector4_base<float>::tostring)
			.addData("r", &vector4_base<float>::r)
			.addData("g", &vector4_base<float>::g)
			.addData("b", &vector4_base<float>::b)
			.addData("a", &vector4_base<float>::a)
			.addData("x", &vector4_base<float>::x)
			.addData("y", &vector4_base<float>::y)
			.addData("z", &vector4_base<float>::z)
			.addData("w", &vector4_base<float>::w)
			.addData("u", &vector4_base<float>::u)
			.addData("v", &vector4_base<float>::v)
		.endClass()

		.beginClass<IGameController>("IGameController")
		.endClass()

		.beginClass<CGameContext>("CGameContext")
			.addFunction("CreateDamageInd", &CGameContext::CreateDamageInd)
		.endClass()

		.beginClass<IServer>("IServer")
			.addProperty("Tick", &IServer::Tick)
			.addProperty("TickSpeed", &IServer::TickSpeed)
			.addProperty("MaxClients", &IServer::MaxClients)
			.addFunction("ClientName", &IServer::ClientName)
			.addFunction("ClientClan", &IServer::ClientClan)
			.addFunction("ClientCountry", &IServer::ClientCountry)
			.addFunction("ClientIngame", &IServer::ClientIngame)
			.addFunction("GetClientInfo", &IServer::GetClientInfo)
			.addFunction("GetClientAddr", &IServer::GetClientAddr)
		.endClass()

		.beginClass<CGameWorld>("CGameWorld")
		.endClass()

		.beginClass<CPlayer>("CPlayer")
		.endClass()

		.beginClass<CNetObj_PlayerInput>("CNetObj_PlayerInput")
			.addData("Direction", &CNetObj_PlayerInput::m_Direction)
			.addData("TargetX", &CNetObj_PlayerInput::m_TargetX)
			.addData("TargetY", &CNetObj_PlayerInput::m_TargetY)
			.addData("Jump", &CNetObj_PlayerInput::m_Jump)
			.addData("Fire", &CNetObj_PlayerInput::m_Fire)
			.addData("Hook", &CNetObj_PlayerInput::m_Hook)
			.addData("PlayerFlags", &CNetObj_PlayerInput::m_PlayerFlags)
			.addData("WantedWeapon", &CNetObj_PlayerInput::m_WantedWeapon)
			.addData("NextWeapon", &CNetObj_PlayerInput::m_NextWeapon)
			.addData("PrevWeapon", &CNetObj_PlayerInput::m_PrevWeapon)
		.endClass()

		.beginClass<CEntity>("CEntity")
			.addConstructor<void (*) (CGameWorld*, int) > ()

			.addData("m_ProximityRadius", &CEntity::m_ProximityRadius)
			.addData("m_Pos", &CEntity::m_Pos)

			.addFunction("GameWorld", &CEntity::GameWorld)
			.addFunction("GameServer", &CEntity::GameServer)
			.addFunction("Server", &CEntity::Server)

			.addFunction("TypeNext", &CEntity::TypeNext)
			.addFunction("TypePrev", &CEntity::TypePrev)

			.addFunction("Destroy", &CEntity::Destroy)
			.addFunction("Reset", &CEntity::Reset)
			.addFunction("Tick", &CEntity::Tick)
			.addFunction("TickDefered", &CEntity::TickDefered)
			.addFunction("TickPaused", &CEntity::TickPaused)
			.addFunction("Snap", &CEntity::Snap)

			.addFunction("NetworkClipped", &CEntity::NetworkClippedLua)
			.addFunction("GameLayerClipped", &CEntity::GameLayerClipped)
		.endClass()

		.deriveClass<CEntity, CCharacter>("CCharacter")
			.addFunction("IsGrounded", &CCharacter::IsGrounded)

			.addFunction("SetWeapon", &CCharacter::SetWeapon)
			.addFunction("HandleWeaponSwitch", &CCharacter::HandleWeaponSwitch)
			.addFunction("DoWeaponSwitch", &CCharacter::DoWeaponSwitch)

			.addFunction("HandleWeapons", &CCharacter::HandleWeapons)
			.addFunction("HandleNinja", &CCharacter::HandleNinja)

			.addFunction("OnPredictedInput", &CCharacter::OnPredictedInput)
			.addFunction("OnDirectInput", &CCharacter::OnDirectInput)
			.addFunction("ResetInput", &CCharacter::ResetInput)
			.addFunction("FireWeapon", &CCharacter::FireWeapon)

			.addFunction("Die", &CCharacter::Die)
			.addFunction("TakeDamage", &CCharacter::TakeDamage)

			.addFunction("Spawn", &CCharacter::Spawn)
			//.addFunction("Remove", &CCharacter::Remove)

			.addFunction("IncreaseHealth", &CCharacter::IncreaseHealth)
			.addFunction("IncreaseArmor", &CCharacter::IncreaseArmor)

			.addFunction("GiveWeapon", &CCharacter::GiveWeapon)
			.addFunction("GiveNinja", &CCharacter::GiveNinja)

			.addFunction("SetEmote", &CCharacter::SetEmote)

			.addFunction("IsAlive", &CCharacter::IsAlive)

			.addFunction("Player", &CCharacter::GetPlayer)
		.endClass()

	; // end global namespace
}
