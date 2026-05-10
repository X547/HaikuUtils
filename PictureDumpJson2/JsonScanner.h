#pragma once

#include <math.h>

#include <string>
#include <stdexcept>
#include <istream>

#include <SupportDefs.h>

#include <rapidjson/reader.h>
#include <rapidjson/istreamwrapper.h>


class JsonParseError: public std::runtime_error {
public:
	JsonParseError(): std::runtime_error("JSON parse error") {}
};


enum class JsonTokenKind {
	Eos,
	Null,
	Bool,
	Int,
	UInt,
	Int64,
	UInt64,
	Double,
	RawNumber,
	String,
	StartObject,
	Key,
	EndObject,
	StartArray,
	EndArray,
};


struct JsonToken {
	JsonTokenKind kind = JsonTokenKind::Eos;
	std::string strVal;
	bool boolVal;
	int64_t int64Val;
	uint64_t uint64Val;
	double doubleVal;

	bool IsInt32()
	{
		switch (kind) {
			case JsonTokenKind::Int:
			case JsonTokenKind::Int64:
				return int64Val >= INT32_MIN && int64Val <= INT32_MAX;
			case JsonTokenKind::UInt:
			case JsonTokenKind::UInt64:
				return uint64Val <= INT32_MAX;
			default:
				return false;
		}
	}

	bool IsReal()
	{
		switch (kind) {
			case JsonTokenKind::Int:
			case JsonTokenKind::Int64:
			case JsonTokenKind::UInt:
			case JsonTokenKind::UInt64:
			case JsonTokenKind::Double:
				return true;
			default:
				return false;
		}
	}

	int32 Int32Val()
	{
		switch (kind) {
			case JsonTokenKind::Int:
			case JsonTokenKind::Int64:
				return int64Val;
			case JsonTokenKind::UInt:
			case JsonTokenKind::UInt64:
				return uint64Val;
			case JsonTokenKind::Double:
				return doubleVal;
			default:
				return 0;
		}
	}

	double RealVal()
	{
		switch (kind) {
			case JsonTokenKind::Int:
			case JsonTokenKind::Int64:
				return int64Val;
			case JsonTokenKind::UInt:
			case JsonTokenKind::UInt64:
				return uint64Val;
			case JsonTokenKind::Double:
				return doubleVal;
			default:
				return NAN;
		}
	}
};


class JsonScanner {
private:
	rapidjson::IStreamWrapper fStream;
	rapidjson::Reader fRd;
	JsonToken fToken;


public:
	JsonScanner(std::istream &is);

	void ReadToken();
	inline JsonToken &Token();
	[[noreturn]] void RaiseError();
	inline void Assume(bool cond);
	inline void AssumeToken(JsonTokenKind tokenKind);

	bool ReadBool();
	uint8 ReadUint8();
	int32 ReadInt32();
	double ReadReal();
};


JsonToken &JsonScanner::Token()
{
	return fToken;
}

void JsonScanner::Assume(bool cond)
{
	if (!cond) {
		RaiseError();
	}
}

void JsonScanner::AssumeToken(JsonTokenKind tokenKind)
{
	Assume(Token().kind == tokenKind);
}
