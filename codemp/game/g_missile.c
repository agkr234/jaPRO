// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "w_saber.h"
#include "qcommon/q_shared.h"

#define	MISSILE_PRESTEP_TIME	50

extern void laserTrapStick( gentity_t *ent, vec3_t endpos, vec3_t normal );
extern void Jedi_Decloak( gentity_t *self );

extern qboolean FighterIsLanded( Vehicle_t *pVeh, playerState_t *parentPS );

/*
================
G_ReflectMissile

  Reflect the missile roughly back at it's owner
================
*/
float RandFloat(float min, float max);
void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward ) 
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	int		isowner = 0;

	if (missile->r.ownerNum == ent->s.number)
	{ //the original owner is bouncing the missile, so don't try to bounce it back at him
		isowner = 1;
	}

	//save the original speed
	speed = VectorNormalize( missile->s.pos.trDelta );

	if ( &g_entities[missile->r.ownerNum] && missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART && !isowner )
	{//bounce back at them if you can
		VectorSubtract( g_entities[missile->r.ownerNum].r.currentOrigin, missile->r.currentOrigin, bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else if (isowner)
	{ //in this case, actually push the missile away from me, and since we're giving boost to our own missile by pushing it, up the velocity
		vec3_t missile_dir;

		speed *= 1.5;

		VectorSubtract( missile->r.currentOrigin, ent->r.currentOrigin, missile_dir );
		VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else
	{
		vec3_t missile_dir;

		VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}

	if (g_tweakForce.integer & FT_FIX_PROJ_PUSH) { // feature from base_enhanced where missile reflects in direction you are a
		VectorCopy(forward, bounce_dir);
		for (i = 0; i < 3; i++)
			bounce_dir[i] += RandFloat(-0.1f, 0.1f); 
	} else { // original behaviour
		for (i = 0; i < 3; i++)
			bounce_dir[i] += RandFloat(-0.2f, 0.2f);
	}

	VectorNormalize( bounce_dir );
	VectorScale( bounce_dir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( missile->r.currentOrigin, missile->s.pos.trBase );
	if ( missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART )
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if ( missile->s.weapon == WP_ROCKET_LAUNCHER )
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}
}

void G_DeflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward ) 
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	vec3_t missile_dir;

	//save the original speed
	speed = VectorNormalize( missile->s.pos.trDelta );

	if (ent->client)
	{
		//VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		AngleVectors(ent->client->ps.viewangles, missile_dir, 0, 0);
		VectorCopy(missile_dir, bounce_dir);
		//VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else
	{
		VectorCopy(forward, bounce_dir);
		VectorNormalize(bounce_dir);
	}

	for ( i = 0; i < 3; i++ )
	{
		bounce_dir[i] += RandFloat( -1.0f, 1.0f );
	}

	VectorNormalize( bounce_dir );
	VectorScale( bounce_dir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( missile->r.currentOrigin, missile->s.pos.trBase );
	if ( missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART )
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if ( missile->s.weapon == WP_ROCKET_LAUNCHER )
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}
}

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if (ent->bounceCount != -5)
	{
		ent->bounceCount--;
	}

	if ( ent->flags & FL_BOUNCE_SHRAPNEL ) 
	{
		VectorScale( ent->s.pos.trDelta, 0.25f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_GRAVITY;

		// check for stop
		if ( trace->plane.normal[2] > 0.7 && ent->s.pos.trDelta[2] < 40 ) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			G_SetOrigin( ent, trace->endpos );
			ent->nextthink = level.time + 100;
			return;
		}
	}
	else if ( ent->flags & FL_BOUNCE_HALF ) 
	{
		if (ent->s.weapon == WP_REPEATER && g_tweakWeapons.integer & WT_ROCKET_MORTAR)
			VectorScale( ent->s.pos.trDelta, 0.4f, ent->s.pos.trDelta );
		else
			VectorScale( ent->s.pos.trDelta, 0.65f, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) 
		{
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	if (ent->s.weapon == WP_THERMAL)
	{ //slight hack for hit sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/thermal/bounce%i.wav", Q_irand(1, 2))));
	}
	else if (ent->s.weapon == WP_SABER)
	{
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/saber/bounce%i.wav", Q_irand(1, 3))));
	}
	else if (ent->s.weapon == G2_MODEL_PART)
	{
		//Limb bounce sound?
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	ent->takedamage = qfalse;
	// splash damage
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, 
				ent, ent->splashMethodOfDeath ) ) 
		{
			if (ent->parent)
			{
				g_entities[ent->parent->s.number].client->accuracy_hits++;
			}
			else if (ent->activator)
			{
				g_entities[ent->activator->s.number].client->accuracy_hits++;
			}
		}
	}

	trap->LinkEntity( (sharedEntity_t *)ent );
}

