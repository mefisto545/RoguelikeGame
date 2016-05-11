#include <stdio.h>
#include <math.h>
#include "main.hpp"

const int LEVEL_UP_BASE = 200;
const int LEVEL_UP_FACTOR = 150;

void TCODConsole::printFramenew(int x, int y, int w, int h, bool empty, TCOD_bkgnd_flag_t flag, const char *fmt) 
{
	TCODConsole::putChar(x, y, 0, flag); //up-left
	TCODConsole::putChar(x + w - 1, y, 2, flag);//up-right
	TCODConsole::putChar(x, y + h - 1, 16, flag);//down-left
	TCODConsole::putChar(x + w - 1, y + h - 1, 18, flag);//down-right
	for (int i = 1; i < w - 1; i++)
	{
		TCODConsole::putChar(x + i, y, 1, flag);//down
		TCODConsole::putChar(x + i, y + h - 1, 17, flag);//up
	}
	if (h > 2) 
	{
		for (int i = 1; i < h-1; i++)
		{
			TCODConsole::putChar(x, y + i, 3, flag);//left
			TCODConsole::putChar(x + w - 1, y + i, 19, flag);//right
			if (empty)
			{
				TCODConsole::rect(x + 1, y + 1, w - 2, h - 2, true, flag);
			}
		}
	}
	if (fmt) 
	{
		int xs;
		xs = x + (w - strlen(fmt) - 2) / 2;
		TCODConsole::printEx(xs, y, TCOD_BKGND_SET, TCOD_LEFT, " %s ", fmt);
	}
}


// how many turns the monster chases the player
// after losing his sight
static const int TRACKING_TURNS = 5;

MonsterAi::MonsterAi() : moveCount(0) {}

void MonsterAi::update(Actor *owner)
{
	if (owner->destructible && owner->destructible->isDead())
	{
		return;
	}
	if (engine.map->isInFov(owner->x, owner->y))
	{
		// we can see the player. move towards him
		moveCount = TRACKING_TURNS;
	}
	else {
		moveCount--;
	}
	if (moveCount > 0)
	{
		moveOrAttack(owner, engine.player->x, engine.player->y);
	}
}

void MonsterAi::moveOrAttack(Actor *owner, int targetx, int targety)
{
	int dx = targetx - owner->x;
	int dy = targety - owner->y;
	float distance = sqrtf(dx*dx + dy*dy);
	if (distance > 2)
	{
		dx = 2 * (int)(round(dx / distance));
		dy = 2 * (int)(round(dy / distance));
		if (abs(dx) > abs(dy))
		{
			if (engine.map->canWalk(owner->x + dx, owner->y))
			{
				owner->x += dx;
			}
			else if (engine.map->canWalk(owner->x, owner->y + dy))
			{
				owner->y += dy;
			}
		}
		else
		{
			if (engine.map->canWalk(owner->x, owner->y + dy))
			{
				owner->y += dy;
			}
			else if (engine.map->canWalk(owner->x + dx, owner->y))
			{
				owner->x += dx;
			}
		}
	}
	else if (owner->attacker)
	{
		owner->attacker->attack(owner, engine.player);
	}
}

PlayerAi::PlayerAi() : xpLevel(1) {}

int PlayerAi::getNextLevelXp() 
{
	return LEVEL_UP_BASE + xpLevel*LEVEL_UP_FACTOR;
}

