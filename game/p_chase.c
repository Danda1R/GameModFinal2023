#include "g_local.h"

cvar_t* tpp;
cvar_t* crossh;

void ChasecamTrack(edict_t* ent);

void ChasecamStart(edict_t* ent)
{
    edict_t *chasecam;
    if (ent->client->resp.spectator)
        return;
    if (level.intermissiontime)
        return;
    ent->client->chasetoggle = 1;
    ent->client->ps.gunindex = 0;
    chasecam = G_Spawn();
    chasecam->owner = ent;
    chasecam->solid = SOLID_NOT;
    chasecam->movetype = MOVETYPE_FLYMISSILE;
    ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
    ent->svflags |= SVF_NOCLIENT; 
    VectorCopy(ent->s.angles, chasecam->s.angles);
    VectorClear(chasecam->mins);
    VectorClear(chasecam->maxs);
    VectorCopy(ent->s.origin, chasecam->s.origin);
    chasecam->classname = "chasecam";
    chasecam->prethink = ChasecamTrack;
    ent->client->chasecam = chasecam;
    ent->client->oldplayer = G_Spawn();
    CheckChasecam_Viewent(ent);
    MakeFakeCrosshair(ent);
}

void ChasecamRestart(edict_t* ent)
{
    if (ent->owner->health <= 0)
    {
        G_FreeEdict(ent);
        return;
    }
    if (ent->owner->waterlevel && !tpp->value)
        return;
    ChasecamStart(ent->owner);
    G_FreeEdict(ent);
}

void ChasecamRemove(edict_t* ent, int opt)
{

    VectorClear(ent->client->chasecam->velocity);

    if (!level.intermissiontime)
        ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
    ent->s.modelindex = ent->client->oldplayer->s.modelindex;
    ent->svflags &= ~SVF_NOCLIENT;

    DestroyFakeCrosshair(ent);

    if (opt == OPTION_BACKGROUND)
    {
        ent->client->chasetoggle = 0;
        G_FreeEdict(ent->client->chasecam);
        G_FreeEdict(ent->client->oldplayer);
        ent->client->oldplayer = NULL;
        ent->client->chasecam = G_Spawn();
        ent->client->chasecam->owner = ent;
        ent->client->chasecam->solid = SOLID_NOT;
        ent->client->chasecam->movetype = MOVETYPE_FLYMISSILE;
        VectorClear(ent->client->chasecam->mins);
        VectorClear(ent->client->chasecam->maxs);
        ent->client->chasecam->classname = "chasecam";
        ent->client->chasecam->prethink = ChasecamRestart;
    }
    else if (opt == OPTION_OFF)
    {
        G_FreeEdict(ent->client->oldplayer);
        ent->client->oldplayer = NULL;
        ent->client->chasetoggle = 0;
        G_FreeEdict(ent->client->chasecam);
        ent->client->chasecam = NULL;
    }
}

void ChasecamTrack(edict_t* ent)
{
    trace_t      tr;
    vec3_t       spot1, spot2, dir;
    vec3_t       forward, right, up, angles;
    int          distance;
    int          tot;
    ent->nextthink = level.time + 0.100;
    if (ent->owner->waterlevel && !tpp->value)
    {
        ChasecamRemove(ent->owner, OPTION_BACKGROUND);
        return;
    }
    VectorCopy(ent->owner->client->v_angle, angles);
    if (angles[PITCH] > 56)
        angles[PITCH] = 56;
    AngleVectors(angles, forward, right, up);
    VectorNormalize(forward);
    VectorMA(ent->owner->s.origin, -ent->chasedist1, forward, spot2);
    spot2[2] += (ent->owner->viewheight + 16);
    if (!ent->owner->groundentity)
        spot2[2] += 16;
    tr = gi.trace(ent->owner->s.origin, vec3_origin, vec3_origin, spot2, ent->owner, MASK_SOLID);
    VectorSubtract(tr.endpos, ent->owner->s.origin, spot1);
    ent->chasedist1 = VectorLength(spot1);
    VectorMA(tr.endpos, 2, forward, spot2);
    VectorCopy(spot2, spot1);
    spot1[2] += 32;
    tr = gi.trace(spot2, vec3_origin, vec3_origin, spot1, ent->owner, MASK_SOLID);
    if (tr.fraction < 1.000)
    {
        VectorCopy(tr.endpos, spot2);
        spot2[2] -= 32;
    }
    VectorSubtract(spot2, ent->s.origin, dir);
    distance = VectorLength(dir);
    VectorNormalize(dir);
    tr = gi.trace(ent->s.origin, vec3_origin, vec3_origin, spot2, ent->owner, MASK_SOLID);
    if (tr.fraction == 1.000)
    {
        VectorSubtract(ent->s.origin, ent->owner->s.origin, spot1);
        VectorNormalize(spot1);
        VectorCopy(spot1, ent->s.angles);
        tot = (distance * 0.400);
        if (tot > 5.200)
        {
            ent->velocity[0] = ((dir[0] * distance) * 5.2);
            ent->velocity[1] = ((dir[1] * distance) * 5.2);
            ent->velocity[2] = ((dir[2] * distance) * 5.2);
        }
        else
        {
            if (tot > 1.000)
            {
                ent->velocity[0] = ((dir[0] * distance) * tot);
                ent->velocity[1] = ((dir[1] * distance) * tot);
                ent->velocity[2] = ((dir[2] * distance) * tot);
            }
            else
            {
                ent->velocity[0] = (dir[0] * distance);
                ent->velocity[1] = (dir[1] * distance);
                ent->velocity[2] = (dir[2] * distance);
            }
        }
        VectorSubtract(ent->owner->s.origin, ent->s.origin, spot1);
        if (VectorLength(spot1) < 20)
        {
            ent->velocity[0] *= 2;
            ent->velocity[1] *= 2;
            ent->velocity[2] *= 2;
        }
    }
    else
        VectorCopy(spot2, ent->s.origin);
    ent->chasedist1 += 2;
    if (ent->chasedist1 > (60.00 + ent->owner->client->zoom))
        ent->chasedist1 = (60.00 + ent->owner->client->zoom);
    if (VectorCompare(ent->movedir, ent->s.origin))
    {
        if (distance > 20)
            ent->chasedist2++;
    }
    if (ent->chasedist2 > 3)
    {
        G_FreeEdict(ent->owner->client->oldplayer);
        ChasecamStart(ent->owner);
        G_FreeEdict(ent);
        return;
    }
    VectorCopy(ent->s.origin, ent->movedir);
    gi.linkentity(ent);
    UpdateFakeCrosshair(ent->owner);
}

