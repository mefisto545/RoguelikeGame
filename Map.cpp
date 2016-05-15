#include "main.hpp"

static const int ROOM_MAX_SIZE = 12;
static const int ROOM_MIN_SIZE = 8;
static const int MAX_ROOM_MONSTERS = 3;
static const int MAX_ROOM_ITEMS = 2;

class BspListener : public ITCODBspCallback
{
private:
	Map &map; // a map to dig
	int roomNum; // room number
	int lastx, lasty; // center of the last room
public:
	BspListener(Map &map) : map(map), roomNum(0) {}
	bool visitNode(TCODBsp *node, void *userData)
	{
		if (node->isLeaf())
		{
			int x, y, w, h;
			bool withActors = (bool)userData;
			// dig a room
			w = map.rng->getInt(ROOM_MIN_SIZE, node->w - 2);
			w -= w % 2;
			h = map.rng->getInt(ROOM_MIN_SIZE, node->h - 2);
			h -= h % 2;
			x = map.rng->getInt(node->x + 2, node->x + node->w - w - 1);
			x -= x % 2;
			y = map.rng->getInt(node->y + 2, node->y + node->h - h - 1);
			y -= y % 2;
			map.createRoom(roomNum == 0, x, y, x + w - 2, y + h - 2, withActors);
			if (roomNum != 0)
			{
				// dig a corridor from last room
				map.dig(lastx, lasty + 1, x + w / 2 - (w / 2) % 2 , lasty + 1);
				map.dig(x + w / 2 - (w / 2) % 2, lasty + 1, x + w / 2 - (w / 2) % 2, y + h / 2 - (h/2) % 2 + 1);
			}
			lastx = x + w / 2;
			lasty = y + h / 2;
			lastx -= lastx % 2;
			lasty -= lasty % 2;
			roomNum++;
		}
		return true;
	}
};

Map::Map(int width, int height)	: width(width), height(height) 
{
	seed = TCODRandom::getInstance()->getInt(0, 0x7FFFFFFF);
}

void Map::init(bool withActors) 
{
	rng = new TCODRandom(seed, TCOD_RNG_CMWC);
	tiles = new Tile[(width + 2)*(height + 2)];
	map = new TCODMap(width + 2, height + 2);
	TCODBsp bsp(2, 2, width, height);
	bsp.splitRecursive(rng, 8, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);
	BspListener listener(*this);
	bsp.traverseInvertedLevelOrder(&listener, (void *)withActors);
	for (int i = 0; i<width; i++)
	{
		map->setProperties(1, i, false, false);
		map->setProperties(0, i, false, false);
		map->setProperties(i, 1, false, false);
		map->setProperties(i, 0, false, false);
		map->setProperties(width - 1, i, false, false);
		map->setProperties(width - 2, i, false, false);
		map->setProperties(i, height - 2, false, false);
		map->setProperties(i, height - 2, false, false);
	}
}

Map::~Map()
{
	delete[] tiles;
	delete map;
}

void Map::dig(int x1, int y1, int x2, int y2) 
{
	if (x2 < x1) 
	{
		int tmp = x2;
		x2 = x1;
		x1 = tmp;
	}
	if (y2 < y1) 
	{
		int tmp = y2;
		y2 = y1;
		y1 = tmp;
	}
	for (int tilex = x1; tilex <= x2; tilex+=2) 
	{
		for (int tiley = y1; tiley <= y2; tiley+=2) 
		{
			map->setProperties(tilex, tiley, true, true);
			map->setProperties(tilex+1, tiley, true, true);
			map->setProperties(tilex, tiley-1, true, true);
			map->setProperties(tilex+1, tiley-1, true, true);
		}
	}
}

