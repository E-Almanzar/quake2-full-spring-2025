/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"
#include "m_player.h"

int wizardspawning;
int returnbool = 0;
int externgivall = 0;
vec3_t returncoords, returnviewangles;


char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	/*if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}*/

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;
	
	if (externgivall) {
		give_all = true;
	}
	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	/*if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}*/

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg, *arg;

	/*if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}*/

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	arg = gi.args();
	if (Q_stricmp(arg, "bounce") == 0) {
		if (ent->movetype != MOVETYPE_FLYRICOCHET) {
			ent->movetype = MOVETYPE_FLYRICOCHET;
			msg = "bounce on\n";
		}
		else {
			ent->movetype = MOVETYPE_WALK;
			msg = "bounce off\n";
		}
	}
	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
	ent->client->showCustomUI = false;

}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		sprintf(st, "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

static void P_ProjectSource(gclient_t* client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);

int didawizardspawnme(int amithewizard, int didawizardspawnme) {
	if (amithewizard) {
		wizardspawning = 1;
		return -1;
	}

	if (didawizardspawnme) {
		if (wizardspawning) {
			wizardspawning = 0;
			return 1;
		}

	}
	return 0;
}


void spawnmonster(edict_t* ent, char* name) {
	vec3_t origin, dir, offset, forward, right, start;
	//VectorCopy(start, ent->s.origin); WAIT CAN WE USE VECTOR COPY ON THIS TO TELEPORT???
	//AngleVectors(ent->client->v_angle, dir, NULL, NULL);
	//Basically getting it to the right spot?
	if ((Q_stricmp(name, "monster_supertank") == 0)) {
		VectorSet(offset, 250, 0, ent->viewheight - 8);

	}
	else
		VectorSet(offset, 100, 0, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	start[2] = ent->s.origin[2];

	edict_t* entspawn;
	entspawn = G_Spawn();
	entspawn->classname = name;

	//VectorSet(dir, 0, 0, 0);
	VectorCopy(start, entspawn->s.origin);
	VectorCopy(ent->s.angles, entspawn->s.angles);

	//Yes a wizard did spawn you 
	didawizardspawnme(1, 0);
	ED_CallSpawn(entspawn);
}

void Cmd_Spawn_f(edict_t* ent) //EALM
{
	if (!ent || !ent->client) return;
	char* arg;
	arg = gi.args();
	spawnmonster(ent, arg);
	//We want to have individual helper functions to call the spawn of each guy
	// Tank brain mutant parasite and medic
	//gi.cprintf(ent, PRINT_HIGH, "your team is %s\n", ent->team);

}
void Cmd_Spawn_Soldier_f(edict_t* ent) { if (!ent || !ent->client) return; 
if (ent->plevel < 1) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 1 to summon the Soldier\n"); return; }
spawnmonster(ent, "monster_soldier"); }
void Cmd_Spawn_Tank_f(edict_t* ent) { if (!ent || !ent->client) return; 
if (ent->plevel < 4) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 4 to summon the Tank\n"); return; }
spawnmonster(ent, "monster_tank"); }
void Cmd_Spawn_Parasite_f(edict_t* ent) { if (!ent || !ent->client) return; 
if (ent->plevel < 2) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 2 to summon the Parasite\n"); return; }
spawnmonster(ent, "monster_parasite"); }
void Cmd_Spawn_Brain_f(edict_t* ent) { if (!ent || !ent->client) return; 
if (ent->plevel < 3) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 3 to summon the Brain\n"); return; }
spawnmonster(ent, "monster_brain"); }
void Cmd_Spawn_Supertank_f(edict_t* ent) { if (!ent || !ent->client) return; 
if (ent->plevel < 5) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 5 to summon the Supertank\n"); return; }
spawnmonster(ent, "monster_supertank"); }

void Cmd_Heal_f(edict_t* ent){ //EALM
	//Be sure to take away a spell slot
	
	if (ent->plevel < 1) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 1 to cast Heal\n"); return; }
	else { gi.cprintf(ent, PRINT_HIGH, "Casted Heal\n"); }
	if(ent->health +25 > 200){
		gi.cprintf(ent, PRINT_HIGH, "Too much health!!\n");
		//we could kill them, id be kinda funny, or do like 100 damage
		ent->health = ent->health - 25;
		return;
	}
	ent->health = ent->health + 25;
	//AMMO
	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ITEM_INDEX(FindItemByClassname("ammo_bullets"))]--;

}

void Cmd_Return_f(edict_t* ent) { //EALM
	//Be sure to take away a spell slot
	//naaa igaf
	vec3_t forward;
	if (ent->plevel < 1) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 2 to cast Return\n"); return; }
	else { gi.cprintf(ent, PRINT_HIGH, "Casted Return\n"); }
	if (returnbool) {
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION2);
		gi.WritePosition(ent->s.origin);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
		
		ent->s.origin[0] = returncoords[0];
		ent->s.origin[1] = returncoords[1];
		ent->s.origin[2] = returncoords[2];
		VectorCopy(returnviewangles, ent->s.angles);

		
		
		returnbool = 0;


	}
	else {
		returncoords[0] = ent->s.origin[0];
		returncoords[1] = ent->s.origin[1];
		returncoords[2] = ent->s.origin[2];
		//AngleVectors(, forward, NULL, NULL);
		VectorCopy(ent->s.angles, returnviewangles);

		
		returnbool = 1;

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION2);
		gi.WritePosition(ent->s.origin);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

	}
	//Ooh a swap spell would be sick
	//but then you'd have to hit stuff :(
	//na its all cool past beth 
	//AMMO
	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ITEM_INDEX(FindItemByClassname("ammo_bullets"))]-=2;

}