void G_RunStuckMissile( gentity_t *ent )
{
	if ( ent->takedamage )
	{
		if ( ent->s.groundEntityNum >= 0 && ent->s.groundEntityNum < ENTITYNUM_WORLD )
		{
			gentity_t *other = &g_entities[ent->s.groundEntityNum];

			if ( (!VectorCompare( vec3_origin, other->s.pos.trDelta ) && other->s.pos.trType != TR_STATIONARY) || 
				(!VectorCompare( vec3_origin, other->s.apos.trDelta ) && other->s.apos.trType != TR_STATIONARY) )
			{//thing I stuck to is moving or rotating now, kill me
				G_Damage( ent, other, other, NULL, NULL, 99999, 0, MOD_CRUSH );
				return;
			}
		}
	}
	// check think function
	G_RunThink( ent );
}

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}

/*
//-----------------------------------------------------------------------------
gentity_t *CreateMissile( vec3_t org, vec3_t dir, float vel, int life, 
							gentity_t *owner, qboolean altFire)
//-----------------------------------------------------------------------------
{
	gentity_t	*missile;

	missile = G_Spawn(qfalse);
	
	missile->nextthink = level.time + life;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	missile->parent = owner;
	missile->r.ownerNum = owner->s.number;

	//japro - do this so clients can know who the missile belongs to.. so they can hide it if its from another dimension
	missile->s.owner = owner->s.number;
	//

	if (altFire)
	{
		missile->s.eFlags |= EF_ALT_FIRING;
	}

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;	// NOTENOTE This is a Quake 3 addition over JK2
	missile->target_ent = NULL;

	if (owner->client && owner->client->sess.raceMode)
		missile->s.pos.trTime -= MISSILE_PRESTEP_TIME;//this be why rocketjump fucks up at high speed

	SnapVector(org);
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, vel, missile->s.pos.trDelta );
	VectorCopy( org, missile->r.currentOrigin);
	SnapVector(missile->s.pos.trDelta);

	return missile;
}

*/

//[JAPRO - Serverside - Weapons - Add missile inheritance function - Start]
//-----------------------------------------------------------------------------
gentity_t *CreateMissileNew( vec3_t org, vec3_t dir, float vel, int life, gentity_t *owner, qboolean altFire, qboolean inheritance, qboolean unlagged)
//-----------------------------------------------------------------------------
{
	gentity_t	*missile;
	float newVel = vel;
	vec3_t newDir;

	VectorCopy(dir, newDir);

	missile = G_Spawn(qfalse);

	missile->nextthink = level.time + life;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	missile->parent = owner;
	missile->r.ownerNum = owner->s.number;

	//japro - do this so clients can know who the missile belongs to.. so they can hide it if its from another dimension
	missile->s.owner = owner->s.number;
	//

	if (altFire)
		missile->s.eFlags |= EF_ALT_FIRING;

	missile->s.pos.trType = TR_LINEAR;

	if (owner->client && owner->client->sess.raceMode) {
		missile->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;//this be why rocketjump fucks up at high speed
	}
	else if (g_unlagged.integer & UNLAGGED_PROJ_NUDGE && owner->client) {
		int amount = owner->client->ps.ping * 0.9;

		if (amount > 135)
			amount = 135;
		else if (amount < 0) //dunno
			amount = 0;

		missile->s.pos.trTime = level.time - amount; //fixmer;
	}
	else {
		missile->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;	// NOTENOTE This is a Quake 3 addition over JK2 - do unlagged stuff here?
	}

	missile->target_ent = NULL;

	SnapVector(org);
	VectorCopy( org, missile->s.pos.trBase );

	if (inheritance && owner->client) {
		if (g_fullInheritance.integer) {
			VectorMA(newDir, g_projectileInheritance.value/vel, owner->client->ps.velocity, newDir);
		}
		else {
			newVel = newVel + DotProduct(newDir, owner->client->ps.velocity)*g_projectileInheritance.value;
		}
	}

	VectorScale( newDir, newVel, missile->s.pos.trDelta );
	VectorCopy( org, missile->r.currentOrigin);
	SnapVector(missile->s.pos.trDelta);

	return missile;
}
//[JAPRO - Serverside - Weapons - Add missile inheritance function - End]

void G_MissileBounceEffect( gentity_t *ent, vec3_t org, vec3_t dir )
{
	//FIXME: have an EV_BOUNCE_MISSILE event that checks the s.weapon and does the appropriate effect
	switch( ent->s.weapon )
	{
	case WP_BOWCASTER:
		G_PlayEffectID( G_EffectIndex("bowcaster/deflect"), ent->r.currentOrigin, dir );
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
		G_PlayEffectID( G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir );
		break;
	default:
		{
			gentity_t *te = G_TempEntity( org, EV_SABER_BLOCK );
			VectorCopy(org, te->s.origin);
			VectorCopy(dir, te->s.angles);
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum
		}
		break;
	}
}

