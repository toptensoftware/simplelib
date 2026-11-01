#include "../UnitTesting.h"
#include "../Json.h"
using namespace SimpleLib;

// Json.h is also included by TestJson2.cpp - linking both verifies the
// header can be used from multiple translation units.

Fact("Json Stringify compact")
{
	auto obj = new JSONObject();
	JSONValue root(obj);
	obj->Set("name", "x");
	obj->Set("count", 3);
	obj->Set("flag", true);

	String text = JSON::Stringify(root, 0);
	Assert(text.IndexOf("\"name\"") >= 0);
	Assert(text.IndexOf("\"x\"") >= 0);
	Assert(text.IndexOf("3") >= 0);
	Assert(text.IndexOf("true") >= 0);
}

Fact("Json value copy shares array")
{
	auto arr = new JSONArray();
	JSONValue a(arr);
	arr->Add(JSONValue(1));
	JSONValue b(a);
	Assert(b.kind == JsonKind::Array);
	Assert(&b.AsArray() == arr);
	Assert(b.AsArray().GetCount() == 1);
}