void freeze_think(edict_t* self)
{

	self->s.frame++;
	self->nextthink = level.time + FRAMETIME + 10;
	//IG we should kill them?
}

void Cmd_Freeze_f(edict_t* ent) {
	if (ent->plevel < 4) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 4 to cast Freeze\n"); return; }
	else { gi.cprintf(ent, PRINT_HIGH, "Casted Freeze\n"); }
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage;
	int			kick;

	damage = 150;
	kick = 250;


	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorScale(forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 0, 7, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	//fire_rail(ent, start, forward, damage, kick);

	vec3_t		from;
	vec3_t		end, aimdir;
	trace_t		tr;
	edict_t* ignore, *self;
	int			mask;
	qboolean	water;
	self = ent; 
	VectorCopy(forward, aimdir);
	
	
	VectorMA(start, 8192, aimdir, end);
	VectorCopy(start, from);
	ignore = self;
	water = false;
	mask = MASK_SHOT | CONTENTS_SLIME | CONTENTS_LAVA;
	while (ignore)
	{
		tr = gi.trace(from, NULL, NULL, end, ignore, mask);

		if (tr.contents & (CONTENTS_SLIME | CONTENTS_LAVA))
		{
			mask &= ~(CONTENTS_SLIME | CONTENTS_LAVA);
			water = true;
		}
		else
		{
			if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
				ignore = tr.ent;
			else
				ignore = NULL;

			if ((tr.ent != self) && (tr.ent->takedamage)){
				//T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, 0, MOD_RAILGUN);
				tr.ent->think = freeze_think;
			}
		}

		VectorCopy(tr.endpos, from);
	}

	// send gun puff / flash TS is cool
	/*gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_RAILTRAIL);
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(self->s.origin, MULTICAST_PHS);*/
	//	gi.multicast (start, MULTICAST_PHS);
	

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ITEM_INDEX(FindItemByClassname("ammo_grenades"))]--;
	
}
void Cmd_Deathnote_f(edict_t* ent) {
	edict_t* target = NULL;
	char* name = gi.args();
	gi.cprintf(ent, PRINT_HIGH, "deathnote %s\n", name);
	while ((target = findradius(target, ent->s.origin, 500)) != NULL)
	{
		if (target == ent)
			continue;

		if (!target->takedamage)
			continue;
		if(Q_stricmp(target->classname, name) == 0){
			G_FreeEdict(target);
			return;
		}
	}
}