void Map::createRoom(bool first, int x1, int y1, int x2, int y2, bool withActors) 
{
	x1 -= x1 % 2;
	x2 -= x2 % 2;
	y1 -= y1 % 2 + 1;
	y2 -= y2 % 2 + 1;
	dig(x1, y1, x2, y2);
	if (!withActors)
	{
		return;
	}
	if (first) 
	{
		// put the player in the first room
		engine.player->x = (x1 + x2) / 2 - ((x1 + x2) / 2) % 2;
		engine.player->y = (y1 + y2) / 2 - ((y1 + y2) / 2) % 2;
		if (engine.level == 1)
		{
			engine.scroll->x = (x1 + x2) / 2 - ((x1 + x2) / 2) % 2 + 2;
			engine.scroll->y = (y1 + y2) / 2 - ((y1 + y2) / 2) % 2;
		}
	}
	else
	{
		TCODRandom *rng = TCODRandom::getInstance();
		// add items
		int nbItems = rng->getInt(0, MAX_ROOM_ITEMS);
		while (nbItems > 0)
		{
			int x = rng->getInt(x1, x2);
			int y = rng->getInt(y1, y2);
			x -= x % 2;
			y -= y % 2;
			if (canWalk(x, y) && canWalk(x + 1, y - 1) && canWalk(x + 1, y) && canWalk(x, y - 1) && x < width - 3 && y < height - 3)
			{
				addItem(x, y);
			}
			nbItems--;
		}
		//add monsters
		int nbMonsters = rng->getInt(0, MAX_ROOM_MONSTERS);
		while (nbMonsters > 0) 
		{
			int x = rng->getInt(x1, x2);
			int y = rng->getInt(y1, y2);
			x -= x % 2;
			y -= y % 2;
			if (canWalk(x, y) && canWalk(x + 1, y - 1) && canWalk(x + 1, y) && canWalk(x, y - 1) && x < width - 3 && y < height - 3)
			{
				addMonster(x, y);
			}
			nbMonsters--;
		}
	}
	// set stairs position
	engine.stairs->x = (x1 + x2) / 2 - (x1 + x2) / 2 % 2;
	engine.stairs->y = (y1 + y2) / 2 - (y1 + y2) / 2 % 2;
}

bool Map::isWall(int x, int y) const
{
	return !map->isWalkable(x, y);
}

bool Map::isExplored(int x, int y) const 
{
	return tiles[x + y*width].explored;
}

bool Map::isInFov(int x, int y) const 
{
	if (x < 0 || x >= width || y < 0 || y >= height) 
	{
		return false;
	}
	if (map->isInFov(x, y)) 
	{
		tiles[x + y*width].explored = true;
		return true;
	}
	return false;
}


void Map::computeFov()
{
	map->computeFov(engine.player->x, engine.player->y, engine.fovRadius);
}

void Map::render(int cx, int cy, int screenWidth, int screenHeight)
{
	static const TCODColor darkWall(0, 0, 100);
	static const TCODColor darkGround(50, 50, 150);
	static const TCODColor lightWall(130, 110, 50);
	static const TCODColor lightGround(200, 180, 50);

	for (int x = cx; x < cx + screenWidth; x++)
	{
		for (int y = cy; y < cy + screenHeight - PANEL_HEIGHT; y++)
		{
			if (x >= 0 && x < width && y >= 0 && y < height)
			{
				if (isInFov(x, y))
				{
					TCODConsole::root->setCharBackground(x - cx, y - cy, isWall(x, y) ? lightWall : lightGround);
				}
				else if (isExplored(x, y))
				{
					TCODConsole::root->setCharBackground(x - cx, y - cy, isWall(x, y) ? darkWall : darkGround);
				}
			}
		}
	}
}

bool Map::canWalk(int x, int y) const {
	if (isWall(x, y)) 
	{
		// this is a wall
		return false;
	}
	for (Actor **iterator = engine.actors.begin(); iterator != engine.actors.end(); iterator++)
	{
		Actor *actor = *iterator;
		if (actor->blocks && actor->x == x && actor->y == y) 
		{
			// there is a blocking actor here. cannot walk
			return false;
		}
	}
	return true;
}

