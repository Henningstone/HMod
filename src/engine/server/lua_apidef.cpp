#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>

#include <game/server/gamecontroller.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>
#include <game/collision.h>

#include <game/generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/laser.h>
#include <game/server/entities/projectile.h>
#include <game/server/entities/flag.h>
#include <game/server/entities/lua_entity.h>

#include "luabinding.h"
#include "lua_class.h"
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

		.beginClass< CProjectileProperties>("CProjectileProperties")
			.addConstructor <void (*) (int, int, bool, float)> ()
			.addData("LifeSpan", &CProjectileProperties::LifeSpan)
			.addData("Damage", &CProjectileProperties::Damage)
			.addData("Explosive", &CProjectileProperties::Explosive)
			.addData("Force", &CProjectileProperties::Force)
		.endClass()

		.beginClass<CLuaClass>("CLuaClass")
			.addFunction("BindClass", &CLuaClass::LuaBindClass)
			.addFunction("GetSelf", &CLuaClass::GetSelf)
		.endClass()

		.beginClass<IGameController>("IGameController")
		.endClass()

		.beginClass<CCollision>("CCollision")
			.addProperty("MapWidth", &CCollision::GetWidth)
			.addProperty("MapHeight", &CCollision::GetHeight)

			.addFunction("Distance", &CCollision::Distance)
			.addFunction("Normalize", &CCollision::Normalize)
			.addFunction("ClosestPointOnLine", &CCollision::ClosestPointOnLine)

			.addFunction("CheckPoint", &CCollision::CheckPointLua)
			.addFunction("GetCollisionAt", &CCollision::GetCollisionAt)

			.addFunction("IntersectLine", &CCollision::IntersectLine)
			.addFunction("MovePoint", &CCollision::MovePoint)
			.addFunction("MoveBox", &CCollision::MoveBox)
			.addFunction("TestBox", &CCollision::TestBox)

			.addFunction("IsTileSolid", &CCollision::IsTileSolid)
			.addFunction("GetTile", &CCollision::GetTileRaw)
			.addFunction("GetTileRaw", &CCollision::GetTileRaw)
		.endClass()

		.beginClass<CTuningParams>("CTuningParams")
		.endClass()

		// Server.Game.GetWorld()
		.beginClass<CGameWorld>("CGameWorld")
			.addData("ResetRequested", &CGameWorld::m_ResetRequested)
			.addData("Paused", &CGameWorld::m_Paused)
			.addFunction("FindFirst", &CGameWorld::FindFirst)
			.addFunction("IntersectCharacter", &CGameWorld::IntersectCharacter)
			.addFunction("ClosestCharacter", &CGameWorld::ClosestCharacter)
			.addFunction("InsertEntity", &CGameWorld::InsertEntity)
			.addFunction("RemoveEntity", &CGameWorld::RemoveEntity)
			.addFunction("DestroyEntity", &CGameWorld::DestroyEntity)
			.addFunction("Snap", &CGameWorld::Snap)
			.addFunction("Tick", &CGameWorld::Tick)
		.endClass()

		// Srv.Game
		.beginClass<CGameContext>("CGameContext")
			.addProperty("Collision", &CGameContext::LuaGetCollision)
			.addProperty("Tuning", &CGameContext::LuaGetTuning)
			.addProperty("World", &CGameContext::LuaGetWorld)
			.addFunction("GetPlayer", &CGameContext::GetPlayer) // -> [CPlayer]
			.addFunction("GetPlayerChar", &CGameContext::GetPlayerChar) // [CCharacter]

			.addFunction("StartVote", &CGameContext::StartVote)
			.addFunction("EndVote", &CGameContext::EndVote)
			.addFunction("SendVoteSet", &CGameContext::SendVoteSet)
			.addFunction("SendVoteStatus", &CGameContext::SendVoteStatus)
			.addFunction("AbortVoteKickOnDisconnect", &CGameContext::AbortVoteKickOnDisconnect)

			.addFunction("CreateDamageInd", &CGameContext::CreateDamageInd)
			.addFunction("CreateSound", &CGameContext::CreateSound)
			.addFunction("CreateDamageInd", &CGameContext::CreateDamageInd)
			.addFunction("CreateExplosion", &CGameContext::CreateExplosion)
			.addFunction("CreateHammerHit", &CGameContext::CreateHammerHit)
			.addFunction("CreatePlayerSpawn", &CGameContext::CreatePlayerSpawn)
			.addFunction("CreateDeath", &CGameContext::CreateDeath)
			.addFunction("CreateSound", &CGameContext::CreateSound)
			.addFunction("CreateSoundGlobal", &CGameContext::CreateSoundGlobal)

			.addFunction("SendChatTarget", &CGameContext::SendChatTarget)
			.addFunction("SendChat", &CGameContext::SendChat)
			.addFunction("SendEmoticon", &CGameContext::SendEmoticon)
			.addFunction("SendWeaponPickup", &CGameContext::SendWeaponPickup)
			.addFunction("SendBroadcast", &CGameContext::SendBroadcast)

			.addFunction("SwapTeams", &CGameContext::SwapTeams)

			.addFunction("IsClientReady", &CGameContext::IsClientReady)
			.addFunction("IsClientPlayer", &CGameContext::IsClientPlayer)
			
			.addFunction("CreateEntityCharacter", &CGameContext::CreateEntityCharacter)
			.addFunction("CreateEntityFlag", &CGameContext::CreateEntityFlag)
			.addFunction("CreateEntityLaser", &CGameContext::CreateEntityLaser)
			.addFunction("CreateEntityPickup", &CGameContext::CreateEntityPickup)
			.addFunction("CreateEntityProjectile", &CGameContext::CreateEntityProjectile)
			.addFunction("CreateEntityCustom", &CGameContext::CreateEntityCustom)

			.addFunction("GameType", &CGameContext::GameType)
			.addFunction("Version", &CGameContext::Version)
			.addFunction("NetVersion", &CGameContext::NetVersion)
		.endClass()

		/// Srv.Server
		.beginClass<IServer>("IServer")
			.addProperty("Tick", &IServer::Tick)
			.addProperty("TickSpeed", &IServer::TickSpeed)

			.addProperty("MaxClients", &IServer::MaxClients)
			.addFunction("ClientIngame", &IServer::ClientIngame)
			.addFunction("GetClientInfo", &IServer::GetClientInfo)
			.addFunction("GetClientAddr", &IServer::GetClientAddr)

			.addFunction("GetClientName", &IServer::ClientName)
			.addFunction("GetClientClan", &IServer::ClientClan)
			.addFunction("GetClientCountry", &IServer::ClientCountry)

			.addFunction("SetClientName", &IServer::SetClientName)
			.addFunction("SetClientClan", &IServer::SetClientClan)
			.addFunction("SetClientCountry", &IServer::SetClientCountry)
			.addFunction("SetClientScore", &IServer::SetClientScore)

			.addFunction("SetRconCID", &IServer::SetRconCID)
			.addFunction("IsAuthed", &IServer::IsAuthed)
			.addFunction("Kick", &IServer::Kick)
		.endClass()

		/// [CCharacterCore].Input
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

		/// Srv.Game:GetPlayer(CID)
		.deriveClass<CPlayer, CLuaClass>("CPlayer")
			.addFunction("TryRespawn", &CPlayer::TryRespawn)
			.addFunction("Respawn", &CPlayer::Respawn)
			.addFunction("GetTeam", &CPlayer::GetTeam)
			.addFunction("SetTeam", &CPlayer::SetTeam)
			//.addProperty("Team", &CPlayer::GetTeam, &CPlayer::SetTeam)
			.addFunction("GetCID", &CPlayer::GetCID)

			.addFunction("Tick", &CPlayer::Tick)
			.addFunction("PostTick", &CPlayer::PostTick)
			.addFunction("Snap", &CPlayer::Snap)

			.addFunction("OnDirectInput", &CPlayer::OnDirectInput)
			.addFunction("OnPredictedInput", &CPlayer::OnPredictedInput)
			.addFunction("OnDisconnect", &CPlayer::OnDisconnect)

			.addFunction("KillCharacter", &CPlayer::KillCharacter)
			.addFunction("GetCharacter", &CPlayer::GetCharacter) // -> [CCharacter]

			.addData("ViewPos", &CPlayer::m_ViewPos)
			.addData("PlayerFlags", &CPlayer::m_PlayerFlags)
			.addData("SpectatorID", &CPlayer::m_SpectatorID)
			.addData("IsReady", &CPlayer::m_IsReady)
			.addFunction("GetActLatency", &CPlayer::GetActLatency)
			.addData("SpectatorID", &CPlayer::m_SpectatorID)
			.addData("IsReady", &CPlayer::m_IsReady)

			.addData("Vote", &CPlayer::m_Vote)
			.addData("VotePos", &CPlayer::m_VotePos)

			.addData("LastVoteCall", &CPlayer::m_LastVoteCall)
			.addData("LastVoteTry", &CPlayer::m_LastVoteTry)
			.addData("LastChat", &CPlayer::m_LastChat)
			.addData("LastSetTeam", &CPlayer::m_LastSetTeam)
			.addData("LastSetSpectatorMode", &CPlayer::m_LastSetSpectatorMode)
			.addData("LastChangeInfo", &CPlayer::m_LastChangeInfo)
			.addData("LastEmote", &CPlayer::m_LastEmote)
			.addData("LastKill", &CPlayer::m_LastKill)

			// TODO add struct m_TeeInfos

			.addData("RespawnTick", &CPlayer::m_RespawnTick)
			.addData("DieTick", &CPlayer::m_DieTick)
			.addData("Score", &CPlayer::m_Score)
			.addData("ScoreStartTick", &CPlayer::m_ScoreStartTick)
			.addData("ForceBalanced", &CPlayer::m_ForceBalanced)
			.addData("LastActionTick", &CPlayer::m_LastActionTick)
			.addData("TeamChangeTick", &CPlayer::m_TeamChangeTick)

			// TODO add struct m_LatestActivity

			// TODO add struct m_Latency
		.endClass()

		/// [CCharacter].Core
		.beginClass<CCharacterCore>("CCharacterCore")
			.addData("Pos", &CCharacterCore::m_Pos)
			.addData("Vel", &CCharacterCore::m_Vel)

			.addData("HookPos", &CCharacterCore::m_HookPos)
			.addData("HookDir", &CCharacterCore::m_HookDir)
			.addData("HookTick", &CCharacterCore::m_HookTick)
			.addData("HookState", &CCharacterCore::m_HookState)
			.addData("HookedPlayer", &CCharacterCore::m_HookedPlayer)

			.addData("Jumped", &CCharacterCore::m_Jumped)

			.addData("Direction", &CCharacterCore::m_Direction)
			.addData("Angle", &CCharacterCore::m_Angle)
			.addData("Input", &CCharacterCore::m_Input) // -> [CNetObj_PlayerInput]

			.addData("TriggeredEvents", &CCharacterCore::m_TriggeredEvents)

			.addFunction("Init", &CCharacterCore::Init)
			.addFunction("Reset", &CCharacterCore::Reset)
			.addFunction("Tick", &CCharacterCore::Tick)
			.addFunction("Move", &CCharacterCore::Move)

			.addFunction("Read", &CCharacterCore::Read)
			.addFunction("Write", &CCharacterCore::Write)
			.addFunction("Quantize", &CCharacterCore::Quantize)
		.endClass()

		.deriveClass<CEntity, CLuaClass>("CEntity")
			//.addConstructor<void (*) (CGameWorld*, int, const char*) > ()

			.addData("ProximityRadius", &CEntity::m_ProximityRadius)
			.addData("Pos", &CEntity::m_Pos)

			.addFunction("GameWorld", &CEntity::GameWorld)
			.addFunction("GameServer", &CEntity::GameServer)
			.addFunction("Server", &CEntity::Server)

			.addFunction("TypeNext", &CEntity::TypeNext)
			.addFunction("TypePrev", &CEntity::TypePrev)

			.addProperty("Type", &CEntity::GetType)

			.addFunction("Destroy", &CEntity::Destroy)

			.addFunction("Reset", &CEntity::Reset)
			.addFunction("Tick", &CEntity::Tick)
			.addFunction("TickDefered", &CEntity::TickDefered)
			.addFunction("TickPaused", &CEntity::TickPaused)
			.addFunction("Snap", &CEntity::Snap)

			.addFunction("NetworkClipped", &CEntity::NetworkClippedLua)
			.addFunction("GameLayerClipped", &CEntity::GameLayerClipped)
		.endClass()

		/// [Player].GetCharacter()
		/// Srv.Game:GetPlayerChar(CID)
		.deriveClass<CCharacter, CEntity>("CCharacter")
			.addFunction("IsGrounded", &CCharacter::IsGrounded)

			.addFunction("SetWeapon", &CCharacter::SetWeapon)
			.addFunction("HandleWeaponSwitch", &CCharacter::HandleWeaponSwitch)
			.addFunction("DoWeaponSwitch", &CCharacter::DoWeaponSwitch)

			.addFunction("HandleWeapons", &CCharacter::HandleWeapons)
			.addFunction("HandleNinja", &CCharacter::HandleNinja)

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

			.addProperty("IsAlive", &CCharacter::IsAlive)

			.addFunction("GetPlayer", &CCharacter::GetPlayer) // -> [CPlayer]
			.addFunction("GetCore", &CCharacter::GetCore) // -> [CCharacterCore]
			.addProperty("Core", &CCharacter::GetCore)
		.endClass()

		.deriveClass<CPickup, CEntity>("CPickup")
			.addProperty("Type", &CPickup::GetPickupType, &CPickup::SetPickupType)
			.addProperty("Subtype", &CPickup::GetSubtype, &CPickup::SetSubtype)
			.addProperty("SpawnTick", &CPickup::GetSpawnTick, &CPickup::SetSpawnTick)
		.endClass()

		.deriveClass<CFlag, CEntity>("CFlag")
			//.addData("ms_PhysSize", CFlag::ms_PhysSize, false)
			.addData("CarryingCharacter", &CFlag::m_pCarryingCharacter)
			.addData("StandPos", &CFlag::m_StandPos)
			.addData("Vel", &CFlag::m_Vel)
			.addData("Team", &CFlag::m_Team)
			.addData("AtStand", &CFlag::m_AtStand)
			.addData("DropTick", &CFlag::m_DropTick)
			.addData("GrabTick", &CFlag::m_GrabTick)
		.endClass()

		.deriveClass<CProjectile, CEntity>("CProjectile")
			.addFunction("GetPos", &CProjectile::GetPos)

			.addProperty("Direction", &CProjectile::GetDirection, &CProjectile::SetDirection)
			.addProperty("LifeSpan", &CProjectile::GetLifeSpan, &CProjectile::SetLifeSpan)
			.addProperty("Owner", &CProjectile::GetOwner, &CProjectile::SetOwner)
			.addProperty("Type", &CProjectile::GetProjectileType, &CProjectile::SetProjectileType)
			.addProperty("Damage", &CProjectile::GetDamage, &CProjectile::SetDamage)
			.addProperty("SoundImpact", &CProjectile::GetSoundImpact, &CProjectile::SetSoundImpact)
			.addProperty("Weapon", &CProjectile::GetWeapon, &CProjectile::SetWeapon)
			.addProperty("Force", &CProjectile::GetForce, &CProjectile::SetForce)
			.addProperty("StartTick", &CProjectile::GetStartTick, &CProjectile::SetStartTick)
			.addProperty("Explosive", &CProjectile::GetExplosive, &CProjectile::SetExplosive)
		.endClass()

		.deriveClass<CLaser, CEntity>("CLaser")
			.addProperty("To", &CLaser::GetTo, &CLaser::SetTo)
			.addProperty("From", &CLaser::GetFrom, &CLaser::SetFrom) // Is the same as CEntity::m_Pos!
			.addProperty("Dir", &CLaser::GetDir, &CLaser::SetDir)
			.addProperty("Energy", &CLaser::GetEnergy, &CLaser::SetEnergy)
			.addProperty("Bounces", &CLaser::GetBounces, &CLaser::SetBounces)
			.addProperty("EvalTick", &CLaser::GetEvalTick, &CLaser::SetEvalTick)
			.addProperty("Owner", &CLaser::GetOwner, &CLaser::SetOwner)
		.endClass()

		.deriveClass<CLuaEntity, CEntity>("CLuaEntity")
		.endClass()

		.beginClass<IConsole::IResult>("CConsole_IResult")
			.addFunction("GetInteger", &IConsole::IResult::GetInteger)
			.addFunction("GetFloat", &IConsole::IResult::GetFloat)
			.addFunction("GetString", &IConsole::IResult::GetString)
			.addFunction("NumArguments", &IConsole::IResult::NumArguments)
		.endClass()

		// Server.Console
		.beginClass<IConsole>("IConsole")
			.addFunction("Print", &IConsole::Print)
			.addFunction("LineIsValid", &IConsole::LineIsValid)
			.addFunction("Register", &IConsole::RegisterLua)
			.addFunction("ExecuteLine", &IConsole::ExecuteLine)
		.endClass()


		.beginNamespace("Srv")
			.addVariable("Console", &CLua::ms_pSelf->m_pConsole, false)
			.addVariable("Lua", &CLua::ms_pSelf, false)
			.addVariable("Game", &CLua::ms_pSelf->m_pGameServer, false)
			.addVariable("Server", &CLua::ms_pSelf->m_pServer, false)
		.endNamespace()

	; // end global namespace
}
