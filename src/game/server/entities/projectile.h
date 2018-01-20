/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

#include <game/server/entity.h>

class CProjectileProperties
{
public:
	CProjectileProperties(int _LifeSpan, int _Damage, bool _Explosive, float _Force) : LifeSpan(_LifeSpan), Damage(_Damage), Explosive(_Explosive), Force(_Force){}
	
	int LifeSpan;
	int Damage;
	bool Explosive;
	float Force;
};

class CProjectile : public CEntity
{
public:
	CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon);

	vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	
	vec2 GetDirection() const { return m_Direction; }
	void SetDirection(vec2 Dir) { m_Direction = Dir; }
	int GetLifeSpan() const { return m_LifeSpan; }
	void SetLifeSpan(int LifeSpan) { m_LifeSpan = LifeSpan; }
	int GetOwner() const { return m_Owner; }
	void SetOwner(int Owner) { m_Owner = Owner; }
	int GetType() const { return m_Type; }
	void SetType(int Type) { m_Type = Type; }
	int GetDamage() const { return m_Damage; }
	void SetDamage(int Damage) { m_Damage = Damage; }
	int GetSoundImpact() const { return m_SoundImpact; }
	void SetSoundImpact(int SoundImpact) { m_SoundImpact = SoundImpact; }
	int GetWeapon() const { return m_Weapon; }
	void SetWeapon(int Weapon) { m_Weapon = Weapon; }
	float GetForce() const { return m_Force; }
	void SetForce(float Force) { m_Force = Force; }
	int GetStartTick() const { return m_StartTick; }
	void SetStartTick(int StartTick) { m_StartTick = StartTick; }
	bool GetExplosive() const { return m_Explosive; }
	void SetExplosive(bool Explosive) { m_Explosive = Explosive; }

private:
	vec2 m_Direction;
	int m_LifeSpan;
	int m_Owner;
	int m_Type;
	int m_Damage;
	int m_SoundImpact;
	int m_Weapon;
	float m_Force;
	int m_StartTick;
	bool m_Explosive;
};

#endif
