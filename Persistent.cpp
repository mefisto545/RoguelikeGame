#include "main.hpp"

void LightningBolt::load(TCODZip &zip)
{
	range = zip.getFloat();
	damage = zip.getFloat();
}

void LightningBolt::save(TCODZip &zip)
{
	zip.putInt(LIGHTNING_BOLT);
	zip.putFloat(range);
	zip.putFloat(damage);
}

void Confuser::load(TCODZip &zip)
{
	range = zip.getFloat();
	nbTurns = zip.getInt();
}

void Confuser::save(TCODZip &zip)
{
	zip.putInt(CONFUSER);
	zip.putFloat(range);
	zip.putInt(nbTurns);
}


void FireBall::load(TCODZip &zip)
{
	range = zip.getFloat();
	damage = zip.getFloat();
}

void FireBall::save(TCODZip &zip)
{
	zip.putInt(FIREBALL);
	zip.putFloat(range);
	zip.putFloat(damage);
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

void Actor::save(TCODZip &zip) {
	zip.putInt(x);
	zip.putInt(y);
	zip.putInt(ch);
	zip.putString(name);
	zip.putInt(blocks);

	zip.putInt(attacker != NULL);
	zip.putInt(destructible != NULL);
	zip.putInt(ai != NULL);
	zip.putInt(pickable != NULL);
	zip.putInt(container != NULL);

	if (attacker) attacker->save(zip);
	if (destructible) destructible->save(zip);
	if (ai) ai->save(zip);
	if (pickable) pickable->save(zip);
	if (container) container->save(zip);
}

void Actor::load(TCODZip &zip) {
	x = zip.getInt();
	y = zip.getInt();
	ch = zip.getInt();
	name = _strdup(zip.getString());
	blocks = zip.getInt();

	bool hasAttacker = zip.getInt();
	bool hasDestructible = zip.getInt();
	bool hasAi = zip.getInt();
	bool hasPickable = zip.getInt();
	bool hasContainer = zip.getInt();

	if (hasAttacker) {
		attacker = new Attacker(0.0f);
		attacker->load(zip);
	}

	if (hasDestructible) {
		destructible = Destructible::create(zip);
	}

	if (hasAi) {
		ai = Ai::create(zip);
	}
	if (hasPickable) {
		pickable = Pickable::create(zip);
	}

	if (hasContainer) {
		container = new Container(0);
		container->load(zip);
	}
}

void Attacker::load(TCODZip &zip) {
	power = zip.getFloat();
}

void Attacker::save(TCODZip &zip) {
	zip.putFloat(power);
}

void Container::load(TCODZip &zip)
{
	size = zip.getInt();
	int nbActors = zip.getInt();
	while (nbActors > 0) {
		Actor *actor = new Actor(0, 0, 10, NULL);
		actor->load(zip);
		inventory.push(actor);
		nbActors--;
	}
}

void Container::save(TCODZip &zip)
{
	zip.putInt(size);
	zip.putInt(inventory.size());
	for (Actor **it = inventory.begin(); it != inventory.end(); it++)
	{
		(*it)->save(zip);
	}
}

void Map::save(TCODZip &zip) {
	zip.putInt(seed);
	for (int i = 0; i < width*height; i++) {
		zip.putInt(tiles[i].explored);
	}
}

void Map::load(TCODZip &zip) {
	seed = zip.getInt();
	init(false);
	for (int i = 0; i < width*height; i++) {
		tiles[i].explored = zip.getInt();
	}
}

void Healer::load(TCODZip &zip) {
	amount = zip.getFloat();
}

void Healer::save(TCODZip &zip) {
	zip.putInt(HEALER);
	zip.putFloat(amount);
}

void Gui::load(TCODZip &zip) {
	int nbMessages = zip.getInt();
	while (nbMessages > 0) {
		const char *text = zip.getString();
		TCODColor col = zip.getColor();
		message(col, text);
		nbMessages--;
	}
}

void Gui::save(TCODZip &zip) {
	zip.putInt(log.size());
	for (Message **it = log.begin(); it != log.end(); it++) {
		zip.putString((*it)->text);
		zip.putColor(&(*it)->col);
	}
}


void Destructible::load(TCODZip &zip) {
	maxHp = zip.getFloat();
	hp = zip.getFloat();
	defense = zip.getFloat();
	corpseName = _strdup(zip.getString());
}

void Destructible::save(TCODZip &zip) {
	zip.putFloat(maxHp);
	zip.putFloat(hp);
	zip.putFloat(defense);
	zip.putString(corpseName);
}

void PlayerDestructible::save(TCODZip &zip) {
	zip.putInt(PLAYER);
	Destructible::save(zip);
}

void MonsterDestructible::save(TCODZip &zip) {
	zip.putInt(MONSTER);
	Destructible::save(zip);
}

const int SAVEGAME_VERSION = 0x1100;

void Engine::save() 
{
	if (player->destructible->isDead())
	{
		TCODSystem::deleteFile("game.sav");
	}
	else 
	{
		TCODZip zip;
		zip.putInt(SAVEGAME_VERSION);
		zip.putInt(level);
		// save the map first
		zip.putInt(map->width);
		zip.putInt(map->height);
		map->save(zip);
		// then the player
		player->save(zip);
		// then the stairs
		stairs->save(zip);
		// then all the other actors
		zip.putInt(actors.size() - 2);
		for (Actor **it = actors.begin(); it != actors.end(); it++)
		{
			if (*it != player && *it != stairs) {
				(*it)->save(zip);
			}
		}
		// finally the message log
		gui->save(zip);
		zip.saveToFile("game.sav");
	}
}

const int PAUSE_MENU_WIDTH = 30;
const int PAUSE_MENU_HEIGHT = 15;

void Engine::load(bool pause) 
{
	TCODZip zip;
	engine.gui->menu.clear();
	if (TCODSystem::fileExists("game.sav"))
	{
		zip.loadFromFile("game.sav");
		int version = zip.getInt();
		if (version == SAVEGAME_VERSION)
		{
			engine.gui->menu.addItem(Menu::CONTINUE, "Продолжить");
		}
	}
	engine.gui->menu.addItem(Menu::NEW_GAME, "Новая игра");
	engine.gui->menu.addItem(Menu::OPTIONS, "Управление");
	engine.gui->menu.addItem(Menu::EXIT, "Выход");
	back:
	Menu::MenuItemCode menuItem = engine.gui->menu.pick(pause ? Menu::PAUSE : Menu::MAIN);
	if (menuItem == Menu::EXIT || menuItem == Menu::NONE)
	{
		// Exit or window closed
		exit(0);
	}
	else if (menuItem == Menu::NEW_GAME)
	{
		// New game
		engine.term();
		engine.init();
	}
	else if (menuItem == Menu::OPTIONS)
	{
		TCOD_key_t key;
		for (int x = 20; x < 60; x++)
			for (int y = 10; y < 40; y++)
				TCODConsole::root->setCharBackground(x, y, TCOD_black);
		TCODConsole::root->printFramenew(20, 10, 40, 30, true, TCOD_BKGND_DEFAULT, "Управление");
		TCODConsole::root->print(23, 13, "'E' - использовать\n'I' - инвентарь\n'D' - выбросить\n'G' - подобрать\n'Esc' - меню");
		TCODConsole::flush();
		TCODSystem::waitForEvent(TCOD_EVENT_KEY_PRESS, &key, NULL, true);
		TCODConsole::root->setDefaultForeground(TCOD_white);
		goto back;
	}
	else
	{
		// continue a saved game
		engine.term();
		// load the map
		level = zip.getInt();
		int width = zip.getInt();
		int height = zip.getInt();
		map = new Map(width, height);
		map->load(zip);
		// then the player
		player = new Actor(0, 0, 0, NULL);
		actors.push(player);
		player->load(zip);
		// the stairs
		stairs = new Actor(0, 0, 0, NULL);
		stairs->load(zip);
		actors.push(stairs);
		// then all other actors
		int nbActors = zip.getInt();
		while (nbActors > 0) {
			Actor *actor = new Actor(0, 0, 0, NULL);
			actor->load(zip);
			actors.push(actor);
			nbActors--;
		}
		// finally the message log
		gui->load(zip);
		// to force FOV recomputation
		gameStatus = STARTUP;
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