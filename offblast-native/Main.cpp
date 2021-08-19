# include <Siv3D.hpp> // OpenSiv3D v0.4.3

struct Law {
	String name;
	String url;
	Array<String> haikus;
};

struct HaikuDetails {
	String text;
	String lawName;
};

class Haiku {
private:
	String font;
	Rect region;

	Transition transition = Transition(1.0s, 1.0s);
	bool living = true;

	Transition transitionHover = Transition(0.4s, 0.2s);

public:
	HaikuDetails details;

	Haiku(HaikuDetails details, String font, Rect region) : details(details), font(font), region(region) { }

	void update() {
		this->transition.update(this->living);

		if (this->region.mouseOver()) {
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		this->transitionHover.update(this->region.mouseOver());
	}

	void draw() const {
		int red = 0x33 + 0x99 * this->transitionHover.value();
		int alpha = 255 * this->transition.value();
		FontAsset(this->font)(this->details.text).draw(this->region.pos, Color(red, 0x33, 0x33, alpha));
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

	bool isSelected() const {
		return this->region.leftClicked();
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

HaikuDetails ChoiceRandomHaiku(Array<Law> laws) {
	while (true) {
		Law& law = laws.choice();
		if (law.haikus.size() == 0) continue;

		String& haiku = law.haikus.choice();
		if (haiku.includes(U" 施行する")) continue;

		String text = haiku.replaced(U" ", U"　");
		return { text, law.name };
	}
}

const int fontNum = 4;
const int margin = 20;
Optional<Haiku> NewHaiku(HaikuDetails details, Array<Haiku> others) {
	const String font = U"Haiku{}"_fmt(RandomUint8() % fontNum);
	const Rect rect = FontAsset(font)(details.text).region();
	
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

		Logger.writeln(U"Found: {}, {}, {}"_fmt(details.text, font, region));
		return Haiku(details, font, region);
	}

	return none;
}

void Main()
{
	// init window
	Scene::SetBackground(Color(240, 236, 229));
	Window::SetFullscreen(true);
	System::SetTerminationTriggers(UserAction::None);

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
		HaikuDetails details = ChoiceRandomHaiku(laws);
		Optional<Haiku> haiku = NewHaiku(details, haikus);
		if (haiku.has_value()) {
			haikus << haiku.value();
		}
	}

	// main loop
	bool showModal = false;
	Optional<HaikuDetails> viewingHaiku;

	Timer timeToDie(8s, true);
	while (System::Update())
	{
		for (auto& haiku : haikus) {
			haiku.update();

			haiku.draw();
		}

		if (showModal) {
			HaikuDetails details = viewingHaiku.value();

			Scene::Rect().draw(Color(0, 0, 0, 128));
			Rect(Arg::center(Scene::Center()), Scene::Width(), 400).draw(Color(240, 236, 229));
			const Color color = Color(0x33, 0x33, 0x33);
			FontAsset(U"Haiku1")(details.lawName).drawAt(Scene::Center().movedBy(0, 50), color);
			FontAsset(U"Haiku3")(details.text).drawAt(Scene::Center().movedBy(0, -50), color);

			if (Scene::Rect().leftClicked()) {
				showModal = false;
			}
			continue;
		}

		for (auto& haiku : haikus) {
			if (haiku.isSelected()) {
				showModal = true;
				viewingHaiku = haiku.details;
			}
		}

		if (timeToDie.reachedZero()) {
			haikus[0].die();
			timeToDie.restart();
		}

		haikus.remove_if([](Haiku haiku) { return haiku.isDied(); });

		bool needMoreHaiku = haikus.size() < 10 && RandomUint8() % 100 == 0;
		if (needMoreHaiku) {
			HaikuDetails details = ChoiceRandomHaiku(laws);
			Optional<Haiku> haiku = NewHaiku(details, haikus);
			if (haiku.has_value()) {
				haikus << haiku.value();
			}
		}
	}
}