void Cmd_GiveSpell_f(edict_t *ent){
	if (!ent)
		return;
	if (ent->plevel < 4) { gi.cprintf(ent, PRINT_HIGH, "You must be at least level 5 to cast Gimmie\n"); return; }
	else { gi.cprintf(ent, PRINT_HIGH, "Casted Gimmie\n"); }
	externgivall = true;
	Cmd_Give_f(ent);
	externgivall = false;
	//VectorInverse(ent->client->v_angle);
	//IDKKKK
	//throw healthpacks/ammo/powerups?

}

void Cmd_Dropb_f(edict_t* ent) {
	if (!ent)
		return;
	gitem_t* item;
	item = FindItemByClassname("ammo_bullets");
	Drop_Ammo(ent, item);
}

void Cmd_Dropc_f(edict_t* ent) {
	if (!ent)
		return;
	gitem_t* item;
	item = FindItemByClassname("ammo_cells");
	Drop_Ammo(ent, item);
}

void Cmd_Dropg_f(edict_t* ent) {
	if (!ent)
		return;
	gitem_t* item;
	item = FindItemByClassname("ammo_grenades");
	Drop_Ammo(ent, item);
}

void Cmd_Drops_f(edict_t* ent) {
	if (!ent)
		return;
	gitem_t* item;
	item = FindItemByClassname("ammo_slugs");
	Drop_Ammo(ent, item);
}
void Cmd_Dropr_f(edict_t* ent) {
	if (!ent)
		return;
	gitem_t* item;
	item = FindItemByClassname("ammo_rockets");
	Drop_Ammo(ent, item);
}

void Cmd_SetLevel_f(edict_t* ent)
{
	char* name;
	name = gi.args();
	int newlev = atoi(name);
	ent->plevel = newlev;
}