void PlayerAi::update(Actor *owner) 
{
	int levelUpXp = getNextLevelXp();
	if (owner->destructible->xp >= levelUpXp)
	{
		xpLevel++;
		owner->destructible->xp -= levelUpXp;
		engine.gui->message(TCODColor::yellow, "Ваши боевые навыки улучшились!\nВы достигли %d уровня", xpLevel);
		engine.gui->menu.clear();
		engine.gui->menu.addItem(Menu::CONSTITUTION, "Телосложение (+20 Ед.зд)");
		engine.gui->menu.addItem(Menu::STRENGTH, "Сила (+1 урон)");
		engine.gui->menu.addItem(Menu::AGILITY, "Толстокожесть (+1 защита)");
		Menu::MenuItemCode menuItem = engine.gui->menu.pick(Menu::PAUSE);
		switch (menuItem)
		{
		case Menu::CONSTITUTION:
			owner->destructible->maxHp += 20;
			owner->destructible->hp += 20;
			break;
		case Menu::STRENGTH:
			owner->attacker->power += 1;
			break;
		case Menu::AGILITY:
			owner->destructible->defense += 1;
			break;
		default:break;
		}
	}
	if (owner->destructible && owner->destructible->isDead()) 
	{
		return;
	}
	int dx = 0, dy = 0;
	switch (engine.lastKey.vk)
	{
		case TCODK_UP: dy = -2; break;
		case TCODK_DOWN: dy = 2; break;
		case TCODK_LEFT: dx = -2; break;
		case TCODK_RIGHT: dx = 2; break;
		case TCODK_CHAR: handleActionKey(owner, engine.lastKey.c); break;
		default:break;
	}
	if (dx != 0 || dy != 0)
	{
		engine.gameStatus = Engine::NEW_TURN;
		if (moveOrAttack(owner, owner->x + dx, owner->y + dy))
		{
			engine.map->computeFov();
		}
	}
}

bool PlayerAi::moveOrAttack(Actor *owner, int targetx, int targety)
{
	if (engine.map->isWall(targetx, targety)) return false;
	// look for living actors to attack
	for (Actor **iterator = engine.actors.begin(); iterator != engine.actors.end(); iterator++)
	{
		Actor *actor = *iterator;
		if (actor->destructible && !actor->destructible->isDead()
			&& actor->x == targetx && actor->y == targety)
		{
			owner->attacker->attack(owner, actor);
			return false;
		}
	}
	// look for corpses or items
	for (Actor **iterator = engine.actors.begin(); iterator != engine.actors.end(); iterator++)
	{
		Actor *actor = *iterator;
		bool corpseOrItem = (actor->destructible && actor->destructible->isDead()) || actor->pickable;
		if (corpseOrItem && actor->x == targetx && actor->y == targety)
		{
			engine.gui->message(TCODColor::lightGrey, "Вы видите %s.", actor->name);
		}
	}
	owner->x = targetx;
	owner->y = targety;
	return true;
}

void PlayerAi::handleActionKey(Actor *owner, int ascii)
{
	switch (ascii)
	{
	case 'g': // pickup item
	{
		bool found = false;
		for (Actor **iterator = engine.actors.begin();
			iterator != engine.actors.end(); iterator++)
		{
			Actor *actor = *iterator;
			if (actor->pickable && actor->x == owner->x && actor->y == owner->y)
			{
				if (actor->pickable->pick(actor, owner))
				{
					found = true;
					engine.gui->message(TCODColor::lightGrey, "Вы подбираете %s.",
						actor->name);
					break;
				}
				else if (!found) 
				{
					found = true;
					engine.gui->message(TCODColor::red, "Ваш инвентарь переполнен.");
				}
			}
		}
		if (!found) 
		{
			engine.gui->message(TCODColor::lightGrey, "Здесь нечего подбирать");
		}
		engine.gameStatus = Engine::NEW_TURN;
	}
	break;
	case 'i': // display inventory
	{
		Actor *actor = choseFromInventory(owner);
		if (actor)
		{
			actor->pickable->use(actor, owner);
			engine.gameStatus = Engine::NEW_TURN;
		}
	}
	break;
	case 'e':
		if (engine.stairs->x == owner->x && engine.stairs->y == owner->y)
		{
			engine.nextLevel();
		}
		else 
		{
			engine.gui->message(TCODColor::lightGrey, "Здесь нет лестницы");
		}
		break;
	case 'd':
	{
				  Actor *actor = choseFromInventory(owner);
				  if (actor) {
					  actor->pickable->drop(actor, owner);
					  engine.gameStatus = Engine::NEW_TURN;
				  }
	}
		break;
	default: break;
	}
}

