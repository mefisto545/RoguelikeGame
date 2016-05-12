#include <math.h>
#include "main.hpp"

Engine::Engine(int screenWidth, int screenHeight) : gameStatus(STARTUP), player(NULL), map(NULL), fovRadius(15),
screenWidth(screenWidth), screenHeight(screenHeight), level(1)
{
	TCODConsole::setCustomFont("BigMainFont.png", TCOD_FONT_LAYOUT_ASCII_INROW | TCOD_FONT_TYPE_GREYSCALE, 16, 256);
	TCODConsole::initRoot(screenWidth, screenHeight, "Test window", false);
	TCODMouse::showCursor(true);
	gui = new Gui();
}

void Engine::init()
{
	player = new Actor(0, 0, 3856, "Игрок");
	player->destructible = new PlayerDestructible(1000, 2, "Твои останки"); //heal and armor
	player->attacker = new Attacker(5);
	player->ai = new PlayerAi();
	player->container = new Container(26);
	actors.push(player);
	scroll = new Actor(0, 0, 3898, "Письмо");
	scroll->blocks = false;
	scroll->fovOnly = false;
	engine.actors.push(scroll);
	stairs = new Actor(0, 0, 3896, "Лестница");//change the symbol
	stairs->blocks = false;
	stairs->fovOnly = false;
	actors.push(stairs);
	map = new Map(100, 100);
	map->init(true);
	gui->message(TCODColor::darkerYellow, "Приветствую тебя, странник, в этом мрачном подеземелье...");
	gameStatus = STARTUP;
}

Engine::~Engine() 
{
	term();
	delete gui;
}

void Engine::term()
{
	actors.clearAndDelete();
	if (map) delete map;
	gui->clear();
}

void Engine::update()
{
	if (gameStatus == STARTUP) map->computeFov();
	gameStatus = IDLE;
	TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS | TCOD_EVENT_MOUSE, &lastKey, &mouse);
	if (lastKey.vk == TCODK_ESCAPE) 
	{
		save();
		load();
	}
	player->update();
	if (gameStatus == NEW_TURN)
	{
		for (Actor **iterator = actors.begin(); iterator != actors.end();
			iterator++) {
			Actor *actor = *iterator;
			if (actor != player) {
				actor->update();
			}
		}
	}
}

void Engine::render() 
{
	TCODConsole::root->clear();
	// draw the map
	map->render(player->x - screenWidth / 2, player->y - screenHeight / 2, screenWidth, screenHeight);
	// draw the actors
	for (Actor **iterator = actors.begin(); iterator != actors.end(); iterator++) 
	{
		Actor *actor = *iterator;
		if (actor != player && ((!actor->fovOnly && map->isExplored(actor->x, actor->y)) || map->isInFov(actor->x, actor->y)) && actor->pickable)
		{
			actor->render(-player->x + screenWidth / 2, -player->y + screenHeight / 2);
		}
	}
	for (Actor **iterator = actors.begin(); iterator != actors.end(); iterator++)
	{
		Actor *actor = *iterator;
		if (map->isInFov(actor->x, actor->y) && !actor->pickable)
		{
			actor->render(-player->x + screenWidth / 2, -player->y + screenHeight / 2);
		}
	}
	player->render(-player->x + screenWidth / 2, -player->y + screenHeight / 2);
	// show the player's stats
	gui->render(player->x - screenWidth / 2, player->y - screenHeight / 2);
	TCODConsole::root->print(0, screenHeight - 3, "mX=%d mY=%d", mouse.cx, mouse.cy);
}

void Engine::sendToBack(Actor *actor)
{
	actors.remove(actor);
	actors.insertBefore(actor, 0);
}


Actor *Engine::getActor(int x, int y) const {
	for (Actor **iterator = actors.begin();
		iterator != actors.end(); iterator++) {
		Actor *actor = *iterator;
		if (actor->x == x && actor->y == y && actor->destructible
			&& !actor->destructible->isDead()) {
			return actor;
		}
	}
	return NULL;
}

