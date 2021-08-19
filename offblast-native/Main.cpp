# include <Siv3D.hpp> // OpenSiv3D v0.4.3

struct Law {
	String name;
	String url;
	Array<String> haikus;
};

class Haiku {
private:
	String text;
	String font;
	Rect region;

	Transition transition = Transition(1.0s, 1.0s);
	bool living = true;
	
public:
	Haiku(String text,  String font, Rect region) : text(text), font(font), region(region) { }

	void update() {
		this->transition.update(this->living);

		this->transitionHover.update(this->region.mouseOver());
	}

	void draw() {
		int alpha = 255 * this->transition.value();
		FontAsset(this->font)(this->text).draw(this->region.pos, Color(0x33, 0x33, 0x33, alpha));
	}

	Rect getRegion() const {
		return this->region;
	}

	void die() {
		this->living = false;
	}

	bool isDied() const {
		return !this->living && this->transition.isZero();
	}
};

Array<Law> ReadLaws(String filename) {
	const JSONReader json(filename);
	Array<Law> laws;

	for (const auto& obj : json[U"laws"].arrayView()) {
		Law law;
		law.name = obj[U"name"].getString();
		law.url = obj[U"url"].getString();
		law.haikus = obj[U"haikus"].getArray<String>();
		laws << law;
	}

	return laws;
}

String ChoiceRandomHaiku(Array<Law> laws) {
	while (true) {
		Law& law = laws.choice();
		if (law.haikus.size() == 0) continue;

		String& haiku = law.haikus.choice();
		if (haiku.includes(U" 施行する")) continue;

		return haiku.replaced(U" ", U"　");
	}
}

const int fontNum = 4;
const int margin = 20;
Optional<Haiku> NewHaiku(String text, Array<Haiku> others) {
	const String font = U"Haiku{}"_fmt(RandomUint8() % fontNum);
	const Rect rect = FontAsset(font)(text).region();
	
	const int windowWidth = Window::ClientWidth();
	const int windowHeight = Window::ClientHeight();

	for (int i = 0; i <= 10; i++) {
		Point pos = RandomPoint(windowWidth - rect.w, windowHeight - rect.h);
		const Rect region = rect.movedBy(pos);

		bool collision = false;
		for (const auto& other : others) {
			if (region.stretched(margin).intersects(other.getRegion())) {
				collision = true;
				break;
			}
		}
		if (collision) continue;

		Logger.writeln(U"Found: {}, {}, {}"_fmt(text, font, region));
		return Haiku(text, font, region);
	}

	return none;
}

void Main()
{
	// init window
	Scene::SetBackground(Color(240, 236, 229));
	Window::SetFullscreen(true);

	// init font
	int base = 36;
	FontAsset::Register(U"Haiku0", base , U"ipamp.ttf");
	FontAsset::Register(U"Haiku1", base * 1.2, U"ipamp.ttf");
	FontAsset::Register(U"Haiku2", base * 1.5, U"ipamp.ttf");
	FontAsset::Register(U"Haiku3", base * 2.4, U"ipamp.ttf");

	// init haiku
	const Array<Law> laws = ReadLaws(U"api.json");

	Array<Haiku> haikus;
	for (int i = 1; i < 10; i++) {
		String text = ChoiceRandomHaiku(laws);
		Optional<Haiku> haiku = NewHaiku(text, haikus);
		if (haiku.has_value()) {
			haikus << haiku.value();
		}
	}

	// main loop
	Timer timeToDie(5s, true);
	Timer timeToBorn(10s, true);
	while (System::Update())
	{
		if (timeToDie.reachedZero()) {
			haikus[0].die();
			timeToDie.set(10s);
			timeToDie.start();
		}

		haikus.remove_if([](Haiku haiku) { return haiku.isDied(); });

		if (timeToBorn.reachedZero()) {
			String text = ChoiceRandomHaiku(laws);
			Optional<Haiku> haiku = NewHaiku(text, haikus);
			if (haiku.has_value()) {
				haikus << haiku.value();
			}
			timeToBorn.restart();
		}

		for (auto& haiku : haikus) {
			haiku.update();

			haiku.draw();
		}
	}
}

