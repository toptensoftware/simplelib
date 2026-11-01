#include "../UnitTesting.h"
#include "../Json.h"
using namespace SimpleLib;

// Second translation unit including Json.h (see TestJson.cpp)

Fact("Json string value")
{
	JSONValue v("hello");
	Assert(v.kind == JsonKind::StringCore);
	Assert(strcmp(v.AsString(), "hello") == 0);

	JSONValue copy(v);
	Assert(copy.AsString() != v.AsString());	// strings are deep copied
	Assert(strcmp(copy.AsString(), "hello") == 0);
}