/*
void summon_think2(edict_t* self)
{
	M_MoveFrame(self);
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround(self);
	}
	M_CatagorizePosition(self);
	M_WorldEffects(self);
	M_SetEffects(self);


	edict_t* ent = NULL;
	int ownercount = 0;
	while ((ent = findradius(ent, self->s.origin, 528)) != NULL) {
		if (ent == self)
			continue;
		if (!ent->takedamage)
			continue;
		if (strcmp(ent->classname, "misc_explobox") != 0)
			continue;
		if (!(ent->svflags & SVF_MONSTER) && (!ent->client) && (strcmp(ent->classname, "misc_explobox") != 0))
			continue;
		if (ent == self->owner)
			continue;

		self->enemy = ent;


	}
}


void Cmd_Hypnosis_f(edict_t * ent){//i give up this isnt working

		vec3_t		start;
		vec3_t		forward, right;
		vec3_t		offset;
		int			damage;
		int			kick;

		damage = 150;
		kick = 250;


		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorScale(forward, -3, ent->client->kick_origin);
		ent->client->kick_angles[0] = -3;

		VectorSet(offset, 0, 7, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		//fire_rail(ent, start, forward, damage, kick);

		vec3_t		from;
		vec3_t		end, aimdir;
		trace_t		tr;
		edict_t* ignore, * self;
		int			mask;
		qboolean	water;
		self = ent;
		VectorCopy(forward, aimdir);


		VectorMA(start, 8192, aimdir, end);
		VectorCopy(start, from);
		ignore = self;
		water = false;
		mask = MASK_SHOT | CONTENTS_SLIME | CONTENTS_LAVA;
		while (ignore)
		{
			tr = gi.trace(from, NULL, NULL, end, ignore, mask);

			if (tr.contents & (CONTENTS_SLIME | CONTENTS_LAVA))
			{
				mask &= ~(CONTENTS_SLIME | CONTENTS_LAVA);
				water = true;
			}
			else
			{
				if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
					ignore = tr.ent;
				else
					ignore = NULL;

				if ((tr.ent != self) && (tr.ent->takedamage)) {
					//T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, 0, MOD_RAILGUN);
					tr.ent->monsterinfo.aiflags |= AI_SUMMONED;
					tr.ent->think = summon_think2;
					//tr.ent->enemy = NULL;
					//crashes the game :(
					
					self = tr.ent;

					M_MoveFrame(self);
					if (self->linkcount != self->monsterinfo.linkcount)
					{
						self->monsterinfo.linkcount = self->linkcount;
						M_CheckGround(self);
					}
					M_CatagorizePosition(self);
					M_WorldEffects(self);
					M_SetEffects(self);


					edict_t* ent1 = NULL;
					int ownercount = 0;
					while ((ent1 = findradius(ent1, self->s.origin, 528)) != NULL) {
						if (ent1 == self)
							continue;
						if (!ent1->takedamage)
							continue;
						if (strcmp(ent1->classname, "misc_explobox") != 0)
							continue;
						if (!(ent1->svflags & SVF_MONSTER) && (!ent1->client) && (strcmp(ent1->classname, "misc_explobox") != 0))
							continue;
						if (ent1 == self->owner)
							continue;

						self->enemy = ent1;


					}
					

					//tr.ent->think = freeze_think;
				}
			}

			VectorCopy(tr.endpos, from);
		}

}
//Useless
void Cmd_HealTarget_f(edict_t* ent) {
	if (!ent)
		return;

	int	i;
	vec3_t		start;
	vec3_t		forward, right, dir, end;
	vec3_t		angles;
	int			damage = -50;
	int			kick = 2;
	vec3_t		offset;
	trace_t	tr;


	AngleVectors(angles, forward, right, NULL);
	gi.c
	(ent, PRINT_HIGH, "%i %i %i origin.\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	//fire_bullet(ent, start, forward, 1, 0, 0, 0, MOD_MACHINEGUN);
	//fire_bullet(ent, start, forward, damage, 0, 0, 0, 0);
	

	tr = gi.trace(ent->s.origin, NULL, NULL, start, ent, MASK_SHOT);
	
	if (!(tr.fraction < 1.0))
	{
		AngleVectors(angles, forward, right, NULL);
		//gi.cprintf(ent, PRINT_HIGH, "%i %i %i origin.\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
		VectorSet(offset, 0, 8, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		//vectoangles(forward, dir);
		//AngleVectors(dir, forward, right, NULL);

		
		//VectorMA(start, 8192, forward, end);
		
		tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);
		if ((tr.fraction < 1.0)){
			gi.cprintf(ent, PRINT_HIGH, "<1.\n");
			//if (tr.ent->takedamage) {
			if (tr.ent->takedamage) {
				gi.cprintf(ent, PRINT_HIGH, "Exists.\n");
				tr.ent->health + 100000;
				//gi.cprintf(ent, PRINT_HIGH, "Can take damage.\n");
				//T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, newKnockback, DAMAGE_BULLET, mod);
			}
		}
	}
	//else
	//	gi.cprintf(ent, PRINT_HIGH, "Fuck off.\n");
	//gi.dprintf("%s at %s, combattarget %s not found\n", self->classname, vtos(self->s.origin), self->combattarget);

}*/


void Cmd_whelp1_f(edict_t* ent) {

	if (ent->client->showCustomUI) {
		ent->client->showCustomUI = false;
		return;
	}
	ent->client->showCustomUI = true;

}


