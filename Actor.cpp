#include "main.hpp"
#include <math.h>

Actor::Actor(int x, int y, int ch, const char *name) : x(x), y(y), ch(ch), name(name), blocks(true), fovOnly(true), attacker(NULL), destructible(NULL),
ai(NULL), pickable(NULL), container(NULL) {}

Actor::~Actor() 
{
	if (attacker) delete attacker;
	if (destructible) delete destructible;
	if (ai) delete ai;
	if (pickable) delete pickable;
	if (container) delete container;
}

void putPic(int x, int y, int c)
{
	TCODConsole::root->putChar(x, y, c);
	TCODConsole::root->putChar(x + 1, y, c + 1);
	TCODConsole::root->putChar(x, y + 1, c + 16);
	TCODConsole::root->putChar(x + 1, y + 1, c + 17);
}

void Actor::render(int cx, int cy)
{
	putPic(x + cx, y + cy, ch);
}

void Actor::update() 
{
	if (ai) ai->update(this);
}

float Actor::getDistance(int cx, int cy) const
{
	int dx = x - cx;
	int dy = y - cy;
	return sqrtf(dx*dx + dy*dy);
}