/*
================
G_MissileImpact
================
*/
qboolean WP_CheckSaberDimension( gentity_t *self,  gentity_t *other);
void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
void WP_flechette_alt_blow( gentity_t *ent );
#if _GRAPPLE
void Weapon_HookThink (gentity_t *ent);
void Weapon_HookFree (gentity_t *ent);
#endif
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	qboolean		isKnockedSaber = qfalse;

	other = &g_entities[trace->entityNum];

	// check for bounce
	if ( other->takedamage &&
		(ent->bounceCount > 0 || ent->bounceCount == -5) &&
		( ent->flags & ( FL_BOUNCE | FL_BOUNCE_HALF ) ) &&
		(g_tweakWeapons.integer & WT_ROCKET_MORTAR && ent->s.weapon == WP_REPEATER && ent->bounceCount == 50 && ent->setTime && ent->setTime > level.time - 300)) 
	{ //if its a direct hit and first 500ms of mortar, bounce off player.
			G_BounceMissile( ent, trace );
			G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
			return;
	}
	else if ( !other->takedamage &&
		(ent->bounceCount > 0 || ent->bounceCount == -5) &&
		( ent->flags & ( FL_BOUNCE | FL_BOUNCE_HALF ) ) ) { //only on the first bounce vv
		if (!(g_tweakWeapons.integer & WT_ROCKET_MORTAR && ent->s.weapon == WP_REPEATER && ent->bounceCount == 50 && ent->setTime && ent->setTime < level.time - 1000))//give this mortar a 1 second 'fuse' until its armed
		{
			G_BounceMissile( ent, trace );
			G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
			return;
		}
	}
	else if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{ //this is a knocked-away saber
		if (ent->bounceCount > 0 || ent->bounceCount == -5)
		{
			G_BounceMissile( ent, trace );
			G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
			return;
		}

		isKnockedSaber = qtrue;
	}
	
	// I would glom onto the FL_BOUNCE code section above, but don't feel like risking breaking something else
	if ( (!other->takedamage && (ent->bounceCount > 0 || ent->bounceCount == -5) && ( ent->flags&(FL_BOUNCE_SHRAPNEL) ) ) || ((trace->surfaceFlags&SURF_FORCEFIELD)&&!ent->splashDamage&&!ent->splashRadius&&(ent->bounceCount > 0 || ent->bounceCount == -5)) ) 
	{
		G_BounceMissile( ent, trace );

		if ( ent->bounceCount < 1 )
		{
			ent->flags &= ~FL_BOUNCE_SHRAPNEL;
		}
		//trap->Print("Shrapnel is still there\n");
		return;
	}

	/*
	if ( !other->takedamage && ent->s.weapon == WP_THERMAL && !ent->alt_fire )
	{//rolling thermal det - FIXME: make this an eFlag like bounce & stick!!!
		//G_BounceRollMissile( ent, trace );
		if ( ent->owner && ent->owner->s.number == 0 ) 
		{
			G_MissileAddAlerts( ent );
		}
		//trap->linkentity( ent );
		return;
	}
	*/

	if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t *otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client && otherOwner->client->ps.duelInProgress &&
			otherOwner->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}
	else if (!isKnockedSaber)
	{
		if (other->takedamage && other->client && other->client->ps.duelInProgress &&
			other->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}

	if (other->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
	{
		if (ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_ROCKET &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_ROCKET_HOMING &&
			ent->methodOfDeath != MOD_THERMAL &&
			ent->methodOfDeath != MOD_THERMAL_SPLASH &&
			ent->methodOfDeath != MOD_TRIP_MINE_SPLASH &&
			ent->methodOfDeath != MOD_TIMED_MINE_SPLASH &&
			ent->methodOfDeath != MOD_DET_PACK_SPLASH &&
			ent->methodOfDeath != MOD_VEHICLE &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			ent->methodOfDeath != MOD_SABER &&
			ent->methodOfDeath != MOD_TURBLAST)
		{
			vec3_t fwd;

			if (trace)
			{
				VectorCopy(trace->plane.normal, fwd);
			}
			else
			{ //oh well
				AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
			}

			G_DeflectMissile(other, ent, fwd);
			G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd);
			return;
		}
	}

	if ((other->flags & FL_SHIELDED) &&
		ent->s.weapon != WP_ROCKET_LAUNCHER &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		ent->s.weapon != WP_DEMP2 &&
		ent->s.weapon != WP_EMPLACED_GUN &&
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH && 
		ent->methodOfDeath != MOD_TURBLAST &&
		ent->methodOfDeath != MOD_VEHICLE &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		!(ent->dflags&DAMAGE_HEAVY_WEAP_CLASS) )
	{
		vec3_t fwd;

		if (other->client)
		{
			AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		}
		else
		{
			AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
		}

		G_DeflectMissile(other, ent, fwd);
		G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd);
		return;
	}
		
	if (other->takedamage && other->client &&
		ent->s.weapon != WP_ROCKET_LAUNCHER &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		ent->s.weapon != WP_DEMP2 &&
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		other->client->ps.saberBlockTime < level.time &&
		!isKnockedSaber &&
		WP_SaberCanBlock(other, ent->r.currentOrigin, 0, 0, qtrue, 0)) //loda fixme, add check for dimensions for blocking here?
	{ //only block one projectile per 200ms (to prevent giant swarms of projectiles being blocked)
		vec3_t fwd;
		gentity_t *te;
		int otherDefLevel = other->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];

		te = G_TempEntity( ent->r.currentOrigin, EV_SABER_BLOCK );
		VectorCopy(ent->r.currentOrigin, te->s.origin);
		VectorCopy(trace->plane.normal, te->s.angles);
		te->s.eventParm = 0;
		te->s.weapon = 0;//saberNum
		te->s.legsAnim = 0;//bladeNum

		/*if (other->client->ps.velocity[2] > 0 ||
			other->client->pers.cmd.forwardmove ||
			other->client->pers.cmd.rightmove)
			*/
		if (other->client->ps.velocity[2] > 0 ||
			other->client->pers.cmd.forwardmove < 0) //now we only do it if jumping or running backward. Should be able to full-on charge.
		{
			otherDefLevel -= 1;
			if (otherDefLevel < 0)
			{
				otherDefLevel = 0;
			}
		}

		AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		if (otherDefLevel == FORCE_LEVEL_1)
		{
			//if def is only level 1, instead of deflecting the shot it should just die here
		}
		else if (otherDefLevel == FORCE_LEVEL_2)
		{
			G_DeflectMissile(other, ent, fwd);
		}
		else
		{
			G_ReflectMissile(other, ent, fwd);
		}
		other->client->ps.saberBlockTime = level.time + (350 - (otherDefLevel*100)); //200;

		//For jedi AI
		other->client->ps.saberEventFlags |= SEF_DEFLECTED;

		if (otherDefLevel == FORCE_LEVEL_3)
		{
			other->client->ps.saberBlockTime = 0; //^_^
		}

		if (otherDefLevel == FORCE_LEVEL_1)
		{
			goto killProj;
		}
		return;
	}
	else if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t *otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client &&
			ent->s.weapon != WP_ROCKET_LAUNCHER &&
			ent->s.weapon != WP_THERMAL &&
			ent->s.weapon != WP_TRIP_MINE &&
			ent->s.weapon != WP_DET_PACK &&
			ent->s.weapon != WP_DEMP2 &&
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_CONC &&
			(g_entities[ent->r.ownerNum].s.bolt1 == other->s.bolt1) &&//loda fixme, this stops missiles deflecting, but they still dont passthrough...
			ent->methodOfDeath != MOD_CONC_ALT /*&&
			otherOwner->client->ps.saberBlockTime < level.time*/)
		{ //for now still deflect even if saberBlockTime >= level.time because it hit the actual saber
			vec3_t fwd;
			gentity_t *te;
			int otherDefLevel = otherOwner->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];

			//in this case, deflect it even if we can't actually block it because it hit our saber
			//WP_SaberCanBlock(otherOwner, ent->r.currentOrigin, 0, 0, qtrue, 0);
			if (otherOwner->client && otherOwner->client->ps.weaponTime <= 0)
			{
				WP_SaberBlockNonRandom(otherOwner, ent->r.currentOrigin, qtrue);
			}

			te = G_TempEntity( ent->r.currentOrigin, EV_SABER_BLOCK );
			VectorCopy(ent->r.currentOrigin, te->s.origin);
			VectorCopy(trace->plane.normal, te->s.angles);
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum

			/*if (otherOwner->client->ps.velocity[2] > 0 ||
				otherOwner->client->pers.cmd.forwardmove ||
				otherOwner->client->pers.cmd.rightmove)*/
			if (otherOwner->client->ps.velocity[2] > 0 ||
				otherOwner->client->pers.cmd.forwardmove < 0) //now we only do it if jumping or running backward. Should be able to full-on charge.
			{
				otherDefLevel -= 1;
				if (otherDefLevel < 0)
				{
					otherDefLevel = 0;
				}
			}

			AngleVectors(otherOwner->client->ps.viewangles, fwd, NULL, NULL);

			if (otherDefLevel == FORCE_LEVEL_1)
			{
				//if def is only level 1, instead of deflecting the shot it should just die here
			}
			else if (otherDefLevel == FORCE_LEVEL_2)
			{
				G_DeflectMissile(otherOwner, ent, fwd);
			}
			else
			{
				G_ReflectMissile(otherOwner, ent, fwd);
			}
			otherOwner->client->ps.saberBlockTime = level.time + (350 - (otherDefLevel*100));//200;

			//For jedi AI
			otherOwner->client->ps.saberEventFlags |= SEF_DEFLECTED;

			if (otherDefLevel == FORCE_LEVEL_3)
			{
				otherOwner->client->ps.saberBlockTime = 0; //^_^
			}

			if (otherDefLevel == FORCE_LEVEL_1)
			{
				goto killProj;
			}
			return;
		}
	}

	// check for sticking
	if ( !other->takedamage && ( ent->s.eFlags & EF_MISSILE_STICK ) ) 
	{
		laserTrapStick( ent, trace->endpos, trace->plane.normal );
		G_AddEvent( ent, EV_MISSILE_STICK, 0 );
		return;
	}

