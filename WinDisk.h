#ifndef _WIN_DISK_H
#define _WIN_DISK_H

#include <string>

// only support winnt 2000 or later

namespace WinDisk
{
	std::string GetSerialNum();
}

#endif