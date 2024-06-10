// TermPod.cpp : Defines the entry point for the application.
//

#include "TermPod.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sstream>
#include <iomanip>

extern "C" {
#include <libtermpod.h>
}

std::string to_format(const pod_ssize_t number) {
	std::stringstream ss;
	ss << std::setw(2) << std::setfill('0') << number;
	return ss.str();
}

void printHelp() {
	fprintf(stderr, "Usage: %s [-h] [-l | -x | -c | -p] file [dir]\n\n", "TermPod");
	fprintf(stderr, "Pack/Unpack Terminal Reality POD and EPD archive files\n\n");
	fprintf(stderr, "positional arguments:\n");
	fprintf(stderr, "  file        input/output POD/EPD file\n");
	fprintf(stderr, "  dir         input/output directory\n\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "-h, --help                        show this help message and exit\n");
	fprintf(stderr, "-l, --list                        list files in POD/EPD archive\n");
	fprintf(stderr, "-x, --extract                     extract files from POD/EPD archive\n");
	fprintf(stderr, "-c, --create                      create POD/EPD archive\n");
	fprintf(stderr, "-p PATTERN, --pattern PATTERN\n");
	fprintf(stderr, "                                  list/extract only the files matching specified pattern\n");
}

// make a argument parser function
struct Arguments {
	bool help;
	bool list;
	bool extract;
	bool create;
	std::string pattern;
	std::string file;
	std::string dir;
};

Arguments parseArguments(int argc, char* argv[]) {
	Arguments args;
	args.help = false;
	args.list = false;
	args.extract = false;
	args.create = false;
	args.pattern = "";
	args.file = "";
	args.dir = "";

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			args.help = true;
		}
		else if (arg == "-l" || arg == "--list") {
			args.list = true;
		}
		else if (arg == "-x" || arg == "--extract") {
			args.extract = true;
		}
		else if (arg == "-c" || arg == "--create") {
			args.create = true;
		}
		else if (arg == "-p" || arg == "--pattern") {
			if (i + 1 < argc) {
				args.pattern = argv[i + 1];
				i++;
			}
		}
		else if (args.file.empty()) {
			args.file = arg;
		}
		else if (args.dir.empty()) {
			args.dir = arg;
		}
		else {
			fprintf(stderr, "TermPod: error: unrecognized arguments: %s\n", arg.c_str());
		}
	}

	return args;
}

