#pragma once
#include "visualobject.h"
#include "sound.h"

class Sprite : public VisualObject {
	public:
		Sprite() = default;
		~Sprite() = default;
		Sound sound;
};
