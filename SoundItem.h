#pragma once
#include <vector>
#include <string>
#include "types.h"

class GGScene;

class SoundItem
{
public:
	bool loadFile(GGScene* g, const std::string& fname);
	std::vector<float>& data() { return samples; }
	int rate() const { return bitrate; }
	SoundItem() : bitrate(0) {}

protected:
	std::vector<float> samples;
	int bitrate;
};

