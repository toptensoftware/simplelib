#include "../UnitTesting.h"
#include "../Threading.h"
#include "../Core.h"

int main(int argc, const char** argv)
{
	// Exit code is non-zero if any test failed
	return SimpleLib::TestRunner::Run() == 0 ? 0 : 1;
}