//JAPRO - Serverside - Flag punting - Start
	if (g_allowFlagThrow.integer && !other->takedamage && other->s.eType == ET_ITEM)
	{
		vec3_t velocity;

		if (ent->s.weapon == WP_REPEATER && (ent->s.eFlags & EF_ALT_FIRING))
		{
			other->s.pos.trType = TR_GRAVITY;
			other->s.pos.trTime = level.time;
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			VectorScale( velocity, 0.7f, other->s.pos.trDelta );
			VectorCopy( other->r.currentOrigin, other->s.pos.trBase );
		}
		else if (ent->s.weapon == WP_ROCKET_LAUNCHER && (ent->s.eFlags & EF_ALT_FIRING))
		{
			other->s.pos.trType = TR_GRAVITY;
			other->s.pos.trTime = level.time;
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			VectorScale( velocity, 2.5f, other->s.pos.trDelta );
			VectorCopy( other->r.currentOrigin, other->s.pos.trBase );
		}
		else if (ent->s.weapon == WP_ROCKET_LAUNCHER)
		{
			other->s.pos.trType = TR_GRAVITY;
			other->s.pos.trTime = level.time;
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			VectorScale( velocity, 0.9f, other->s.pos.trDelta );
			VectorCopy( other->r.currentOrigin, other->s.pos.trBase );
		}
		else if (ent->s.weapon == WP_THERMAL)
		{
			other->s.pos.trType = TR_GRAVITY;
			other->s.pos.trTime = level.time;
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			VectorScale( velocity, 0.9f, other->s.pos.trDelta ); //tweak?
			VectorCopy( other->r.currentOrigin, other->s.pos.trBase );
		}
	}