void Cmd_Chasecam_Toggle(edict_t* ent)
{
    if (!(ent->waterlevel && !tpp->value) && !ent->deadflag)
    {
        if (ent->client->chasetoggle)
            ChasecamRemove(ent, OPTION_OFF);
        else
            ChasecamStart(ent);
    }
    else if ((ent->waterlevel && !tpp->value) && !ent->deadflag)
        gi.cprintf(ent, PRINT_HIGH, "Camera cannot be modified while in water\n");
}

void CheckChasecam_Viewent(edict_t* ent)
{
    vec3_t angles;
    if (!ent->client->oldplayer->client)
        ent->client->oldplayer->client = ent->client;
    if ((ent->client->chasetoggle == 1) && (ent->client->oldplayer))
    {
        if (ent->client->use)
            VectorCopy(ent->client->oldplayer->s.angles, angles);
        ent->client->oldplayer->s = ent->s; //Copy player related info
        if (ent->client->use)
            VectorCopy(angles, ent->client->oldplayer->s.angles);
        gi.linkentity(ent->client->oldplayer);
    }
}

int nohud = 0;

void Cmd_ToggleHud()
{
    if (deathmatch->value)
        return;
    nohud = (1 - nohud);
    if (nohud)
        gi.configstring(CS_STATUSBAR, NULL);
    else
        gi.configstring(CS_STATUSBAR, single_statusbar);
}

static void P_ProjectSource(gclient_t* client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
    vec3_t	_distance;

    VectorCopy(distance, _distance);
    if (client->pers.hand == LEFT_HANDED)
        _distance[1] *= -1;
    else if (client->pers.hand == CENTER_HANDED)
        _distance[1] = 0;
    G_ProjectSource(point, _distance, forward, right, result);
}

void UpdateFakeCrosshair(edict_t* ent)
{
    vec3_t offset, spot, forward, right, start;
    trace_t tr;
    if (!ent->crosshair)
        return;
    VectorSet(offset, 8, 8, ent->viewheight - 8);
    if (ent->client->use)
        AngleVectors(ent->client->oldplayer->s.angles, forward, right, NULL);
    else
        AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorNormalize(forward);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    VectorMA(start, 8192, forward, spot);
    tr = gi.trace(start, vec3_origin, vec3_origin, spot, ent, MASK_PLAYERSOLID);
    VectorCopy(tr.endpos, ent->crosshair->s.origin);
    gi.linkentity(ent->crosshair);
}

void MakeFakeCrosshair(edict_t* ent)
{
    if (!crossh->value)
        return;
    ent->crosshair = G_Spawn();
    ent->crosshair->solid = SOLID_NOT;
    ent->crosshair->movetype = MOVETYPE_NONE;
    ent->crosshair->s.renderfx = RF_FULLBRIGHT;
    gi.setmodel(ent->crosshair, "models/objects/gibs/sm_meat/tris.md2");
    UpdateFakeCrosshair(ent);
}

void DestroyFakeCrosshair(edict_t* ent)
{
    if (!ent->crosshair)
        return;
    G_FreeEdict(ent->crosshair);
    ent->crosshair = NULL;
}