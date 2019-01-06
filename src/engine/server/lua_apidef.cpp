#include <engine/lua_include.h>
#include <engine/storage.h>

#include <game/gamecore.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>
#include <game/collision.h>
#include <game/voting.h>

#include <game/generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/laser.h>
#include <game/server/entities/projectile.h>
#include <game/server/entities/flag.h>
#include <game/server/entities/lua_entity.h>

#include "luabinding.h"
#include "lua/lua_config.h"
#include "lua/luajson.h"
#include "engine/server/lua/luasqlite.h"
#include "lua_class.h"
#include "lua.h"

#include <engine/server/server.h>


void CLua::RegisterLuaCallbacks()
{
	using namespace luabridge;
	lua_State *L = m_pLuaState;

	lua_register(L, "print", CLuaBinding::Print);
	lua_register(L, "throw", CLuaBinding::Throw); // adds an exception, but doesn't jump out like 'error' does
	lua_register(L, "_io_open", CLuaBinding::LuaIO_Open);
	lua_register(L, "GetScriptName", CLuaBinding::ScriptName);
	lua_register(L, "Listdir", CLuaBinding::LuaListdir);

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

		// json
		.beginClass< CJsonValue >("JsonValue")
			//.addFunction("__tostring", &CJsonValue::ToString) TODO: serialize
			//.addFunction("__tonumber", &CJsonValue::ToNumber)
			.addFunction("Destroy", &CJsonValue::Destroy)
			.addFunction("GetType", &CJsonValue::GetType)
			.addFunction("ToString", &CJsonValue::ToString)
			.addFunction("ToNumber", &CJsonValue::ToNumber)
			.addFunction("ToBoolean", &CJsonValue::ToBoolean)
			.addFunction("ToTable", &CJsonValue::ToTable)
			.addFunction("ToObject", &CJsonValue::ToObject)
		.endClass()

		.beginClass< CLuaJson >("CLuaJson")
		.endClass()

		.beginNamespace("json")
			.addFunction("Parse", &CLuaJson::Parse)
			.addFunction("Convert", &CLuaJson::Convert)
			.addFunction("Serialize", &CLuaJson::Serialize)
			.addFunction("Read", &CLuaJson::Read)
		.endNamespace()

		// sql
		.beginClass< CQuery >("CQuery")
			.addFunction("GetColumnCount", &CQuery::GetColumnCount)
			.addFunction("GetName", &CQuery::GetName)
			.addFunction("GetType", &CQuery::GetType)
			.addFunction("GetID", &CQuery::GetID)

			.addFunction("GetInt", &CQuery::GetInt)
			.addFunction("GetFloat", &CQuery::GetFloat)
			.addFunction("GetText", &CQuery::GetText)
			.addFunction("GetStr", &CQuery::GetText) // alias
			.addFunction("GetBlob", &CQuery::GetBlob)
			.addFunction("GetSize", &CQuery::GetSize)

			.addFunction("GetIntN", &CQuery::GetIntN)
			.addFunction("GetFloatN", &CQuery::GetFloatN)
			.addFunction("GetTextN", &CQuery::GetTextN)
			.addFunction("GetStrN", &CQuery::GetTextN) // alias
			.addFunction("GetBlobN", &CQuery::GetBlobN)
			.addFunction("GetSizeN", &CQuery::GetSizeN)
		.endClass()

		.beginClass< CLuaSqlite >("CLuaSqlite")
			//.addConstructor <void (*) (const char *)> ()
			.addFunction("Execute", &CLuaSqlite::Execute)
			.addFunction("Flush", &CLuaSqlite::Flush)
			.addFunction("Work", &CLuaSqlite::Work)
			.addFunction("Clear", &CLuaSqlite::Clear)
			.addFunction("GetDatabasePath", &CLuaSqlite::GetDatabasePath)
		.endClass()

		.beginClass< CLuaSql >("CLuaSql")
		.endClass()

		.beginNamespace("sql")
			.addFunction("Open", &CLuaSql::Open)
			.addFunction("Flush", &CLuaSql::Flush)
			.addFunction("Clear", &CLuaSql::Clear)
		.endNamespace()


		.beginClass< CProjectileProperties>("CProjectileProperties")
			.addConstructor <void (*) (int, int, bool, float)> ()
			.addData("LifeSpan", &CProjectileProperties::LifeSpan)
			.addData("Damage", &CProjectileProperties::Damage)
			.addData("Explosive", &CProjectileProperties::Explosive)
			.addData("Force", &CProjectileProperties::Force)
		.endClass()

		/// [CGameContext].VoteOption{First,Last}
		.beginClass<CVoteOptionServer>("CVoteOptionServer")
			.addData("Next", &CVoteOptionServer::m_pNext, false)
			.addData("Prev", &CVoteOptionServer::m_pPrev, false)
			.addData("Description", &CVoteOptionServer::m_pDescription, false)
			.addData("Command", &CVoteOptionServer::m_pCommand, false)
		.endClass()

		.beginClass<CLuaClass>("CLuaClass")
			.addFunction("BindClass", &CLuaClass::LuaBindClass)
			.addFunction("GetSelf", &CLuaClass::GetSelf)
		.endClass()

		.beginClass<IGameController>("IGameController")
		.endClass()

		/// Srv.Game.Collision
		.beginClass<CCollision>("CCollision")
			.addProperty("MapWidth", &CCollision::GetWidth)
			.addProperty("MapHeight", &CCollision::GetHeight)

			.addFunction("Distance", &CCollision::Distance)
			.addFunction("Normalize", &CCollision::Normalize)
			.addFunction("Rotate", &CCollision::Rotate)
			.addFunction("ClosestPointOnLine", &CCollision::ClosestPointOnLine)

			.addFunction("CheckPoint", &CCollision::CheckPointLua)
			.addFunction("GetCollisionAt", &CCollision::GetCollisionAt)

			.addFunction("IntersectLine", &CCollision::IntersectLine)
			.addFunction("MovePoint", &CCollision::MovePoint)
			.addFunction("MoveBox", &CCollision::MoveBox)
			.addFunction("TestBox", &CCollision::TestBox)

			.addFunction("IsTileSolid", &CCollision::IsTileSolid)
			.addFunction("GetTile", &CCollision::GetTile)
			.addFunction("GetTileRaw", &CCollision::GetTileRaw)
		.endClass()

		/// Srv.Game.Tuning
#define MACRO_TUNING_PARAM(Name,ScriptName,Value) \
		.addProperty(#Name, &CTuningParams::GetTuneF_##Name, &CTuningParams::SetTuneF_##Name) \
		.addProperty(#ScriptName, &CTuningParams::GetTuneF_##Name, &CTuningParams::SetTuneF_##Name)\
		.addProperty(("_"#Name), &CTuningParams::GetTuneI_##Name, &CTuningParams::SetTuneI_##Name)\
		.addProperty(("_"#ScriptName), &CTuningParams::GetTuneI_##Name, &CTuningParams::SetTuneI_##Name)

		.beginClass<CTuningParams>("CTuningParams")
			.addFunction("Copy", &CTuningParams::LuaCopy)
			#include <game/tuning.h>
		.endClass()
#undef MACRO_TUNING_PARAM

		/// Srv.Game.World
		.beginClass<CGameWorld>("CGameWorld")
			.addData("ResetRequested", &CGameWorld::m_ResetRequested)
			.addData("Paused", &CGameWorld::m_Paused)
			.addFunction("FindFirst", &CGameWorld::FindFirst)
			.addFunction("cast_Character", &CGameWorld::cast_CCharacter)
			.addFunction("cast_Flag", &CGameWorld::cast_CFlag)
			.addFunction("cast_Laser", &CGameWorld::cast_CLaser)
			.addFunction("cast_Pickup", &CGameWorld::cast_CPickup)
			.addFunction("cast_Projectile", &CGameWorld::cast_CProjectile)
			.addFunction("IntersectCharacter", &CGameWorld::IntersectCharacter)
			.addFunction("ClosestCharacter", &CGameWorld::ClosestCharacter)
			.addFunction("InsertEntity", &CGameWorld::InsertEntity) // inserts an entity into the world
			.addFunction("RemoveEntity", &CGameWorld::RemoveEntity) // remove the reference from the world
			.addFunction("DestroyEntity", &CGameWorld::DestroyEntity) // marks the entity for deletion
			.addFunction("Snap", &CGameWorld::Snap)
			.addFunction("Tick", &CGameWorld::Tick)
		.endClass()

		/// Srv.Game
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
			.addFunction("CreateExplosion", &CGameContext::CreateExplosionLua)
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

			.addData("VoteOptionFirst", &CGameContext::m_pVoteOptionFirst, false)
			.addData("VoteOptionLast", &CGameContext::m_pVoteOptionLast, false)
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
			.addFunction("SetClientAccessLevel", &IServer::SetClientAccessLevel)

			.addFunction("SetRconCID", &IServer::SetRconCID)
			.addFunction("IsAuthed", &IServer::IsAuthed)
			.addFunction("HasAccess", &IServer::HasAccess)
			.addFunction("Kick", &IServer::Kick)

			.addFunction("SendRconLine", &IServer::SendRconLine)

			.addFunction("IDTranslate", &IServer::IDTranslateLua)
			.addFunction("IDTranslateReverse", &IServer::IDTranslateReverseLua)
		.endClass()

		.deriveClass<CServer, IServer>("CServer")
		.endClass()

		/// [CCharacterCore].Input
		.beginClass<CNetObj_PlayerInput>("CNetObj_PlayerInput")
			.addConstructor <void (*) ()> ()
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

		// [CPlayer].TeeInfos
		.beginClass<CPlayer::CTeeInfos>("CPlayer_CTeeInfos")
			.addProperty("SkinName", &CPlayer::CTeeInfos::GetSkinName, &CPlayer::CTeeInfos::SetSkinName)
			.addData("UseCustomColor", &CPlayer::CTeeInfos::m_UseCustomColor)
			.addData("ColorBody", &CPlayer::CTeeInfos::m_ColorBody)
			.addData("ColorFeet", &CPlayer::CTeeInfos::m_ColorFeet)
		.endClass()

		/// Srv.Game:GetPlayer(CID)
		/// [CCharacter].GetPlayer()
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

			.addData("TeeInfos", &CPlayer::m_TeeInfos)

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

			.addFunction("Copy", &CCharacterCore::copy)
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

			// events; use them only in the respective event-callback in lua
//			.addFunction("Destroy", &CEntity::Destroy) // <- DO NOT BIND THIS!
			.addFunction("Reset", &CEntity::Reset)
			.addFunction("Tick", &CEntity::Tick)
			.addFunction("TickDefered", &CEntity::TickDefered)
			.addFunction("TickPaused", &CEntity::TickPaused)
			.addFunction("Snap", &CEntity::Snap)

			.addFunction("NetworkClipped", &CEntity::NetworkClippedLua)
			.addFunction("GameLayerClipped", &CEntity::GameLayerClipped)
		.endClass()

		/// [CCharacter]:WeaponSlot(ID)
		.beginClass<CCharacter::WeaponStat>("CCharacter_WeaponStat")
			.addData("AmmoRegenStart", &CCharacter::WeaponStat::m_AmmoRegenStart)
			.addData("Ammo", &CCharacter::WeaponStat::m_Ammo)
			.addData("Ammocost", &CCharacter::WeaponStat::m_Ammocost)
			.addData("WeaponId", &CCharacter::WeaponStat::m_WeaponId)
			.addData("Got", &CCharacter::WeaponStat::m_Got)
			.addData("FullAuto", &CCharacter::WeaponStat::m_FullAuto)
		.endClass()

		/// [CPlayer]:GetCharacter()
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
			.addFunction("IncreaseHealthEndless", &CCharacter::IncreaseHealthEndless)
			.addFunction("IncreaseArmor", &CCharacter::IncreaseArmor)
			.addFunction("IncreaseArmorEndless", &CCharacter::IncreaseArmorEndless)

			.addFunction("GiveWeapon", &CCharacter::GiveWeapon)
			.addFunction("GiveNinja", &CCharacter::GiveNinja)
			.addFunction("GiveWeaponSlot", &CCharacter::GiveWeaponSlot)
			.addFunction("SetWeaponAutoFire", &CCharacter::SetWeaponAutoFire)
			.addFunction("WeaponSlot", &CCharacter::WeaponSlot)
			.addFunction("ActiveWeaponSlot", &CCharacter::GetActiveWeaponSlot)

			.addData("AttackTick", &CCharacter::m_AttackTick)
			.addData("ReloadTimer", &CCharacter::m_ReloadTimer)

			.addData("AttackTick", &CCharacter::m_AttackTick)
			.addData("ReloadTimer", &CCharacter::m_ReloadTimer)

			.addFunction("SetEmote", &CCharacter::SetEmote)

			.addProperty("IsAlive", &CCharacter::IsAlive)

			.addFunction("GetPlayer", &CCharacter::GetPlayer) // -> [CPlayer]

			.addData("PhysicsEnabled", &CCharacter::m_PhysicsEnabled)
			.addFunction("GetCore", &CCharacter::GetCore) // -> [CCharacterCore]
			.addProperty("Core", &CCharacter::GetCore)
			.addProperty("Tuning", &CCharacter::LuaGetTuning, &CCharacter::LuaWriteTuning)

			.addData("Health", &CCharacter::m_Health)
			.addData("Armor", &CCharacter::m_Armor)
//			.addData("Jumped", &CCharacter::m_Jumped)
			.addData("ExtraJumps", &CCharacter::m_ExtraJumps)

			.addFunction("Freeze", &CCharacter::Freeze)
			.addFunction("UnFreeze", &CCharacter::UnFreeze)
			.addFunction("DeepFreeze", &CCharacter::DeepFreeze)
			.addFunction("UnDeep", &CCharacter::UnDeep)
			.addFunction("IsFrozen", &CCharacter::IsFrozen)
			.addFunction("IsDeep", &CCharacter::IsDeep)
			.addFunction("SetEndlessHook", &CCharacter::SetEndlessHook)
			.addFunction("SetSuperJump", &CCharacter::SetSuperJump)
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

		/// Srv.Storage
		.beginClass<IStorage>("IStorage")
			.addFunction("RemoveFile", &IStorage::RemoveFileLua)
			.addFunction("RenameFile", &IStorage::RenameFileLua)
			.addFunction("CreateFolder", &IStorage::CreateFolderLua)
			.addFunction("GetFullPath", &IStorage::GetFullPathLua)
		.endClass()

		.beginClass<IConsole::IResult>("IConsole_IResult")
			.addFunction("GetInteger", &IConsole::IResult::GetInteger)
			.addFunction("GetFloat", &IConsole::IResult::GetFloat)
			.addFunction("GetString", &IConsole::IResult::GetString)
			.addFunction("OptInteger", &IConsole::IResult::OptInteger)
			.addFunction("OptFloat", &IConsole::IResult::OptFloat)
			.addFunction("OptString", &IConsole::IResult::OptString)
			.addFunction("NumArguments", &IConsole::IResult::NumArguments)
			.addFunction("GetCID", &IConsole::IResult::GetCID)
		.endClass()

		// Srv.Console
		.beginClass<IConsole>("IConsole")
			.addFunction("Print", &IConsole::PrintLua)
			.addFunction("LineIsValid", &IConsole::LineIsValid)
			.addFunction("Register", &IConsole::RegisterLua)
			.addFunction("SetCommandAccessLevel", &IConsole::SetCommandAccessLevel) // set to > 0xFFFF to give *everyone* access by default
			//.addFunction("ExecuteLine", &IConsole::ExecuteLine) // peeps pls no use dis hacky shit
		.endClass()


		.beginNamespace("Srv")
			.addVariable("Console", &CLua::ms_pSelf->m_pConsole, false)
			.addVariable("Lua", &CLua::ms_pSelf, false)
			.addVariable("Game", &CLua::ms_pSelf->m_pGameServer, false)
			.addVariable("Server", &CLua::ms_pSelf->m_pServer, false)
			.addVariable("Storage", &CLua::ms_pSelf->m_pStorage, false)
		.endNamespace()

		/// Config.<var_name>
#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) \
			.addStaticProperty(#Name, &CConfigProperties::GetConfig_##Name) \
			.addStaticProperty(#ScriptName, &CConfigProperties::GetConfig_##Name)

#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) \
			.addStaticProperty(#Name, &CConfigProperties::GetConfig_##Name) \
			.addStaticProperty(#ScriptName, &CConfigProperties::GetConfig_##Name)

		.beginClass<CConfigProperties>("Config")
			#include <engine/shared/config_variables.h>
		.endClass()

#undef MACRO_CONFIG_STR
#undef MACRO_CONFIG_INT


	; // end global namespace
}