//JAPRO - Serverside - Flag punting - End

	// impact damage
	if (other->takedamage && !isKnockedSaber) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;
			qboolean didDmg = qfalse;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}

			//damage falloff option, assumes bullet lifetime is 10,000 (default)
			if ((g_tweakWeapons.integer & WT_TRIBES) &&
				((ent->s.weapon == WP_BLASTER && (ent->s.eFlags & EF_ALT_FIRING)) ||
				(ent->s.weapon == WP_REPEATER && !(ent->s.eFlags & EF_ALT_FIRING))	
				))
			{ //If the weapon has spread, just reduce damage based on distance for nospread tweak.  This should probably be accompanied with the damagenumber setting so you can keep track of your dmg..
				float lifetime = (10000 - ent->nextthink + level.time) * 0.001;
				//float scale = powf(2, -lifetime);
				float scale = -1.5 * lifetime + 1;

				scale += 0.1f; //offset it a bit so super close shots dont get affected at all

				if (scale < 0.2f)
					scale = 0.2f;
				else if (scale > 1.0f) 
					scale = 1.0f;

				ent->damage *= scale;

				//trap->SendServerCommand(-1, va("chat \"Missile has been alive for %.2f s new dmg is %i scale is %.2f\n\"", lifetime, ent->damage, scale));
			}

			if (ent->s.weapon == WP_BOWCASTER || ent->s.weapon == WP_FLECHETTE ||
				ent->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (ent->s.weapon == WP_FLECHETTE && (ent->s.eFlags & EF_ALT_FIRING))
				{
					/* fix: there are rare situations where flechette did
					explode by timeout AND by impact in the very same frame, then here
					ent->think was set to G_FreeEntity, so the folowing think 
					did invalidate this entity, BUT it would be reused later in this 
					function for explosion event. This, then, would set ent->freeAfterEvent
					to qtrue, so event later, when reusing this entity by using G_InitEntity(),
					it would have this freeAfterEvent set AND this would in case of dropped
					item erase it from game immeadiately. THIS for example caused
					very rare flag dissappearing bug.	 */
					if (ent->think == WP_flechette_alt_blow)
						ent->think(ent);
				}
				else
				{
					G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
						/*ent->s.origin*/ent->r.currentOrigin, ent->damage, 
						DAMAGE_HALF_ABSORB, ent->methodOfDeath);
					didDmg = qtrue;
				}
			}
			else
			{
				G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
					/*ent->s.origin*/ent->r.currentOrigin, ent->damage, 
					0, ent->methodOfDeath);
				didDmg = qtrue;
			}

			if (didDmg && other && other->client)
			{ //What I'm wondering is why this isn't in the NPC pain funcs. But this is what SP does, so whatever.
				class_t	npc_class = other->client->NPC_class;

				// If we are a robot and we aren't currently doing the full body electricity...
				if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
					   npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class == CLASS_REMOTE ||
					   npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 || //npc_class == CLASS_PROTOCOL ||//no protocol, looks odd
					   npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )
				{
					// special droid only behaviors
					if ( other->client->ps.electrifyTime < level.time + 100 )
					{
						// ... do the effect for a split second for some more feedback
						other->client->ps.electrifyTime = level.time + 450;
					}
					//FIXME: throw some sparks off droids,too
				}
			}
		}

		if ( ent->s.weapon == WP_DEMP2 )
		{//a hit with demp2 decloaks people, disables ships
			if ( other && other->client && other->client->NPC_class == CLASS_VEHICLE )
			{//hit a vehicle
				if ( other->m_pVehicle //valid vehicle ent
					&& other->m_pVehicle->m_pVehicleInfo//valid stats
					&& (other->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER//always affect speeders
						||(other->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER && ent->classname && Q_stricmp("vehicle_proj", ent->classname ) == 0) )//only vehicle ion weapons affect a fighter in this manner
					&& !FighterIsLanded( other->m_pVehicle , &other->client->ps )//not landed
					&& !(other->spawnflags&2) )//and not suspended
				{//vehicles hit by "ion cannons" lose control
					if ( other->client->ps.electrifyTime > level.time )
					{//add onto it
						//FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime += Q_irand(200,500);
						if ( other->client->ps.electrifyTime > level.time + 4000 )
						{//cap it
							other->client->ps.electrifyTime = level.time + 4000;
						}
					}
					else
					{//start it
						//FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime = level.time + Q_irand(200,500);
					}
				}
			}
			else if ( other && other->client && other->client->ps.powerups[PW_CLOAKED] )
			{
				Jedi_Decloak( other );
				if ( ent->methodOfDeath == MOD_DEMP2_ALT )
				{//direct hit with alt disables cloak forever
					//permanently disable the saboteur's cloak
					other->client->cloakToggleTime = Q3_INFINITE;
				}
				else
				{//temp disable
					other->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
				}
			}
		}
	}

