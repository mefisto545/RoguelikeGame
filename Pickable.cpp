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

Pickable *Pickable::create(TCODZip &zip)
{
	PickableType type = (PickableType)zip.getInt();
	Pickable *pickable = NULL;
	switch (type)
	{
	case HEALER: pickable = new Healer(0); break;
	case LIGHTNING_BOLT: pickable = new LightningBolt(0, 0); break;
	case FIREBALL: pickable = new FireBall(0, 0); break;
	case CONFUSER: pickable = new Confuser(0, 0); break;
	}
	pickable->load(zip);
	return pickable;
}

LightningBolt::LightningBolt(float range, float damage) : range(range), damage(damage) {}

bool LightningBolt::use(Actor *owner, Actor *wearer) {
	Actor *closestMonster = engine.getClosestMonster(wearer->x, wearer->y, range + wearer->attacker->intell / 2);
	if (!closestMonster) {
		engine.gui->message(TCODColor::lightGrey, "Нет никого достаточно близко для удара.");
		return false;
	}
	engine.gui->message(TCODColor::lightBlue,
		"%s ударен молнией!\n"
		"Нанесено %g урона.",
		closestMonster->name, damage + wearer->attacker->intell * 5);
	closestMonster->destructible->takeDamage(closestMonster, damage + wearer->attacker->intell * 5);
	return Pickable::use(owner, wearer);
}

FireBall::FireBall(float range, float damage) : range(range), damage(damage) {}

bool FireBall::use(Actor *owner, Actor *wearer) {
	engine.gui->message(TCODColor::cyan, "ЛКМ для выбора центра огненного шара,\nили ПКМ для отмены.");
	int x, y;
	if (!engine.pickATile(&x, &y, 0)) {
		return false;
	}
	engine.gui->message(TCODColor::orange, "Огненный шар взрывается и сжигает всё в радиусе %g клеток!", range + wearer->attacker->intell / 4);
	for (Actor **iterator = engine.actors.begin();
		iterator != engine.actors.end(); iterator++) {
		Actor *actor = *iterator;
		if (actor->destructible && !actor->destructible->isDead()
			&& actor->getDistance(x, y) <= range + wearer->attacker->intell / 4) {
			engine.gui->message(TCODColor::orange, "%s горит и получает %g очков урона.",
				actor->name, damage + wearer->attacker->intell * 3);
			actor->destructible->takeDamage(actor, damage + wearer->attacker->intell * 3);
		}
	}
	return Pickable::use(owner, wearer);
}

Confuser::Confuser(int nbTurns, float range) : nbTurns(nbTurns), range(range) {}

bool Confuser::use(Actor *owner, Actor *wearer) {
	engine.gui->message(TCODColor::cyan, "ЛКМ для сбития противника с толку,\nПКМ для отмены.");
	int x, y;
	if (!engine.pickATile(&x, &y, range + wearer->attacker->intell / 4)) {
		return false;
	}
	Actor *actor = engine.getActor((x / 2) * 2, (y / 2) * 2);
	if (!actor) {
		return false;
	}
	Ai *confusedAi = new ConfusedMonsterAi(nbTurns + wearer->attacker->intell, actor->ai);
	actor->ai = confusedAi;
	engine.gui->message(TCODColor::lightGreen, "%s выглядит потерянным,\nи он начинает бесцельно бродить!",
		actor->name);
	return Pickable::use(owner, wearer);
}

void Pickable::drop(Actor *owner, Actor *wearer)
{
	if (wearer->container) {
		wearer->container->remove(owner);
		engine.actors.push(owner);
		owner->x = wearer->x;
		owner->y = wearer->y;
		engine.gui->message(TCODColor::lightGrey, "%s выбрасывает %s.",
			wearer->name, owner->name);
	}
}