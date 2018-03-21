/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_LASER_H
#define GAME_SERVER_ENTITIES_LASER_H

#include <game/server/entity.h>

class CLaser : public CEntity
{
public:
	CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	// for lua
	vec2 GetFrom() const { return m_From; }
	void SetFrom(vec2 Pos) { m_From = Pos; }
	vec2 GetTo() const { return m_Pos; }
	void SetTo(vec2 Pos) { m_Pos = Pos; }
	vec2 GetDir() const { return m_Dir; }
	void SetDir(vec2 Dir) { m_Dir = Dir; }
	float GetEnergy() const { return m_Energy; }
	void SetEnergy(float Energy) { m_Energy = Energy; }
	int GetBounces() const { return m_Bounces; }
	void SetBounces(int Bounces) { m_Bounces = Bounces; }
	int GetEvalTick() const { return m_EvalTick; }
	void SetEvalTick(int EvalTick) { m_EvalTick = EvalTick; }
	int GetOwner() const { return m_Owner; }
	void SetOwner(int Owner) { m_Owner = Owner; }

protected:
	bool HitCharacter(vec2 From, vec2 To);
	void DoBounce();

private:
	vec2 m_From;
	vec2 m_Dir;
	float m_Energy;
	int m_Bounces;
	int m_EvalTick;
	int m_Owner;
};

#endif