#if _GRAPPLE//_GRAPPLE
	//Grapple hook crash here somewhere?
	if (!strcmp(ent->classname, "laserTrap") && ent->s.weapon == WP_BRYAR_PISTOL) {
		//gentity_t *nent;
		vec3_t v;

		/*
		nent = G_Spawn(qtrue);
		nent->freeAfterEvent = qtrue;
		nent->s.weapon = WP_BRYAR_PISTOL;//WP_GRAPPLING_HOOK; -- idk what this is
		nent->s.saberInFlight = qtrue;
		nent->s.owner = ent->s.owner;
		*/

		ent->enemy = NULL;
		ent->s.otherEntityNum = -1;
		ent->s.groundEntityNum = -1;

		if ( other->s.eType == ET_MOVER || (other->client && !( other->s.eFlags & EF_DEAD ) ) ) {
			if ( other->client ) {
				//G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );							//Event

				if (!ent->s.hasLookTarget) {
					G_PlayEffectID( G_EffectIndex("tusken/hit"), trace->endpos, trace->plane.normal );
				}
				ent->s.hasLookTarget = qtrue;

				ent->enemy = other;
				other->s.otherEntityNum = ent->parent->s.number;

				v[0] = other->r.currentOrigin[0];// + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
				v[1] = other->r.currentOrigin[1];// + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
				v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

				SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
				ent->s.otherEntityNum = ent->enemy->s.clientNum;
				other->s.otherEntityNum = ent->parent->s.clientNum;
			} else {
				if ( !strcmp(other->classname, "func_rotating") || !strcmp(other->classname, "func_pendulum") ) {
					Weapon_HookFree(ent);	// don't work
					return;
				}
				ent->s.otherEntityNum = other->s.number;
				ent->s.groundEntityNum = other->s.number;
				VectorCopy(trace->endpos, v);
				//G_AddEvent( nent, EV_MISSILE_MISS, 0); //DirToByte( trace->plane.normal ) );				//Event
				if (!ent->s.hasLookTarget) {
					G_PlayEffectID( G_EffectIndex("tusken/hitwall"), trace->endpos, trace->plane.normal );
				}
				ent->s.hasLookTarget = qtrue;
			}
		} else {
			VectorCopy(trace->endpos, v);
			//G_AddEvent( nent, EV_MISSILE_MISS, 0);//DirToByte( trace->plane.normal ) );						//Event
			if (!ent->s.hasLookTarget) {
				G_PlayEffectID( G_EffectIndex("tusken/hitwall"), trace->endpos, trace->plane.normal );
			}
			ent->s.hasLookTarget = qtrue;
		}

		VectorCopy(trace->plane.normal, ent->s.angles);
		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		// change over to a normal entity right at the point of impact
		//nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_MISSILE;

		G_SetOrigin( ent, v );
		//G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.lastHitLoc);
		VectorSubtract( ent->r.currentOrigin, ent->parent->client->ps.origin, v );

		trap->LinkEntity( (sharedEntity_t *)ent ); //BROADCAST? so it wont get fucked by PVS?
		//trap->LinkEntity( (sharedEntity_t *)nent );

		return;
	}