Actor *PlayerAi::choseFromInventory(Actor *owner)
{
	static const int INVENTORY_WIDTH = 50;
	static const int INVENTORY_HEIGHT = 28;
	static TCODConsole con(INVENTORY_WIDTH, INVENTORY_HEIGHT);
	// display the inventory frame
	con.setDefaultForeground(TCODColor(200, 180, 50));
	con.printFramenew(0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT, true, TCOD_BKGND_DEFAULT, "Инвентарь"); //console.hpp add this func
	// display the items with their keyboard shortcut
	con.setDefaultForeground(TCODColor::white);
	int shortcut = 'a';
	int y = 1;
	for (Actor **it = owner->container->inventory.begin(); it != owner->container->inventory.end(); it++)
	{
		Actor *actor = *it;
		con.print(2, y, "(%c) %s", shortcut, actor->name);
		y++;
		shortcut++;
	}
	// blit the inventory console on the root console
	TCODConsole::blit(&con, 0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT, TCODConsole::root, engine.screenWidth / 2 - INVENTORY_WIDTH / 2,
		engine.screenHeight / 2 - INVENTORY_HEIGHT / 2);
	TCODConsole::flush();
	// wait for a key press
	TCOD_key_t key;
	TCODSystem::waitForEvent(TCOD_EVENT_KEY_PRESS, &key, NULL, true);
	if (key.vk == TCODK_CHAR) 
	{
		int actorIndex = key.c - 'a';
		if (actorIndex >= 0 && actorIndex < owner->container->inventory.size())
		{
			return owner->container->inventory.get(actorIndex);
		}
	}
	return NULL;
}

void MonsterAi::load(TCODZip &zip) {
	moveCount = zip.getInt();
}

void MonsterAi::save(TCODZip &zip) {
	zip.putInt(MONSTER);
	zip.putInt(moveCount);
}

void PlayerAi::load(TCODZip &zip) {
}

void PlayerAi::save(TCODZip &zip) {
	zip.putInt(PLAYER);
}

Ai *Ai::create(TCODZip &zip) {
	AiType type = (AiType)zip.getInt();
	Ai *ai = NULL;
	switch (type) {
	case PLAYER: ai = new PlayerAi(); break;
	case MONSTER: ai = new MonsterAi(); break;
	case CONFUSED_MONSTER: ai = new ConfusedMonsterAi(0, NULL); break;
	}
	ai->load(zip);
	return ai;
}

ConfusedMonsterAi::ConfusedMonsterAi(int nbTurns, Ai *oldAi) : nbTurns(nbTurns), oldAi(oldAi) {}

void ConfusedMonsterAi::update(Actor *owner)
{
	TCODRandom *rng = TCODRandom::getInstance();
	int dx = rng->getInt(-1, 1);
	int dy = rng->getInt(-1, 1);
	if (dx != 0 || dy != 0) 
	{
		int destx = owner->x + 2*dx;
		int desty = owner->y + 2*dy;
		if (engine.map->canWalk(destx, desty)) 
		{
			owner->x = destx;
			owner->y = desty;
		}
		else 
		{
			Actor *actor = engine.getActor(destx, desty);
			if (actor) 
			{
				owner->attacker->attack(owner, actor);
			}
		}
	}
	nbTurns--;
	if (nbTurns == 0) 
	{
		owner->ai = oldAi;
		delete this;
	}
}

void ConfusedMonsterAi::load(TCODZip &zip)
{
	nbTurns = zip.getInt();
	oldAi = Ai::create(zip);
}

void ConfusedMonsterAi::save(TCODZip &zip) 
{
	zip.putInt(CONFUSED_MONSTER);
	zip.putInt(nbTurns);
	oldAi->save(zip);
}