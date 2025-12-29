#include "PictureReaderBinary.h"
#include "PictureReaderJson.h"
#include "PictureWriterBinary.h"
#include "PictureWriterJson.h"
#include "PictureWriterYaml.h"

#include <optional>

#include <File.h>
#include <BufferIO.h>

#include <iostream>
#include <fstream>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>


enum class FileFormat {
	Binary,
	Json,
	Yaml,
};


struct Options {
	std::optional<std::string> outputPath;
	std::optional<std::string> inputPath;
	std::optional<FileFormat> outputFormat;
	std::optional<FileFormat> inputFormat;
};


static FileFormat FileFormatFromString(std::string_view str)
{
	if (str == "binary") {
		return FileFormat::Binary;
	}
	if (str == "json") {
		return FileFormat::Json;
	}
	if (str == "yaml") {
		return FileFormat::Yaml;
	}
	throw std::runtime_error("unknown argument");
}

static void ParseOptions(Options &opts, int argc, char **argv)
{
	int nextArgIdx = 1;
	std::string_view arg;
	auto NextArg = [argc, argv, &nextArgIdx, &arg]() {
		if (nextArgIdx >= argc) {
			throw std::runtime_error("argument missing");
		}
		arg = argv[nextArgIdx++];
	};

	while (nextArgIdx < argc) {
		NextArg();
		if (arg == "--output") {
			NextArg();
			opts.outputPath = arg;
		} else if (arg == "--input") {
			NextArg();
			opts.inputPath = arg;
		} else if (arg == "--output-format") {
			NextArg();
			opts.outputFormat = FileFormatFromString(arg);
		} else if (arg == "--input-format") {
			NextArg();
			opts.inputFormat = FileFormatFromString(arg);
		} else {
			throw std::runtime_error("unknown argument");
		}
	}

	if (!opts.outputPath.has_value()) {
		throw std::runtime_error("`--output` option missing");
	}
	if (!opts.inputPath.has_value()) {
		throw std::runtime_error("`--input` option missing");
	}
	if (!opts.outputFormat.has_value()) {
		throw std::runtime_error("`--output-format` option missing");
	}
	if (!opts.inputFormat.has_value()) {
		throw std::runtime_error("`--input-format` option missing");
	}
}


using JsonWriter = rapidjson::Writer<rapidjson::OStreamWrapper>;


static void Accept(Options &opts, PictureVisitor &vis)
{
	switch (opts.inputFormat.value()) {
		case FileFormat::Binary: {
			BFile file(opts.inputPath.value().c_str(), B_READ_ONLY);
			BBufferIO buf(&file, 65536, false);
			PictureReaderBinary pict(buf);
			pict.Accept(vis);
			break;
		}
		case FileFormat::Json: {
			std::ifstream is(opts.inputPath.value(), std::ios::binary);
			if (!is) {
				throw std::runtime_error("can't open input file");
			}
			PictureReaderJson pict(is);
			pict.Accept(vis);
			break;
		}
		case FileFormat::Yaml: {
			throw std::runtime_error("YAML input format unimplemented");
			break;
		}
	}
}


int main(int argc, char **argv)
{
	try {
		Options opts;
		ParseOptions(opts, argc, argv);

		std::unique_ptr<PictureVisitor> vis;

		switch (opts.outputFormat.value()) {
			case FileFormat::Binary: {
				BFile file(opts.outputPath.value().c_str(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
				//BBufferIO buf(&file, 65536, false); // broken
				PictureWriterBinary vis(file);

				Accept(opts, vis);
				break;
			}
			case FileFormat::Json: {
				std::ofstream os(opts.outputPath.value(), std::ios::binary | std::ios::trunc);
				if (!os) {
					throw std::runtime_error("can't open output file");
				}
				rapidjson::OStreamWrapper osWrap(os);
				JsonWriter wr(osWrap);
				PictureWriterJson vis(wr);

				Accept(opts, vis);
				break;
			}
			case FileFormat::Yaml: {
				std::ofstream os(opts.outputPath.value(), std::ios::binary | std::ios::trunc);
				if (!os) {
					throw std::runtime_error("can't open output file");
				}
				YAML::Emitter wr(os);
				PictureWriterYaml vis(wr);

				Accept(opts, vis);
				os << std::endl;
				break;
			}
		}
	} catch (const std::runtime_error &e) {
		std::cerr << "[!] " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
