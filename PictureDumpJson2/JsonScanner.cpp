#include "JsonScanner.h"

#include <rapidjson/reader.h>
#include <rapidjson/istreamwrapper.h>


namespace {


class JsonTokenHandler {
private:
	JsonToken &fToken;

	JsonTokenHandler(const JsonTokenHandler& noCopyConstruction);
	JsonTokenHandler& operator=(const JsonTokenHandler& noAssignment);

public:
	JsonTokenHandler(JsonToken &token): fToken(token) {}

	bool Null() {
		fToken.kind = JsonTokenKind::Null;
		return true;
	}

	bool Bool(bool b) {
		fToken.kind = JsonTokenKind::Bool;
		fToken.boolVal = b;
		return true;
	}

	bool Int(int i) {
		fToken.kind = JsonTokenKind::Int;
		fToken.int64Val = i;
		return true;
	}

	bool Uint(unsigned u) {
		fToken.kind = JsonTokenKind::UInt;
		fToken.uint64Val = u;
		return true;
	}

	bool Int64(int64_t i) {
		fToken.kind = JsonTokenKind::Int64;
		fToken.int64Val = i;
		return true;
	}

	bool Uint64(uint64_t u) {
		fToken.kind = JsonTokenKind::UInt64;
		fToken.uint64Val = u;
		return true;
	}

	bool Double(double d) {
		fToken.kind = JsonTokenKind::Double;
		fToken.doubleVal = d;
		return true;
	}

	bool RawNumber(const char* str, rapidjson::SizeType length, bool) {
		fToken.kind = JsonTokenKind::RawNumber;
		fToken.strVal = std::string_view(str, length);
		return true;
	}

	bool String(const char* str, rapidjson::SizeType length, bool) {
		fToken.kind = JsonTokenKind::String;
		fToken.strVal = std::string_view(str, length);
		return true;
	}

	bool StartObject() {
		fToken.kind = JsonTokenKind::StartObject;
		return true;
	}

	bool Key(const char* str, rapidjson::SizeType length, bool) {
		fToken.kind = JsonTokenKind::Key;
		fToken.strVal = std::string_view(str, length);
		return true;
	}

	bool EndObject(rapidjson::SizeType memberCount) {
		fToken.kind = JsonTokenKind::EndObject;
		fToken.uint64Val = memberCount;
		return true;
	}

	bool StartArray() {
		fToken.kind = JsonTokenKind::StartArray;
		return true;
	}

	bool EndArray(rapidjson::SizeType elementCount) {
		fToken.kind = JsonTokenKind::EndArray;
		fToken.uint64Val = elementCount;
		return true;
	}
};


}


JsonScanner::JsonScanner(std::istream &is):
	fStream(is)
{
	fRd.IterativeParseInit();
}

void JsonScanner::RaiseError()
{
	throw JsonParseError();
}

void JsonScanner::ReadToken()
{
	JsonTokenHandler handler(fToken);
	if (fRd.IterativeParseComplete()) {
		fToken.kind = JsonTokenKind::Eos;
		return;
	}
	if (!fRd.IterativeParseNext<rapidjson::kParseDefaultFlags>(fStream, handler)) {
		RaiseError();
	}
}

bool JsonScanner::ReadBool()
{
	AssumeToken(JsonTokenKind::Bool);
	bool val = Token().boolVal;
	ReadToken();
	return val;
}

uint8 JsonScanner::ReadUint8()
{
	Assume(Token().IsInt32());
	int32 val = Token().Int32Val();
	Assume(val >= 0 && val <= 0xff);
	ReadToken();
	return (uint8)val;
}

int32 JsonScanner::ReadInt32()
{
	Assume(Token().IsInt32());
	int32 val = Token().Int32Val();
	ReadToken();
	return val;
}

double JsonScanner::ReadReal()
{
	Assume(Token().IsReal());
	double val = Token().RealVal();
	ReadToken();
	return val;
}
