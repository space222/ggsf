#include "pch.h"
#include <iostream>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <memory>
#include "miniz.h"
#include "nczlib.h"

FileResource* NativeFileSystem::open(const std::string& filename)
{
	FILE* fp = fopen(filename.c_str(), "rb");
	if (!fp) return nullptr;

	return new NativeFileResource(filename, fp);
}

FileResource* NativeFileSystem::open_subresource(const std::string& name, const std::string& subsrc)
{


	return nullptr;
}

FileSystem* NativeFileSystem::get()
{
	static NativeFileSystem* nfs = new NativeFileSystem;
	return nfs;
}

std::vector<char>& NativeFileResource::read_all()
{
	if (fdata.size() == 0)
	{
		seek(0);
		fdata.resize(size());
		size_t unu = fread(fdata.data(), 1, fdata.size(), fp);
	}

	return fdata;
}

FileSystem* ZipFileSystem::open_zip(const std::wstring& filename)
{
	FILE* fp = _wfopen(filename.c_str(), L"rb");
	if (!fp)
	{
		//todo: log error
		return nullptr;
	}

	// set up scan for file footer ("end of central directory record")
	fseek(fp, 0, SEEK_END);
	size_t fsize = ftell(fp);

	int a = 0x69;
	int offset = -21;  // the structure is at least 22 bytes long, so don't need to start at actual end of file

	// looking for a == signature and the archive comment length is only 16bits
	while (a != 0x06054b50 && -offset < 0x10000)
	{
		offset -= 1;
		fseek(fp, offset, SEEK_END);
		size_t unu = fread(&a, 1, 4, fp);
	}

	// if 'a' isn't the signature, the file isn't a zip archive
	if (a != 0x06054b50)
	{
		//log error, not a zip
		//std::cout << "File is not an archive\n";
		fclose(fp);
		return nullptr;
	}

	// back up to include signature & read in struct
	fseek(fp, -4, SEEK_CUR);
	zip_end_header zeh{};
	size_t unu = fread(&zeh, 1, 22, fp);

	// seek to the primary directory
	fseek(fp, zeh.directory_offset, SEEK_SET);

	// its time to read the initial list of files
	// currently includes folders that get listed but have no actual
	// data other than 2(!?) headers (here and the actual file header)
	std::vector<CompressedFileInfo> files(zeh.total_files);
	for (int i = 0; i < zeh.total_files; ++i)
	{
		CompressedFileInfo& file_info = files[i];

		fseek(fp, 20, SEEK_CUR);
		unu = fread(&file_info.comp_size, 1, 4, fp);
		unu = fread(&file_info.uncomp_size, 1, 4, fp);

		u16 fname_len;
		unu = fread(&fname_len, 1, 2, fp);

		u16 len1, len2;
		unu = fread(&len1, 1, 2, fp); // comments and extra data will be ignored
		unu = fread(&len2, 1, 2, fp); // but need them later
		fseek(fp, 8, SEEK_CUR);

		unu = fread(&file_info.header_offset, 1, 4, fp);
		file_info.name.resize(fname_len, ' ');
		unu = fread(file_info.name.data(), 1, fname_len, fp);

		/*		for(int i = 0; i < fname_len; ++i)
				{
					char c;
					unu = fread(&c,1,1,fp);
					file_info.name += c;
				}
		*/
		fseek(fp, len1 + len2, SEEK_CUR); // skip the comments + extra data

		// hopefully nothing else here?
	}

	// time to go through the files themselves and check some things
	// and get the actual offset of the data stream
	for (int i = 0; i < zeh.total_files; ++i)
	{
		CompressedFileInfo& file = files[i];
		fseek(fp, file.header_offset, SEEK_SET);
		zip_file_header head;
		unu = fread(&head, 1, sizeof(head), fp);

		file.comp_size = (file.comp_size > head.comp_size) ? file.comp_size : head.comp_size;
		file.uncomp_size = (file.uncomp_size > head.uncomp_size) ? file.uncomp_size : head.uncomp_size;
		file.flags = head.flags;
		file.comp_method = head.comp_method;
		file.data_stream_offset = file.header_offset + sizeof(zip_file_header) + head.file_name_len + head.extra_data_len;
	}

	ZipFileSystem* zfs = new ZipFileSystem;
	zfs->archive_filename = filename;
	zfs->archive_fp = fp;
	zfs->central_dir_offset = zeh.directory_offset;
	zfs->total_files = zeh.total_files;
	zfs->files = files;
	return zfs;
}

FileSystem* ZipFileSystem::clone() const
{
	ZipFileSystem* zfs = new ZipFileSystem;
	zfs->archive_filename = this->archive_filename;
	zfs->archive_fp = _wfopen(this->archive_filename.c_str(), L"rb");
	zfs->central_dir_offset = this->central_dir_offset;
	for (const CompressedFileInfo& fi : this->files)
	{
		zfs->files.push_back(fi);
	}
	return zfs;
}

FileResource* ZipFileSystem::open(const std::string& filename)
{
	auto iter = std::find_if(std::begin(files), std::end(files), [&](const auto& i) { return i.name == filename; });
	if (iter == std::end(files))
	{
		//todo: log error, file not in archive
		return nullptr;
	}
	CompressedFileInfo& info = *iter;

	if (info.uncomp_size == 0)
	{
		// only way to know if file is directory/folder?
		return nullptr;
	}

	// seek to the actual data stream
	fseek(archive_fp, info.data_stream_offset, SEEK_SET);
	std::vector<char> data(info.uncomp_size);
	size_t unu; // somewhere to silence fread warning

	if (info.comp_method == 8)
	{
		std::vector<char> deflate(info.comp_size);
		unu = fread(deflate.data(), 1, deflate.size(), archive_fp);

		if (0 > tinfl_decompress_mem_to_mem(data.data(), info.uncomp_size, deflate.data(), info.comp_size, 0))
		{
			//log decompress error
			return nullptr;
		}
	}
	else {
		// only supporting 8 (plain DEFLATE) and 0 (not compressed)
		unu = fread(data.data(), 1, data.size(), archive_fp);
	}

	return new ZipFileResource(archive_fp, info, data);
}

FileResource* ZipFileSystem::open_subresource(const std::string& filename, const std::string& subsrc)
{

	return nullptr;
}





