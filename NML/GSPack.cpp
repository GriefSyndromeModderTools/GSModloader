#include "stdafx.h"
#include "GSPack.h"

namespace
{

	struct encrypt {
		unsigned int* p;
		unsigned int c;
		unsigned int arr[0x270];
	};

	unsigned int inner1(unsigned int a, unsigned int b, unsigned int m) {
		unsigned int i = (b ^ a) & 0x7FFFFFFE;
		i = (i ^ a) >> 1;
		return  i ^ ((b & 1) ? 0x9908B0DF : 0) ^ m;
	}

	unsigned int inner2(encrypt& e) {
		if ((--e.c) == 0) {
			e.p = e.arr;
			e.c = 0x270;

			int i = 0;
			for (; i < 0xE3; ++i) {
				e.arr[i] = inner1(e.arr[i], e.arr[i + 1], e.arr[i + 0x18D]);
			}
			for (; i < 0x26F; ++i) {
				e.arr[i] = inner1(e.arr[i], e.arr[i + 1], e.arr[i - 0xE3]);
			}
			e.arr[0x26F] = inner1(e.arr[0x26F], e.arr[0], e.arr[i - 0xE3]);
		}
		return *(e.p++);
	}

	char inner(encrypt& e) {
		unsigned int ret = inner2(e);
		ret ^= (ret >> 0x0B);
		ret ^= (ret & 0xFF3A58AD) << 7;
		ret ^= (ret & 0xFFFFDF8C) << 0x0F;
		return ret ^ (ret >> 0x12);
	}

	void fill_decrypt_array(unsigned int* arr, int len_filetable) {
		arr[0] = len_filetable + 6;
		for (int i = 1; i < 0x270; ++i) {
			unsigned int medi = arr[i - 1];
			arr[i] = ((medi >> 0x1E) ^ medi) * 0x6C078965 + i;
		}
	}
}

namespace
{
	void do_encrypt_first(char* buf, int len)
	{
		encrypt stack;
		stack.p = 0;
		stack.c = 1;

		fill_decrypt_array(stack.arr, len);

		for (int i = 0; i < len; ++i) {
			buf[i] ^= inner(stack);
		}
	}

	void do_encrypt_second(char* buf, int len)
	{
		unsigned char mcl = 0xC5;
		unsigned char mdl = 0x89;

		for (int i = 0; i < len; ++i) {
			buf[i] ^= mcl;
			mcl += mdl;
			mdl += 0x49;
		}
	}

	std::vector<char> do_encrypt_block(char* buf, int offset, int length)
	{
		std::vector<char> ret;
		ret.resize(length);
		memcpy(ret.data(), buf + offset, length);

		unsigned char mask = (offset >> 1) & 0xFF;
		mask |= 0x23;
		for (int i = 0; i < length; ++i) {
			ret[i] ^= mask;
		}

		return ret;
	}

	void do_encrypt_block2(char* buf, int offset, int length)
	{
		unsigned char mask = (offset >> 1) & 0xFF;
		mask |= 0x23;
		for (int i = 0; i < length; ++i) {
			buf[offset + i] ^= mask;
		}
	}

	std::string append_path(std::string path1, std::string path2)
	{
		std::replace(path1.begin(), path1.end(), '/', '\\');
		std::replace(path2.begin(), path2.end(), '/', '\\');
		if (path1.back() != '\\')
		{
			path1 = path1 + '\\';
		}
		if (path2.front() == '\\')
		{
			path2 = path2.substr(1);
		}
		return path1 + path2;
	}

	template <typename T>
	void write_to_buffer(char*& buffer_ref, T val)
	{
		*(T*)buffer_ref = val;
		buffer_ref += sizeof(T);
	}

	void write_to_buffer(char*& buffer_ref, const void* data, int size)
	{
		memcpy(buffer_ref, data, size);
		buffer_ref += size;
	}
}

void GSPack::Serialize()
{
	std::vector<char> data;
	char* file;
	char* buffer;
	int ftable_length = 0;
	{
		int length = 2 + 4;
		
		for (auto&& file_entry : file_list)
		{
			int entry_size = 4 + 4 + 1 + file_entry.path.size();
			length += entry_size + file_entry.buffer.size();
			ftable_length += entry_size;
		}
		data.resize(length);
		buffer = data.data();
		file = buffer;
	}

	write_to_buffer<uint16_t>(buffer, file_list.size());
	write_to_buffer<uint32_t>(buffer, ftable_length);

	int next_block = ftable_length + 2 + 4;

	for (auto&& file_entry : file_list)
	{
		write_to_buffer<uint32_t>(buffer, next_block);
		write_to_buffer<uint32_t>(buffer, file_entry.buffer.size());
		write_to_buffer<uint8_t>(buffer, file_entry.path.size());
		write_to_buffer(buffer, file_entry.path.data(), file_entry.path.size());

		memcpy(&file[next_block], file_entry.buffer.data(), file_entry.buffer.size());
		do_encrypt_block2(file, next_block, file_entry.buffer.size());

		next_block += file_entry.buffer.size();
	}

	{
		char* buf = &file[6];
		do_encrypt_second(buf, ftable_length);
		do_encrypt_first(buf, ftable_length);
	}

	internalBuffer = std::move(data);
}

void GSPack::Deserialize()
{
	char* file = internalBuffer.data();
	char* buf = file;

	uint16_t a = *(uint16_t*)buf;
	buf += sizeof(a);
	uint32_t b = *(uint32_t*)buf;
	buf += sizeof(b);

	char* file_table_end = buf + b;
	int len = b;

	do_encrypt_first(buf, len);
	do_encrypt_second(buf, len);

	file_list.clear();

	while (buf < file_table_end)
	{
		FileInfoFile& info = *(FileInfoFile*) buf;

		file_list.emplace_back(
			std::string(info.path, info.path_length),
			std::move(do_encrypt_block(file, info.offset, info.length)));

		buf += info.Size();
	}
}
/*
int main()
{
	GSPack pack1;
	GSPack pack2;
	//pack1.OpenPackage("E:\\Games\\[game]GRIEFSYNDROME\\griefsyndrome\\multi03.dat");
	pack1.OpenPackage("D:\\Documents\\Programmes\\cpp\\GSOX\\Debug\\gs05.dat");
	//pack2.OpenPackage("E:\\Games\\[game]GRIEFSYNDROME\\griefsyndrome\\gougou03.dat");
	//pack1.Merge(pack2);
	pack1.SavePackage("E:\\Temp\\multigougou.dat");

#if 0
	for (auto&& i : pack)
	{
		std::vector<char> cc = i.GetContent();
		FILE* fw = fopen(append_path("E:\\Temp\\gs00", i.GetPath()).c_str(), "wb+");
		fwrite(cc.data(), 1, cc.size(), fw);
		fclose(fw);
	}
#endif
	return 0;
}*/
