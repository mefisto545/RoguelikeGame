class Attacker : public Persistent
{
public:
	float power; // hit points given
	float intell;
	Attacker(float power, float intell);
	void attack(Actor *owner, Actor *target);
	void load(TCODZip &zip);
	void save(TCODZip &zip);
};