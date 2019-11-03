#pragma once
#include <vector>
#include <string>
#include <stdio.h>
#include <unordered_map>
#include "types.h"

class FileResource;

class FileSystem
{
public:
	virtual ~FileSystem() {}

	virtual FileResource* open(const std::string& filename) = 0;
	virtual FileResource* open_subresource(const std::string& name, const std::string& subsrc) = 0;

	virtual FileSystem* clone() const = 0;
};

struct CompressedFileInfo
{
	std::string name;
	int flags;
	bool is_folder;
	int header_offset;
	int data_stream_offset;
	int comp_size, uncomp_size;
	int comp_method;
};

class ZipFileSystem : public FileSystem
{
public:
	virtual FileResource* open(const std::string& filename) override;
	virtual FileResource* open_subresource(const std::string& name, const std::string& subsrc) override;

	virtual FileSystem* clone() const override;

	virtual ~ZipFileSystem() { fclose(archive_fp); return; }

	static FileSystem* open_zip(const std::wstring& filename);
protected:
	std::wstring archive_filename;
	FILE* archive_fp;
	std::vector<CompressedFileInfo> files;
	int central_dir_offset;
	int total_files;

};

class NativeFileSystem : public FileSystem
{
public:
	virtual FileResource* open(const std::string& filename) override;
	virtual FileResource* open_subresource(const std::string& name, const std::string& subsrc) override;

	static FileSystem* get();
	virtual FileSystem* clone() const override { return get(); }
};

class FileResource
{
public:
	virtual FileResource* open_subresource(const std::string& filename)
	{
		return from_fs->open_subresource(m_name, filename);
	}

	virtual ~FileResource() {}

	virtual int getc() = 0;
	virtual u16 read16() = 0;
	virtual u32 read32() = 0;
	virtual u64 read64() = 0;
	virtual bool read(std::vector<u8>& bytes, int len = -1) = 0;
	virtual int pos() = 0;
	virtual void seek(int) = 0;
	virtual void skip(int) = 0;
	virtual void ungetc(int) = 0;
	virtual bool eof() = 0;
	virtual std::vector<char>& read_all() = 0;
	virtual int size() const = 0;
	std::string name() const { return m_name; }
	FileSystem* get_filesystem() const { return from_fs; }

	virtual void close() = 0;
protected:
	FileSystem* from_fs = nullptr;
	std::string m_name;
};

class ZipFileResource : public FileResource
{
public:
	virtual FileResource* open_subresource(const std::string& filename) override
	{
		return from_fs->open_subresource(file_info.name, filename);
	}

	virtual void close() override { return; }

	virtual int getc() override { return eof() ? -1 : stream[fpos++]; }

	// I had been using ungetc only for a parser that only ever ungets the
	// same char that had originally been read
	virtual void ungetc(int) override { if (fpos > 0) fpos--; return; }

	virtual u16 read16() override
	{
		if (fpos + 2 >= stream.size()) return 0;
		u16 a = *(u16*)(stream.data() + fpos);
		fpos += 2;
		return a;
	}

	virtual u32 read32() override
	{
		if (fpos + 4 >= stream.size()) return 0;
		u32 a = *(u32*)(stream.data() + fpos);
		fpos += 4;
		return a;
	}

	virtual u64 read64() override
	{
		if (fpos + 8 >= stream.size()) return 0;
		u64 a = *(u64*)(stream.data() + fpos);
		fpos += 8;
		return a;
	}

	virtual bool read(std::vector<u8>& bytes, int len) override
	{
		int size = (len < 1) ? (int)bytes.size() : len;
		if (fpos + size >= stream.size()) size = (int)stream.size() - fpos;
		memcpy(bytes.data(), stream.data() + fpos, size);
		fpos += size;
		return true;
	}

	virtual bool eof() override { return fpos >= file_info.uncomp_size; }
	virtual std::vector<char>& read_all() override { return stream; }
	virtual int size() const override { return file_info.uncomp_size; }

	virtual int pos() override { return fpos; }
	virtual void seek(int p) override { fpos = (p < 0) ? file_info.uncomp_size + p : p; return; }
	virtual void skip(int p) override { fpos += p; return; }

	friend class ZipFileSystem;
protected:
	ZipFileResource(FILE* arch, const CompressedFileInfo& fi, std::vector<char>& dat)
		: archive_fp(arch), file_info(fi), stream(dat), fpos(0) {}

	FILE* archive_fp;
	CompressedFileInfo file_info;
	std::vector<char> stream;
	int fpos;
};

class NativeFileResource : public FileResource
{
public:
	NativeFileResource(const std::string fname, FILE* ff) : fp(ff)
	{
		m_name = fname;
		from_fs = NativeFileSystem::get();
		fseek(fp, 0, SEEK_END);
		file_size = (int)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		return;
	}

	virtual ~NativeFileResource() { fclose(fp); return; }

	virtual void close() override { fclose(fp); return; }

	virtual int getc() override { return fgetc(fp); }
	virtual void ungetc(int c) override { ::ungetc(c, fp); return; }
	virtual bool eof() override { return feof(fp); }
	virtual int size() const override { return file_size; }

	virtual u16 read16() override
	{
		u16 a;
		fread(&a, 1, 2, fp);
		return a;
	}

	virtual u32 read32() override
	{
		u32 a;
		size_t unu = fread(&a, 1, 4, fp);
		return a;
	}

	virtual u64 read64() override
	{
		u64 a;
		size_t unu = fread(&a, 1, 8, fp);
		return a;
	}

	virtual bool read(std::vector<u8>& bytes, int len = -1) override
	{
		size_t size = (len < 1) ? bytes.size() : (size_t)len;
		return size == fread(bytes.data(), 1, size, fp);
	}

	virtual int pos() override { return ftell(fp); }
	virtual void seek(int offs) override { fseek(fp, offs, (offs < 0) ? SEEK_END : SEEK_SET); return; }
	virtual void skip(int offs) override { fseek(fp, offs, SEEK_CUR); return; }

	virtual std::vector<char>& read_all() override;

protected:
	std::vector<char> fdata;
	FILE* fp;
	int file_size;
};


#pragma pack(push, 1)
struct zip_file_header
{
	u32 magic;
	u16 version;
	u16 flags;
	u16 comp_method;
	u16 last_mod_time;
	u16 last_mod_date;
	u32 crc32;
	u32 comp_size;
	u32 uncomp_size;
	u16 file_name_len;
	u16 extra_data_len;
};

struct zip_end_header
{
	u32 magic;
	u16 this_disk;
	u16 directory_start_disk;
	u16 num_files;
	u16 total_files;
	u32 directory_size;
	u32 directory_offset;
	u16 comment_length;
};
#pragma pack(pop)



