#include "pch.h"
#include <vector>
#include <string>
#include <cstdio>
#include <sound/vorbis/codec.h>
#include "SoundItem.h"
#include "types.h"
#include "GGScene.h"

void read_packet(FileResource* fp, std::vector<u8>& buf, ogg_packet& oggp)
{
	int len = fp->read16();
	//if( len > buf.size() ) buf.resize(len);
	fp->read(buf, len);
	oggp.bytes = len;
	oggp.packet = buf.data();
	oggp.packetno++;

	return;
}

bool SoundItem::loadFile(GGScene* scene, const std::string& fname)
{
	std::unique_ptr<FileSystem> fs(scene->fsys->clone());
	std::unique_ptr<FileResource> fp(fs->open(fname));
	if (!fp)
	{
		//TODO: log error
		return false;
	}

	int serial = fp->read32();
	int strle = fp->read32();
	u64 dur = fp->read64();
	int numpackets = fp->read32();
	numpackets -= 3;

	//std::wstring np(winrt::to_hstring(numpackets));
	//np += L" num packets\n";
	//OutputDebugStringW(np.c_str());

	vorbis_info vinfo;
	vorbis_comment vcomm;
	vorbis_info_init(&vinfo);
	vorbis_comment_init(&vcomm);

	ogg_packet oggp = { 0 };
	std::vector<u8> buf(0xffff);

	read_packet(fp.get(), buf, oggp);
	oggp.b_o_s = 1;
	vorbis_synthesis_headerin(&vinfo, &vcomm, &oggp);
	oggp.b_o_s = 0;
	read_packet(fp.get(), buf, oggp);
	vorbis_synthesis_headerin(&vinfo, &vcomm, &oggp);
	read_packet(fp.get(), buf, oggp);
	vorbis_synthesis_headerin(&vinfo, &vcomm, &oggp);

	//todo: error out if vinfo.rate != 44100

	SoundItem* si = this;
	si->bitrate = vinfo.rate;

	vorbis_block vblock;
	vorbis_dsp_state vstate;

	vorbis_synthesis_init(&vstate, &vinfo);
	vorbis_block_init(&vstate, &vblock);

	for (int i = 0; i < numpackets; ++i)
	{
		read_packet(fp.get(), buf, oggp);
		if (!vorbis_synthesis(&vblock, &oggp))
		{
			vorbis_synthesis_blockin(&vstate, &vblock);
			float** channels;
			int num_samples = vorbis_synthesis_pcmout(&vstate, &channels);
			if (num_samples == 0) continue;
			si->samples.reserve(si->samples.size() + num_samples * 2);

			if (vinfo.channels < 2)
			{
				for (int s = 0; s < num_samples; ++s)
				{
					si->samples.push_back(channels[0][s]);
					si->samples.push_back(channels[0][s]);
				}
			}
			else {
				for (int s = 0; s < num_samples; ++s)
				{
					si->samples.push_back(channels[0][s]);
					si->samples.push_back(channels[1][s]);
				}
			}

			scene->addLoadedBytes(num_samples * 8);
			vorbis_synthesis_read(&vstate, num_samples);
		}
		else {
			break; //?
		}
	} //end of vorbis decoder

	vorbis_comment_clear(&vcomm);
	vorbis_dsp_clear(&vstate);
	vorbis_block_clear(&vblock);
	vorbis_info_clear(&vinfo);

	return true;
}



