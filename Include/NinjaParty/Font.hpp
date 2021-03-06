#ifndef NINJAPARTY_FONT_HPP
#define NINJAPARTY_FONT_HPP

#include <algorithm>
#include <map>
#include <stdint.h>
#include <string>

#include <NinjaParty/Math.hpp>

namespace NinjaParty
{
	struct CharacterData
	{
		int16_t x, y;
		uint16_t width, height;
		int16_t offsetX, offsetY;
		int16_t advanceX;
	};

	struct Font
	{
		std::map<int16_t, CharacterData> characters;
		int size, lineHeight;
	};
	
	NinjaParty::Vector2 MeasureString(Font *font, const std::string &string);
	void MeasureString(Font *font, const std::string &string, NinjaParty::Vector2 &maximum);
	std::string WordWrapString(Font *font, const std::string &string, float width);
}

#endif//NINJAPARTY_FONT_HPP
