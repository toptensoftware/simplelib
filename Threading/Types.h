#pragma once

namespace SimpleLib
{

// Special timeout milliseconds for "forever"
static const uint32_t kWaitForever = 0xFFFFFFFF;


// Typical CPU cache line size - hot atomics that are independently
// written by different threads are padded apart by this much to avoid
// false sharing.
static const int kCacheLineSize = 64;


// Thread priority
enum class ThreadPriority
{
	BelowNormal,
	Normal,
	AboveNormal,
	RealTime,
};

namespace Platform
{
	typedef void (*ThreadProc)(void* param);
}


}