void Map::addMonster(int x, int y)
{
	x -= x % 2;
	y -= y % 2;
	TCODRandom *rng = TCODRandom::getInstance();
	switch (engine.level) {
	case 1:
		if (rng->getInt(0, 100) < 75) {
			// create an monster
			Actor *orc = new Actor(x, y, 3860, "Крыса");
			orc->destructible = new MonsterDestructible(5, 0, "Останки крысы", 30);
			orc->attacker = new Attacker(1,0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3894, "Вор");
			troll->destructible = new MonsterDestructible(10, 0, "Труп", 40);
			troll->attacker = new Attacker(2, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	case 2:
		if (rng->getInt(0, 100) < 60) {
			// create an monster
			Actor *orc = new Actor(x, y, 3892, "Разбойник");
			orc->destructible = new MonsterDestructible(10, 2, "Изуродованное тело", 50);
			orc->attacker = new Attacker(3, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3900, "Сектант");
			troll->destructible = new MonsterDestructible(10, 0, "Останки сектанта", 40);
			troll->attacker = new Attacker(5, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	case 3:
		if (rng->getInt(0, 100) < 80) {
			// create an monster
			Actor *orc = new Actor(x, y, 3858, "Злобный орк");
			orc->destructible = new MonsterDestructible(10, 0, "труп орка", 40);
			orc->attacker = new Attacker(3, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3866, "Толстый тролль");
			troll->destructible = new MonsterDestructible(30, 0, "Останки тролля", 60);
			troll->attacker = new Attacker(5, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	case 4:
		if (rng->getInt(0, 100) < 80) {
			// create an monster
			Actor *orc = new Actor(x, y, 3890, "Зомби");
			orc->destructible = new MonsterDestructible(20, 1, "Павший зомби", 55);
			orc->attacker = new Attacker(5, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3902, "Гигантский паук");//
			troll->destructible = new MonsterDestructible(50, 0, "Тело паука", 100);
			troll->attacker = new Attacker(4, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	case 5:
		if (rng->getInt(0, 100) < 50) {
			// create an monster
			Actor *orc = new Actor(x, y, 3888, "Скелет");//
			orc->destructible = new MonsterDestructible(10, 0, "Кости", 60);
			orc->attacker = new Attacker(6, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3930, "Бес");//
			troll->destructible = new MonsterDestructible(20, 1, "Труп беса", 70);
			troll->attacker = new Attacker(4, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	case 6:
		if (rng->getInt(0, 100) < 50) {
			// create an monster
			Actor *orc = new Actor(x, y, 3864, "Скелет-боец");
			orc->destructible = new MonsterDestructible(15, 5, "Кости", 100);
			orc->attacker = new Attacker(10, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3954, "Минотавр");//
			troll->destructible = new MonsterDestructible(60, 0, "Останки минотавра", 120);
			troll->attacker = new Attacker(6, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	case 7:
		if (rng->getInt(0, 100) < 20) {
			// create an monster
			Actor *orc = new Actor(x, y, 3932, "Циклоп");//
			orc->destructible = new MonsterDestructible(90, 1, "Тело циклопа", 150);
			orc->attacker = new Attacker(8, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3920, "Голем");//
			troll->destructible = new MonsterDestructible(30, 5, "Обломки голема", 30);
			troll->attacker = new Attacker(3, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	case 8:
		if (rng->getInt(0, 100) < 50) {
			// create an monster
			Actor *orc = new Actor(x, y, 3922, "Чёрт");//
			orc->destructible = new MonsterDestructible(15, 0, "Тело чёрта", 50);
			orc->attacker = new Attacker(6, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3924, "Цербер");//
			troll->destructible = new MonsterDestructible(32, 5, "Тело цербера", 200);
			troll->attacker = new Attacker(12, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	case 9:
		if (rng->getInt(0, 100) < 85) {
			// create an monster
			Actor *orc = new Actor(x, y, 3928, "Адский гвардеец");//
			orc->destructible = new MonsterDestructible(150, 10, "Тело гвардейца", 400);
			orc->attacker = new Attacker(20, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3926, "Злобоглаз");//
			troll->destructible = new MonsterDestructible(50, 0, "Останки злобоглаза", 100);
			troll->attacker = new Attacker(8, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	default:
		if (rng->getInt(0, 100) < 85) {
			// create an monster
			Actor *orc = new Actor(x, y, 3928, "Адский гвардеец");//
			orc->destructible = new MonsterDestructible(150, 10, "Тело гвардейца", 400);
			orc->attacker = new Attacker(20, 0);
			orc->ai = new MonsterAi();
			engine.actors.push(orc);
		}
		else
		{
			// create another monster
			Actor *troll = new Actor(x, y, 3926, "Злобоглаз");//
			troll->destructible = new MonsterDestructible(50, 0, "Останки злобоглаза", 100);
			troll->attacker = new Attacker(8, 0);
			troll->ai = new MonsterAi();
			engine.actors.push(troll);
		}
		break;
	}
}

void Map::addItem(int x, int y)
{
	TCODRandom *rng = TCODRandom::getInstance();
	int dice = rng->getInt(0, 100);
	if (dice < 70) {
				// create a health potion
			Actor *healthPotion = new Actor(x, y, 3862, "Зелье здоровья");
		healthPotion->blocks = false;
		healthPotion->pickable = new Healer(4);
		engine.actors.push(healthPotion);
	}
	else if (dice < 70 + 10) {
				// create a scroll of lightning bolt 
			Actor *scrollOfLightningBolt = new Actor(x, y, 3870, "'Молния'");
		scrollOfLightningBolt->blocks = false;
		scrollOfLightningBolt->pickable = new LightningBolt(10, 20);
		engine.actors.push(scrollOfLightningBolt);
	}
	else if (dice < 70 + 10 + 10) {
				// create a scroll of fireball
			Actor *scrollOfFireball = new Actor(x, y, 3870, "'Огненный шар'");
		scrollOfFireball->blocks = false;
		scrollOfFireball->pickable = new FireBall(6, 12);
		engine.actors.push(scrollOfFireball);
	}
	else
		 {
				// create a scroll of confusion
		Actor *scrollOfConfusion = new Actor(x, y, 3870, "'Обескураживание'");
		scrollOfConfusion->blocks = false;
		scrollOfConfusion->pickable = new Confuser(10, 8);
		engine.actors.push(scrollOfConfusion);
		}
}