/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp(cmd, "use") == 0)
		Cmd_Use_f(ent);
	else if (Q_stricmp(cmd, "drop") == 0)
		Cmd_Drop_f(ent);
	else if (Q_stricmp(cmd, "give") == 0)
		Cmd_Give_f(ent);
	else if (Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if (Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if (Q_stricmp(cmd, "noclip") == 0)
		Cmd_Noclip_f(ent);
	else if (Q_stricmp(cmd, "inven") == 0)
		Cmd_Inven_f(ent);
	else if (Q_stricmp(cmd, "invnext") == 0)
		SelectNextItem(ent, -1);
	else if (Q_stricmp(cmd, "invprev") == 0)
		SelectPrevItem(ent, -1);
	else if (Q_stricmp(cmd, "invnextw") == 0)
		SelectNextItem(ent, IT_WEAPON);
	else if (Q_stricmp(cmd, "invprevw") == 0)
		SelectPrevItem(ent, IT_WEAPON);
	else if (Q_stricmp(cmd, "invnextp") == 0)
		SelectNextItem(ent, IT_POWERUP);
	else if (Q_stricmp(cmd, "invprevp") == 0)
		SelectPrevItem(ent, IT_POWERUP);
	else if (Q_stricmp(cmd, "invuse") == 0)
		Cmd_InvUse_f(ent);
	else if (Q_stricmp(cmd, "invdrop") == 0)
		Cmd_InvDrop_f(ent);
	else if (Q_stricmp(cmd, "weapprev") == 0)
		Cmd_WeapPrev_f(ent);
	else if (Q_stricmp(cmd, "weapnext") == 0)
		Cmd_WeapNext_f(ent);
	else if (Q_stricmp(cmd, "weaplast") == 0)
		Cmd_WeapLast_f(ent);
	else if (Q_stricmp(cmd, "kill") == 0)
		Cmd_Kill_f(ent);
	else if (Q_stricmp(cmd, "putaway") == 0)
		Cmd_PutAway_f(ent);
	else if (Q_stricmp(cmd, "wave") == 0)
		Cmd_Wave_f(ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	else if (Q_stricmp(cmd, "spawn") == 0) //past here is mine
		Cmd_Spawn_f(ent);
	else if (Q_stricmp(cmd, "whelp") == 0)
		Cmd_whelp_f(ent);
	else if (Q_stricmp(cmd, "heal") == 0)
		Cmd_Heal_f(ent);
	else if (Q_stricmp(cmd, "return") == 0)
		Cmd_Return_f(ent);
	else if (Q_stricmp(cmd, "freeze") == 0)
		Cmd_Freeze_f(ent);
	else if (Q_stricmp(cmd, "tank") == 0)
		Cmd_Spawn_Tank_f(ent);
	else if (Q_stricmp(cmd, "soldier") == 0)
		Cmd_Spawn_Soldier_f(ent);
	else if (Q_stricmp(cmd, "brain") == 0)
		Cmd_Spawn_Brain_f(ent);
	else if (Q_stricmp(cmd, "parasite") == 0)
		Cmd_Spawn_Parasite_f(ent);
	else if (Q_stricmp(cmd, "supertank") == 0)
		Cmd_Spawn_Supertank_f(ent);
	else if (Q_stricmp(cmd, "dropb") == 0)
		Cmd_Dropb_f(ent);
	else if (Q_stricmp(cmd, "dropc") == 0)
		Cmd_Dropc_f(ent);
	else if (Q_stricmp(cmd, "dropg") == 0)
		Cmd_Dropg_f(ent);
	else if (Q_stricmp(cmd, "drops") == 0)
		Cmd_Drops_f(ent);
	else if (Q_stricmp(cmd, "dropr") == 0)
		Cmd_Dropr_f(ent);
	else if (Q_stricmp(cmd, "showxp") == 0)
		Cmd_drawxp_f(ent);
	else if (Q_stricmp(cmd, "whelp1") == 0)
		Cmd_whelp1_f(ent);
	else if (Q_stricmp(cmd, "givespell") == 0)
		Cmd_GiveSpell_f(ent);
	else if (Q_stricmp(cmd, "setlevel") == 0)
		Cmd_SetLevel_f(ent);
	else if (Q_stricmp(cmd, "deathnote") == 0)
		Cmd_Deathnote_f(ent);
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
//EALM we need project source?
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