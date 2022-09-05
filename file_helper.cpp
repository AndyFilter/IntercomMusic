#include "file_helper.h"
#include <fstream>

bool FileHelper::SaveRecording(Recording& rec, char save_path[MAX_PATH])
{
	std::ofstream saveFile(save_path, std::ios::binary);

	if (!saveFile.good())
		return false;

	//for (size_t i = 0; i < rec.data.size(); i++)
	//{
	//	auto Type = rec.data[i].type;
	//	auto Value = rec.data[i].value;
	//	BYTE data[sizeof(RecordingEvent)];
	//	saveFile.write((char*)data, 8);
	//}
	size_t size = sizeof(RecordingEvent) * rec.data.size();
	BYTE* data = new BYTE[size];

	memcpy(data, rec.data.data(), size);

	printf("size is: %lld\n", size);

	saveFile.write((char*)data, size);

	saveFile.close();
	delete[] data;

	return true;
}

bool FileHelper::OpenRecording(Recording(&rec), char file_path[MAX_PATH])
{
	std::ifstream saveFile(file_path, std::ios::binary | std::ios::ate);

	if (!saveFile.good())
		return false;

	int size = saveFile.tellg();
	saveFile.seekg(0, std::ios::beg);

	BYTE* buffer = new BYTE[size];
	saveFile.read((char*)buffer, size);

	rec.data.resize(size / sizeof(RecordingEvent));
	memcpy(rec.data.data(), buffer, size);

	saveFile.close();
	delete[] buffer;

	return true;
}

