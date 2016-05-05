void putPic(int x, int y, int c);

class Actor 
{
public:
	int x, y; // position on map
	int ch; // ascii code
	const char *name; // the actor's name
	bool blocks; // can we walk on this actor?
	Attacker *attacker; // something that deals damage
	Destructible *destructible; // something that can be damaged
	Ai *ai; // something self-updating

	Actor(int x, int y, int ch, const char *name);
	void update();
	void render() const;
};