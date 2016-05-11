#include "main.hpp"

bool Pickable::pick(Actor *owner, Actor *wearer) 
{
	if (wearer->container && wearer->container->add(owner))
	{
		engine.actors.remove(owner);
		return true;
	}
	return false;
}

bool Pickable::use(Actor *owner, Actor *wearer) 
{
	if (wearer->container)
	{
		wearer->container->remove(owner);
		delete owner;
		return true;
	}
	return false;
}

Healer::Healer(float amount) : amount(amount) {}

bool Healer::use(Actor *owner, Actor *wearer)
{
	if (wearer->destructible)
	{
		float amountHealed = wearer->destructible->heal(amount);
		if (amountHealed > 0)
		{
			return Pickable::use(owner, wearer);
		}
	}
	return false;
}

void Healer::load(TCODZip &zip) {
	amount = zip.getFloat();
}

void Healer::save(TCODZip &zip) {
	zip.putInt(HEALER);
	zip.putFloat(amount);
}

Pickable *Pickable::create(TCODZip &zip) {
	PickableType type = (PickableType)zip.getInt();
	Pickable *pickable = NULL;
	switch (type) {
		case HEALER: pickable = new Healer(0); break;
		case LIGHTNING_BOLT: pickable = new LightningBolt(0, 0); break;
		case FIREBALL: pickable = new FireBall(0,0); break;
		case CONFUSER: pickable = new Confuser(0, 0); break;
	}
	pickable->load(zip);
	return pickable;
}

LightningBolt::LightningBolt(float range, float damage) : range(range), damage(damage) {}

bool LightningBolt::use(Actor *owner, Actor *wearer) {
	Actor *closestMonster = engine.getClosestMonster(wearer->x, wearer->y, range);
	if (!closestMonster) {
		engine.gui->message(TCODColor::lightGrey, "Нет никого достаточно близко для удара.");
		return false;
	}
	engine.gui->message(TCODColor::lightBlue,
		"%s ударен молнией!\n"
		"Нанесено %g урона.",
		closestMonster->name, damage);
	closestMonster->destructible->takeDamage(closestMonster, damage);
	return Pickable::use(owner, wearer);
}

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

FireBall::FireBall(float range, float damage) : range(range), damage(damage) {}

bool FireBall::use(Actor *owner, Actor *wearer) {
	engine.gui->message(TCODColor::cyan, "ЛКМ для выбора центра файрбола,\nили ПКМ для отмены.");
	int x, y;
	if (!engine.pickATile(&x, &y, 0)) {
		return false;
	}
	engine.gui->message(TCODColor::orange, "Файрбол взрывается и сжигает всё в радиусе %g клеток!", range/2);
	for (Actor **iterator = engine.actors.begin();
		iterator != engine.actors.end(); iterator++) {
		Actor *actor = *iterator;
		if (actor->destructible && !actor->destructible->isDead()
			&& actor->getDistance(x, y) <= range) {
			engine.gui->message(TCODColor::orange, "%s горит и получает %g очков урона.",
				actor->name, damage);
			actor->destructible->takeDamage(actor, damage);
		}
	}
	return Pickable::use(owner, wearer);
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

Confuser::Confuser(int nbTurns, float range) : nbTurns(nbTurns), range(range) {}

bool Confuser::use(Actor *owner, Actor *wearer) {
	engine.gui->message(TCODColor::cyan, "ЛКМ для сбития противника с толку,\nПКМ для отмены.");
	int x, y;
	if (!engine.pickATile(&x, &y, range)) {
		return false;
	}
	Actor *actor = engine.getActor((x/2)*2, (y/2)*2);
	if (!actor) {
		return false;
	}
	Ai *confusedAi = new ConfusedMonsterAi(nbTurns, actor->ai);
	actor->ai = confusedAi;
	engine.gui->message(TCODColor::lightGreen, "%s выглядит потерянным,\nи он начинает бесцельно бродить!",
		actor->name);
	return Pickable::use(owner, wearer);
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

void Pickable::drop(Actor *owner, Actor *wearer) {
	if (wearer->container) {
		wearer->container->remove(owner);
		engine.actors.push(owner);
		owner->x = wearer->x;
		owner->y = wearer->y;
		engine.gui->message(TCODColor::lightGrey, "%s drops a %s.",
			wearer->name, owner->name);
	}
}