int main(int argc, char* argv[])
{
	SetConsoleTitleA("TermPod (By Malte0621)");
	fprintf(stderr, "TermPod (By Malte0621)\n");
	
	// Args override (for debug testing):
	//argc = 6;
	//argv = new char* [6] { strdup("TermPod"), strdup("-c"), strdup("PATCH.POD"), strdup("-p"), strdup("POD6"), strdup("test") };
	//argc = 3;
	//argv = new char* [3] { strdup("TermPod"), strdup("-l"), strdup("PATCH.POD") };
	//argc = 4;
	//argv = new char* [4] { strdup("TermPod"), strdup("-x"), strdup("PATCH.POD"), strdup("test_ext") };
	
	if (argc < 2) {
		printHelp();
		fprintf(stderr, "\nTermPod: error: the following arguments are required: file\n");
		return 1;
	}

	Arguments args = parseArguments(argc, argv);

	if (args.help) {
		printHelp();
		return 0;
	}

	if (args.list) {
		fprintf(stderr, "List files in POD/EPD archive\n");
		pod_file_type_t pod = pod_file_create((pod_path_t)args.file.c_str(), UNKNOWN);
		if (!args.pattern.empty()) {
			pod_file_print(pod, (pod_char_t*)args.pattern.c_str());
		}
		else {
			pod_file_print(pod, NULL);
		}
	}
	else if (args.extract) {
		fprintf(stderr, "Extract files from POD/EPD archive\n");
		pod_file_type_t pod = pod_file_create((pod_path_t)args.file.c_str(), UNKNOWN);
		if (!args.pattern.empty()) {
			pod_file_extract(pod, (pod_char_t*)args.pattern.c_str(), (pod_path_t)args.dir.c_str());
		}
		else {
			pod_file_extract(pod, NULL, (pod_path_t)args.dir.c_str());
		}
	}
	else if (args.create) {
		fprintf(stderr, "Create POD/EPD archive\n");
		
		pod_ident_type_t type;

		if (args.pattern == "POD1")
			type = POD1;
		else if (args.pattern == "POD2")
			type = POD2;
		else if (args.pattern == "POD3")
			type = POD3;
		else if (args.pattern == "POD4")
			type = POD4;
		else if (args.pattern == "POD5")
			type = POD5;
		else if (args.pattern == "POD6")
			type = POD6;
		else if (args.pattern == "EPD")
			type = EPD;
		else {
			fprintf(stderr, "Invalid pattern (POD Version)\n");
			return 1;
		}

		std::transform(args.file.begin(), args.file.end(), args.file.begin(), ::toupper);

		std::string new_filename = args.file;
		
		pod_file_type_t pod = pod_file_create((pod_path_t)args.dir.c_str(), type);
		pod_ssize_t pods_count = 1;
		
		pod_ssize_t file_count = 0;
		pod_ssize_t final_file_count = 0;
		for (const auto& direntry : std::filesystem::recursive_directory_iterator(args.dir)) {
			if (!std::filesystem::is_regular_file(direntry.status())) {
				continue;
			}

			final_file_count++;
		}

		for (const auto& direntry : std::filesystem::recursive_directory_iterator(args.dir)) {
			std::string path = direntry.path().string();

			if (!std::filesystem::is_regular_file(direntry.status())) {
				continue;
			}

			std::string relative_path = path.substr(args.dir.length());
			std::replace(relative_path.begin(), relative_path.end(), '/', POD_PATH_SEPARATOR);

			if (relative_path[0] == POD_PATH_SEPARATOR) {
				relative_path = relative_path.substr(1);
			}
			
			std::ifstream file(path, std::ios::binary);
			std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});

			pod_byte_t* data = (pod_byte_t*)buffer.data();

			void* entry = nullptr;

			// get file timestamp
			unsigned long long timestamp = 0;
			struct stat result;
			if (stat(path.c_str(), &result) == 0) {
				timestamp = result.st_mtime;
			}

			pod_byte_t* data2 = nullptr;
			pod_number_t data2_size = 0;
			pod_char_t name[POD_DIR_ENTRY_POD1_FILENAME_SIZE] = { 0 };
			pod_char_t name2[POD_DIR_ENTRY_EPD_FILENAME_SIZE] = { 0 };

			unsigned char compressionLevel = 0;

			switch (type) {
				case POD1:
					data2_size = buffer.size();
					strncpy(name, relative_path.c_str(), POD_DIR_ENTRY_POD1_FILENAME_SIZE);
					entry = new pod_entry_pod1_t {
						.name = *name,
						.size = (pod_number_t)buffer.size(),
						.offset = (pod_number_t)(POD_HEADER_POD1_SIZE + file_count * POD_DIR_ENTRY_POD1_SIZE),
					};
					break;
				case POD2:
					data2_size = buffer.size();
					entry = new pod_entry_pod2_t{
						.path_offset = (pod_number_t)(pod.pod2->size + POD_DIR_ENTRY_POD2_SIZE + buffer.size()),
						.size = (pod_number_t)buffer.size(),
						.offset = (pod_number_t)(POD_HEADER_POD2_SIZE + file_count * POD_DIR_ENTRY_POD2_SIZE),
						.timestamp = (pod_signed_number_t)timestamp,
						.checksum = pod_crc((pod_byte_t*)data, (pod_size_t)buffer.size())
					};
					break;
				case POD3:
					data2_size = buffer.size();
					entry = new pod_entry_pod3_t{
						.path_offset = (pod_number_t)(pod.pod3->size + POD_DIR_ENTRY_POD3_SIZE + buffer.size()),
						.size = (pod_number_t)buffer.size(),
						.offset = (pod_number_t)(POD_HEADER_POD3_SIZE + file_count * POD_DIR_ENTRY_POD3_SIZE),
						.timestamp = (pod_number_t)timestamp,
						.checksum = pod_crc((pod_byte_t*)data, (pod_size_t)buffer.size())
					};
					break;
				case POD4:
					if (buffer.size() >= 1024) {
						if (buffer.size() >= 1024 * 4) compressionLevel = 8;
						else if (buffer.size() >= 1024 * 2) compressionLevel = 4;
						else if (buffer.size() >= 1024 * 1) compressionLevel = 2;
						else compressionLevel = 1;
						data2 = pod_compress((pod_byte_t*)data, compressionLevel, (pod_size_t)buffer.size(), (pod_string_t)relative_path.c_str(), &data2_size);
					}
					else {
						data2_size = buffer.size();
					}

					entry = new pod_entry_pod4_t{
						.path_offset = (pod_number_t)(pod.pod4->size + POD_DIR_ENTRY_POD4_SIZE + data2_size),
						.size = (pod_number_t)data2_size,
						.offset = (pod_number_t)(pod.pod4->size + file_count * POD_DIR_ENTRY_POD4_SIZE),
						.uncompressed = (pod_number_t)buffer.size(),
						.compression_level = (pod_number_t)(data2 == nullptr ? 0 : compressionLevel),
						.timestamp = (pod_number_t)timestamp,
						.checksum = pod_crc((pod_byte_t*)data2, (pod_size_t)buffer.size())
					};
					break;
				case POD5:
					if (buffer.size() >= 1024) {
						if (buffer.size() >= 1024 * 4) compressionLevel = 8;
						else if (buffer.size() >= 1024 * 2) compressionLevel = 4;
						else if (buffer.size() >= 1024 * 1) compressionLevel = 2;
						else compressionLevel = 1;
						data2 = pod_compress((pod_byte_t*)data, compressionLevel, (pod_size_t)buffer.size(), (pod_string_t)relative_path.c_str(), &data2_size);
					}
					else {
						data2_size = buffer.size();
					}

					entry = new pod_entry_pod5_t{
						.path_offset = (pod_number_t)(pod.pod5->size + POD_DIR_ENTRY_POD5_SIZE + data2_size),
						.size = (pod_number_t)data2_size,
						.offset = (pod_number_t)(pod.pod5->size + file_count * POD_DIR_ENTRY_POD5_SIZE),
						.uncompressed = (pod_number_t)buffer.size(),
						.compression_level = (pod_number_t)(data2 == nullptr ? 0 : compressionLevel),
						.timestamp = (pod_number_t)timestamp,
						.checksum = pod_crc((pod_byte_t*)data2, (pod_size_t)buffer.size())
					};
					break;
				case POD6:
					if (buffer.size() >= 1024) {
						if (buffer.size() >= 1024 * 4) compressionLevel = 8;
						else if (buffer.size() >= 1024 * 2) compressionLevel = 4;
						else if (buffer.size() >= 1024 * 1) compressionLevel = 2;
						else compressionLevel = 1;
						data2 = pod_compress((pod_byte_t*)data, compressionLevel, (pod_size_t)buffer.size(), (pod_string_t)relative_path.c_str(), &data2_size);
					}
					else {
						data2_size = buffer.size();
					}

					entry = new pod_entry_pod6_t{
						.path_offset = (pod_number_t)(pod.pod6->size + POD_DIR_ENTRY_POD6_SIZE + data2_size),
						.size = (pod_number_t)data2_size,
						.offset = (pod_number_t)(pod.pod6->size + file_count * POD_DIR_ENTRY_POD6_SIZE),
						.uncompressed = (pod_number_t)buffer.size(),
						.compression_level = (pod_number_t)(data2 == nullptr ? 0 : compressionLevel),
						.zero = 0,
					};
					break;
				case EPD:
					data2_size = buffer.size();
					strncpy(name2, relative_path.c_str(), POD_DIR_ENTRY_EPD_FILENAME_SIZE);
					entry = new pod_entry_epd_t{
						.name = *name2,
						.size = (pod_number_t)buffer.size(),
						.offset = (pod_number_t)(POD_HEADER_EPD_SIZE + file_count * POD_DIR_ENTRY_EPD_SIZE),
						.timestamp = (pod_number_t)timestamp,
						.checksum = pod_crc((pod_byte_t*)data2, (pod_size_t)buffer.size())
					};
					break;
				default:
					fprintf(stderr, "Unknown pod type\n");
					break;
			}

			if (entry == nullptr) {
				fprintf(stderr, "Entry is null\n");
				return 1;
			}
			
			switch (type) {
			case POD5:
				if (pod.pod5->size + data2_size >= 0x7FFFFFFF) {
					fprintf(stderr, "Archive size exceeds 2GB limit! Saving & splitting archive.\n");
					file_count = 0;
					pods_count++;
					new_filename = args.file.substr(0, args.file.length() - 4) + to_format(pods_count) + ".POD";
					if (new_filename.length() >= POD_HEADER_NEXT_ARCHIVE_SIZE) {
						fprintf(stderr, "Filename too long! Aborting.\n");
						return 1;
					}
					if (pod.pod5->size >= 0x7FFFFFFF) {
						fprintf(stderr, "File '%s' size exceeds 2GB limit! Aborting.\n", name);
						return 1;
					}

					strcpy(pod.pod5->header->next_archive, new_filename.c_str());
					pod_file_write(pod, (pod_path_t)args.file.c_str());
					pod_file_delete(pod);
					pod = pod_file_create((pod_path_t)args.dir.c_str(), type);
				}
				break;
			case POD6:
				if (pod.pod6->size + data2_size >= 0x7FFFFFFF) {
					fprintf(stderr, "Archive size exceeds 2GB limit! Saving & splitting archive.\n");
					file_count = 0;
					pods_count++;
					new_filename = args.file.substr(0, args.file.length() - 4) + to_format(pods_count) + ".POD";
					if (new_filename.length() >= POD_HEADER_NEXT_ARCHIVE_SIZE) {
						fprintf(stderr, "Filename too long! Aborting.\n");
						return 1;
					}
					if (pod.pod5->size >= 0x7FFFFFFF) {
						fprintf(stderr, "File '%s' size exceeds 2GB limit! Aborting.\n", name);
						return 1;
					}
					strcpy(pod.pod6->header->next_archive, new_filename.c_str());
					pod_file_write(pod, (pod_path_t)args.file.c_str());
					pod_file_delete(pod);
					pod = pod_file_create((pod_path_t)args.dir.c_str(), type);
				}
				break;
			}

			if (data2 == nullptr) {
				pod_file_entry_data_add(pod, entry, (pod_string_t)relative_path.c_str(), (pod_byte_t*)data);
			}
			else {
				pod_file_entry_data_add(pod, entry, (pod_string_t)relative_path.c_str(), (pod_byte_t*)data2);
			}
			
			delete data2;
			delete entry;

			fprintf(stderr, "%s -> %s\n", path.c_str(), new_filename.c_str());
			file_count++;
		}
		
		pod_file_write(pod, (pod_path_t)new_filename.c_str());
	}
	else {
		printHelp();
		fprintf(stderr, "\nInvalid arguments\n");
		return 1;
	}

	return 0;
}