#endif

killProj:
	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client && !isKnockedSaber ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else if (ent->s.weapon != G2_MODEL_PART && !isKnockedSaber) {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	if (!isKnockedSaber)
	{
		ent->freeAfterEvent = qtrue;

		// change over to a normal entity right at the point of impact
		ent->s.eType = ET_GENERAL;
	}

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	ent->takedamage = qfalse;
	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent, ent->splashMethodOfDeath ) ) {
			if( !hitClient 
				&& g_entities[ent->r.ownerNum].client ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		ent->freeAfterEvent = qfalse; //it will free itself
	}

	trap->LinkEntity( (sharedEntity_t *)ent );
}

/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin, groundSpot;
	trace_t		tr;
	int			passent;
	qboolean	isKnockedSaber = qfalse;

	if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{
		isKnockedSaber = qtrue;
		ent->s.pos.trType = TR_GRAVITY;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );


	//If its a rocket, and older than 500ms, make it solid to the shooter.
	if ((g_tweakWeapons.integer & WT_SOLID_ROCKET) && (ent->s.weapon == WP_ROCKET_LAUNCHER) && (!ent->raceModeShooter) && (ent->nextthink - level.time < 9500)) {
		ent->r.ownerNum = ENTITYNUM_WORLD;
	}

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
	else {
		// ignore interactions with the missile owner
		if ( (ent->r.svFlags&SVF_OWNERNOTSHARED) 
			&& (ent->s.eFlags&EF_JETPACK_ACTIVE) )
		{//A vehicle missile that should be solid to its owner
			//I don't care about hitting my owner
			passent = ent->s.number;
		}
		else
		{
			passent = ent->r.ownerNum;
		}
	}
	// trace a line from the previous position to the current position
	if (d_projectileGhoul2Collision.integer == 1) //JAPRO - Serverside - Weapons - New Hitbox Option
	{
		JP_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, qfalse, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );

		if (tr.fraction != 1.0 && tr.entityNum < ENTITYNUM_WORLD)
		{
			gentity_t *g2Hit = &g_entities[tr.entityNum];

			if (g2Hit->inuse && g2Hit->client && g2Hit->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				g2Hit->client->g2LastSurfaceHit = tr.surfaceFlags;
				g2Hit->client->g2LastSurfaceTime = level.time;
			}

			if (g2Hit->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}
	}
	else
	{
		JP_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, qfalse, 0, 0 );
	}

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		JP_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask, qfalse, 0, 0 );
		tr.fraction = 0;
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	if (ent->passThroughNum && tr.entityNum == (ent->passThroughNum-1))
	{
		VectorCopy( origin, ent->r.currentOrigin );
		trap->LinkEntity( (sharedEntity_t *)ent );
		goto passthrough;
	}

	trap->LinkEntity( (sharedEntity_t *)ent );

	if (ent->s.weapon == G2_MODEL_PART && !ent->bounceCount)
	{
		vec3_t lowerOrg;
		trace_t trG;

		VectorCopy(ent->r.currentOrigin, lowerOrg);
		lowerOrg[2] -= 1;
		JP_Trace( &trG, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, lowerOrg, passent, ent->clipmask, qfalse, 0, 0 );

		VectorCopy(trG.endpos, groundSpot);

		if (!trG.startsolid && !trG.allsolid && trG.entityNum == ENTITYNUM_WORLD)
		{
			ent->s.groundEntityNum = trG.entityNum;
		}
		else
		{
			ent->s.groundEntityNum = ENTITYNUM_NONE;
		}
	}

	if (tr.fraction != 1) { //Hit something maybe
		qboolean skip = qfalse;
	
		gentity_t *other = &g_entities[tr.entityNum]; //Check to see if we hit a lightsaber and they are in another dimension, if so dont do the hit code..
		if (other && other->r.contents & CONTENTS_LIGHTSABER)
		{
			gentity_t *otherOwner = &g_entities[other->r.ownerNum];
			gentity_t *owner = &g_entities[ent->r.ownerNum];
			/*
			if (owner->s.bolt1 && !otherOwner->s.bolt1)//We are dueling/racing and they are not
				skip = qtrue;
			else if (!owner->s.bolt1 && otherOwner->s.bolt1)//They are dueling/racing and we are not
				skip = qtrue;
			*/
			if (owner->s.bolt1 != otherOwner->s.bolt1) //Dont impact if its from another dimension
				skip = qtrue;
		}
	
		if ( tr.fraction != 1 && !skip) {
		
			// never explode or bounce on sky
			if ( tr.surfaceFlags & SURF_NOIMPACT ) {
				// If grapple, reset owner
				if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
					ent->parent->client->hook = NULL;
				}

				if ((ent->s.weapon == WP_SABER && ent->isSaberEntity) || isKnockedSaber)
				{
					G_RunThink( ent );
					return;
				}
				else if (ent->s.weapon != G2_MODEL_PART)
				{
					G_FreeEntity( ent );
					return;
				}
			}

			if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
				(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
			{ //player or NPC, try making a mark on him
				//copy current pos to s.origin, and current projected traj to origin2
				VectorCopy(ent->r.currentOrigin, ent->s.origin);
				BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin2 );

				if (VectorCompare(ent->s.origin, ent->s.origin2))
				{
					ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
				}
			}

			G_MissileImpact( ent, &tr );

			if (tr.entityNum == ent->s.otherEntityNum)
			{ //if the impact event other and the trace ent match then it's ok to do the g2 mark
				ent->s.trickedentindex = 1;
			}

			if ( ent->s.eType != ET_MISSILE && ent->s.weapon != G2_MODEL_PART )
			{
				return;		// exploded
			}
		}
	}

