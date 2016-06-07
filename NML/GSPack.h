#pragma once
#include <inttypes.h>
#include <vector>
#include <algorithm>

class GSPack
{
private:
	struct FileInfoMemory
	{
		std::string path;
		std::vector<char> buffer;

		FileInfoMemory(const FileInfoMemory&) = default;
		FileInfoMemory(FileInfoMemory&& other) :
			path(other.path),
			buffer(std::move(other.buffer))
		{
		}
		FileInfoMemory(std::string path, std::vector<char>&& buffer) :
			path(path),
			buffer(std::move(buffer))
		{
		}

		FileInfoMemory& operator=(FileInfoMemory const&) = default;
	};

	struct FileInfoFile
	{
		uint32_t offset;
		uint32_t length;
		uint8_t path_length;
		char path[1];

		int Size()
		{
			return 4 + 4 + 1 + path_length;
		}
	};

	std::vector<FileInfoMemory> file_list;

public:
	GSPack()
	{
	}

	void AddFile(std::string path, std::vector<char>& buf_in)
	{
		file_list.emplace_back(std::move(path), std::move(buf_in));
	}

	bool GetFile(std::string path, std::vector<char>& buf_out)
	{
		for (auto&& f : file_list)
		{
			if (path == f.path)
			{
				buf_out = std::vector<char>(f.buffer);
				return true;
			}
		}
		return false;
	}

	bool HasFile(std::string path)
	{
		for (auto&& f : file_list)
		{
			if (path == f.path) return true;
		}
		return false;
	}

	void OpenPackage(std::vector<char>& file_in)
	{
		internalBuffer = std::move(file_in);
		file_in.clear();
		Deserialize();
		internalBuffer.clear();
	}

	void OpenPackage(std::string filename)
	{
		FILE* fr;
		fopen_s(&fr, filename.c_str(), "rb");
		fseek(fr, 0, SEEK_END);
		int file_size = (int)ftell(fr);
		fseek(fr, 0, SEEK_SET);

		std::vector<char> data;
		data.resize(file_size);
		fread(data.data(), 1, file_size, fr);
		fclose(fr);

		OpenPackage(data);
	}

	void SavePackage(std::vector<char>& file_out)
	{
		internalBuffer.clear();
		Serialize();
		file_out = std::move(internalBuffer);
	}

	void SavePackage(std::string filename)
	{
		std::vector<char> output;
		SavePackage(output);

		FILE* fw;
		fopen_s(&fw, filename.c_str(), "wb+");
		fwrite(output.data(), 1, output.size(), fw);
		fclose(fw);
	}

	void Merge(const GSPack& other)
	{
		//std::copy(other.file_list.begin(), other.file_list.end(), file_list.end());
		file_list.insert(file_list.end(), other.file_list.begin(), other.file_list.end());
	}

	std::vector<char> internalBuffer;

public:
	struct Iterator
	{
		std::vector<FileInfoMemory>::iterator itor;

		std::string GetPath()
		{
			return itor->path;
		}

		std::vector<char> GetContent()
		{
			return itor->buffer;
		}

		bool operator != (const Iterator& other)
		{
			return itor != other.itor;
		}

		void operator ++()
		{
			++itor;
		}

		Iterator& operator *()
		{
			return *this;
		}
	};

public:
	Iterator begin()
	{
		return { file_list.begin() };
	}

	Iterator end()
	{
		return { file_list.end() };
	}

private:
	void Serialize();
	void Deserialize();

};
