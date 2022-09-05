#pragma once

#include "structs.h"

namespace FileHelper
{
	bool SaveRecording(Recording& rec, char save_path[MAX_PATH]);
	bool OpenRecording(Recording(&rec), char file_path[MAX_PATH]);
}