Actor *Engine::getClosestMonster(int x, int y, float range) const 
{
	Actor *closest = NULL;
	float bestDistance = 1E6f;
	for (Actor **iterator = actors.begin();
		iterator != actors.end(); iterator++) 
	{
		Actor *actor = *iterator;
		if (actor != player && actor->destructible
			&& !actor->destructible->isDead())
		{
			float distance = actor->getDistance(x, y);
			if (distance < bestDistance && (distance <= range || range == 0.0f))
			{
				bestDistance = distance;
				closest = actor;
			}
		}
	}
	return closest;
}

bool Engine::pickATile(int *x, int *y, float maxRange)
{
	while (!TCODConsole::isWindowClosed())
	{
		render();
		// highlight the possible range
		for (int cx = 0; cx < map->width; cx++) {
			for (int cy = 0; cy < map->height; cy++) {
				if (map->isInFov(cx, cy)
					&& (maxRange == 0 || player->getDistance(cx, cy) <= maxRange)) {
					TCODColor col = TCODConsole::root->getCharBackground(cx - (player->x - screenWidth / 2), cy - (player->y - screenHeight / 2));
					col = col * 1.2f;
					TCODConsole::root->setCharBackground(cx - (player->x - screenWidth / 2), cy - (player->y - screenHeight / 2), col);
				}
			}
		}
		TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS | TCOD_EVENT_MOUSE, &lastKey, &mouse);
		if (map->isInFov(mouse.cx + (player->x - screenWidth / 2), mouse.cy + (player->y - screenHeight / 2))
			&& (maxRange == 0 || player->getDistance(mouse.cx + (player->x - screenWidth / 2), mouse.cy + (player->y - screenHeight / 2)) <= maxRange)) {
			TCODConsole::root->setCharBackground(mouse.cx, mouse.cy, TCODColor::white);
			if (mouse.lbutton_pressed) {
				*x = mouse.cx + (player->x - screenWidth / 2);
				*y = mouse.cy + (player->y - screenHeight / 2);
				return true;
			}
		}
		if (mouse.rbutton_pressed || lastKey.vk != TCODK_NONE) {
			return false;
		}
		TCODConsole::flush();
	}
	return false;
}

void Engine::nextLevel()
{
	level++;
	gui->message(TCODColor::red, "Спускаясь всё глубже в позмелье,\nвы сильнее ощущаете приближение зла...");
	delete map;
	// delete all actors but player and stairs
	for (Actor **it = actors.begin(); it != actors.end(); it++)
	{
		if (*it != player && *it != stairs && *it != scroll) 
		{
			delete *it;
			it = actors.remove(it);
		}
	}
	// create a new map
	if (level != 2)
	{
		map = new Map(100, 100);
		map->init(true);
		gameStatus = STARTUP;
	}
	else
	{
		map = new Map(100, 160);
		map->init(false);
		for (int tilex = 0; tilex < 100; tilex++) //tile in the middle room
			for (int tiley = 0; tiley < 160; tiley++)
				map->map->setProperties(tilex, tiley, false, false);
		map->dig(10, 8, 54, 8); //the highest space
		map->dig(2, 6, 60, 24); // space with boss
		map->dig(10, 24, 52, 48); // the middle room
		map->dig(19, 48, 21, 50);// the corridor
		map->dig(16, 50, 46, 60); //the last room
		for (int tilex = 28; tilex <= 46; tilex += 2) //tile in the middle room
		{
			for (int tiley = 30; tiley <= 32; tiley += 2)
			{
				map->map->setProperties(tilex, tiley, false, false);
				map->map->setProperties(tilex + 1, tiley, false, false);
				map->map->setProperties(tilex, tiley - 1, false, false);
				map->map->setProperties(tilex + 1, tiley - 1, false, false);
			}
		}
		engine.scroll->x = player->x + 2;
		engine.scroll->y = player->y + 2;
	}
}