passthrough:
	if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
	{//stuck missiles should check some special stuff
		G_RunStuckMissile( ent );
		return;
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		if (ent->s.groundEntityNum == ENTITYNUM_WORLD)
		{
			ent->s.pos.trType = TR_LINEAR;
			VectorClear(ent->s.pos.trDelta);
			ent->s.pos.trTime = level.time;

			VectorCopy(groundSpot, ent->s.pos.trBase);
			VectorCopy(groundSpot, ent->r.currentOrigin);

			if (ent->s.apos.trType != TR_STATIONARY)
			{
				ent->s.apos.trType = TR_STATIONARY;
				ent->s.apos.trTime = level.time;

				ent->s.apos.trBase[ROLL] = 0;
				ent->s.apos.trBase[PITCH] = 0;
			}
		}
	}
	// check think function after bouncing
	G_RunThink( ent );
}

#if _GRAPPLE//_GRAPPLE
void StandardSetBodyAnim(gentity_t *self, int anim, int flags, int body);
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	float vel = 2400.0f;
	float inheritance = 0.5f;
	int lifetime = 1000;
	gentity_t	*hook;
	//gentity_t *missile;

	if (!self->client->sess.raceMode) {
		inheritance = g_hookInheritance.value;
		vel = g_hookSpeed.integer;
		lifetime = 30000;
	}

	VectorNormalize (dir);

	vel = vel + DotProduct(dir, self->client->ps.velocity)*inheritance; //Inheritence scale - Hardcode this in racemode

	if (vel < 250)
		vel = 250;

	hook = G_Spawn(qtrue);
	hook->classname = "laserTrap";
	hook->nextthink = level.time + lifetime; //Dont let it go super far in racemode?
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->s.clientNum = self->s.clientNum;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_BRYAR_PISTOL;//WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_STUN_BATON;//MOD_GRAPPLE
	hook->clipmask = MASK_SHOT;
	hook->parent = self;

	hook->s.owner = self->s.number;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;
	hook->s.otherEntityNum = -1;
	hook->s.groundEntityNum = -1;

	hook->s.saberInFlight = qtrue;

	//hook->target_ent = NULL; // ???

	VectorCopy( start, hook->s.pos.trBase );

	if ( self->client->pers.haste )
		VectorScale( dir, vel * 1.3, hook->s.pos.trDelta );
	else 
		VectorScale( dir, vel, hook->s.pos.trDelta );

	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	self->client->hook = hook;

	//hm.
	G_Sound( self, CHAN_AUTO, G_SoundIndex( "sound/weapons/melee/swing2.wav" ) );
	StandardSetBodyAnim(self, BOTH_FORCEPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, SETANIM_TORSO);
	//G_AddEvent( hook, EV_FIRE_WEAPON, 0 );

	return hook;
}
#endif



/*
//-----------------------------------------------------------------------------
gentity_t *fire_grapple( gentity_t *self, vec3_t org, vec3_t dir )
//-----------------------------------------------------------------------------
{
	gentity_t	*missile;

	missile = G_Spawn(qfalse);
	
	missile->nextthink = level.time + 5000;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;

		missile->classname = "hook";
	//missile->parent = owner;
	//missile->r.ownerNum = owner->s.number;

	//japro - do this so clients can know who the missile belongs to.. so they can hide it if its from another dimension
	//missile->s.owner = owner->s.number;
	//

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;	// NOTENOTE This is a Quake 3 addition over JK2
	missile->target_ent = NULL;

	//if (owner->client && owner->client->sess.raceMode)
		missile->s.pos.trTime -= MISSILE_PRESTEP_TIME;//this be why rocketjump fucks up at high speed

	SnapVector(org);
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, 555, missile->s.pos.trDelta );
	VectorCopy( org, missile->r.currentOrigin);
	SnapVector(missile->s.pos.trDelta);

	Com_Printf("ass\n");

	self->client->hook = missile;

	Com_Printf("Missile made\n");

	return missile;
}


*/
//=============================================================================




