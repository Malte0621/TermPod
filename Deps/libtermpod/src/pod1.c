#include "pod_common.h"
#include "pod1.h"

bool pod_is_pod1(char* ident)
{
  return (POD1 == pod_type(ident) >= 0);
}

pod_checksum_t pod_crc_pod1(pod_file_pod1_t* file)
{
	if(file == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_pod1() file == NULL!");
		return 0;
	}

	return pod_crc(file->data + POD_IDENT_SIZE + POD_HEADER_CHECKSUM_SIZE, file->size - POD_IDENT_SIZE - POD_HEADER_CHECKSUM_SIZE);
}

pod_checksum_t pod_crc_pod1_entry(pod_file_pod1_t* file, pod_number_t entry_index)
{
	if(file == NULL || file->entry_data == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_pod1() file == NULL!");
		return 0;
	}

	return pod_crc(file->data + file->entries[entry_index].offset, file->entries[entry_index].size);
}
 

pod_checksum_t   pod_file_pod1_chksum(pod_file_pod1_t* podfile)
{
	return pod_crc_pod1(podfile);
}

pod_file_pod1_t* pod_file_pod1_create(pod_string_t filename)
{
	pod_file_pod1_t* pod_file = calloc(1, sizeof(pod_file_pod1_t));

	if (dir_exists(filename))
	{
		pod_file->data = calloc(1, POD_HEADER_POD1_SIZE);
		pod_file->header = (pod_header_pod1_t*)pod_file->data;
		memcpy(pod_file->header, "POD1", POD_IDENT_SIZE);
		pod_file->header->file_count = 0;
		memset(pod_file->header->comment, 0, POD_COMMENT_SIZE);
		pod_file->header->comment[0] = '\0';
		pod_file->header->comment[POD_COMMENT_SIZE - 1] = '\0';

		pod_file->entries = calloc(1, sizeof(pod_entry_pod1_t));
		pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_POD1_SIZE + pod_file->header->file_count * POD_DIR_ENTRY_POD1_SIZE);
		pod_file->entry_data_size = 0;
		pod_file->filename = calloc(1, strlen(filename));
		pod_file->filename = strcpy(pod_file->filename, filename);
		pod_file->checksum = 0;
		pod_file->data = calloc(1, sizeof(pod_byte_t));
		pod_file->size = POD_HEADER_POD1_SIZE;
		return pod_file;
	}

	struct stat sb;
	if(stat(filename, &sb) != 0 || sb.st_size == 0)
	{
		perror("stat");
		return NULL;
	}

	pod_file->filename = calloc(1, strlen(filename));
	pod_file->filename = strcpy(pod_file->filename, filename);
	pod_file->size = sb.st_size;

	FILE* file = fopen(filename, "rb");

	if(!file)
	{
		fprintf(stderr, "ERROR: Could not open POD file: %s\n", filename);
		return NULL;
	}

	pod_file->data = calloc(1, pod_file->size);

	if(!pod_file->data)
	{
		fprintf(stderr, "ERROR: Could not allocate memory of size %zu for file %s!\n", pod_file->size, filename);
		fclose(file);
		pod_file_pod1_delete(pod_file);
		return NULL;
	}

	if(fread(pod_file->data, POD_BYTE_SIZE, pod_file->size, file) != pod_file->size * POD_BYTE_SIZE)
	{
		fprintf(stderr, "ERROR: Could not read file %s!\n", filename);
		fclose(file);
		pod_file_pod1_delete(pod_file);
		return NULL;
	}

	fclose(file);
	pod_file->checksum = pod_crc(pod_file->data, pod_file->size);

	size_t data_pos = 0;
	pod_file->header = (pod_header_pod1_t*)pod_file->data;
	data_pos += POD_HEADER_POD1_SIZE;
	pod_file->entries = (pod_entry_pod1_t*)(pod_file->data + data_pos);
	data_pos += pod_file->header->file_count * POD_DIR_ENTRY_POD1_SIZE;

	pod_number_t min_entry_index = 0;
	pod_number_t max_entry_index = 0;

	for(pod_number_t i = 0; i < pod_file->header->file_count; i++)
	{
		if(pod_file->entries[i].offset < pod_file->entries[min_entry_index].offset)
		{
			min_entry_index = i;
		}
		if(pod_file->entries[i].offset > pod_file->entries[max_entry_index].offset)
		{
			max_entry_index = i;
		}
	}

	size_t max_entry_len = pod_file->entries[max_entry_index].size;
	pod_file->entry_data_size = (pod_file->data + pod_file->entries[max_entry_index].offset + max_entry_len) - 
				 (pod_file->data + pod_file->entries[min_entry_index].offset);

	pod_file->entry_data = pod_file->data + pod_file->entries[min_entry_index].offset;

	data_pos += pod_file->entry_data_size;

	return pod_file;
}

bool pod_file_pod1_add_entry(pod_file_pod1_t* pod_file, pod_entry_pod1_t* entry, pod_string_t filename, pod_byte_t* data)
{
	if(pod_file == NULL || entry == NULL || data == NULL)
	{
		fprintf(stderr, "ERROR: pod_file, entry or data equals NULL!\n");
		return false;
	}

	size_t new_entry_size = POD_DIR_ENTRY_POD1_SIZE;
	size_t new_entry_data_size = entry->size;
	size_t new_path_data_size = strlen(filename) + 1;
	size_t new_total_size = pod_file->size + new_entry_data_size;

	void* new_data = realloc(pod_file->data, new_total_size);
	if (!new_data) {
		return false;
	}
	pod_file->data = new_data;

	pod_file->header = (pod_header_pod1_t*)pod_file->data;
	pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_POD1_SIZE);

	pod_file->header->file_count++;

	memcpy(pod_file->data + pod_file->size, data, entry->size);
	entry->offset = pod_file->size;
	pod_file->size += entry->size;

	// re-allocate entries
	void* new_entries = realloc(pod_file->entries, pod_file->header->file_count * new_entry_size);
	if (!new_entries) {
		return false;
	}
	pod_file->entries = new_entries;
	// copy new entry data to end of entries
	memcpy(pod_file->entries + (pod_file->header->file_count - 1), entry, new_entry_size);

	//pod_file->checksum = pod_crc_pod1(pod_file);

	return true;
}

bool pod_file_pod1_del_entry(pod_file_pod1_t* pod_file, pod_number_t entry_index)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_del_entry(pod_file == NULL)\n");
		return false;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_del_entry(entry_index >= pod_file->header->file_count)\n");
		return false;
	}

	pod_entry_pod1_t* entry = &pod_file->entries[entry_index];
	pod_file->entry_data_size -= entry->size;
	pod_file->entry_data = realloc(pod_file->entry_data, pod_file->entry_data_size);

	if (entry_index < pod_file->header->file_count - 1)
	{
		memmove(pod_file->entries + entry_index, pod_file->entries + entry_index + 1, (pod_file->header->file_count - entry_index - 1) * POD_DIR_ENTRY_POD1_SIZE);
	}

	pod_file->header->file_count--;
	pod_file->entries = realloc(pod_file->entries, pod_file->header->file_count * POD_DIR_ENTRY_POD1_SIZE);

	//pod_file->checksum = pod_crc_pod1(pod_file);

	return true;
}

pod_entry_pod1_t* pod_file_pod1_get_entry(pod_file_pod1_t* pod_file, pod_number_t entry_index)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_get_entry(pod_file == NULL)\n");
		return NULL;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_get_entry(entry_index >= pod_file->header->file_count)\n");
		return NULL;
	}

	return &pod_file->entries[entry_index];
}

bool pod_file_pod1_extract_entry(pod_file_pod1_t* pod_file, pod_number_t entry_index, pod_string_t dst)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_extract_entry(pod_file == NULL)\n");
		return false;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_extract_entry(entry_index >= pod_file->header->file_count)\n");
		return false;
	}

	FILE* file = fopen(dst, "wb");

	if (file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_extract_entry(fopen(%s, \"wb\") failed: %s\n", dst, strerror(errno));
		return false;
	}

	pod_entry_pod1_t* entry = &pod_file->entries[entry_index];

	if (fwrite(pod_file->data + entry->offset, entry->size, 1, file) != 1)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_extract_entry(fwrite failed!\n");
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

bool pod_file_pod1_write(pod_file_pod1_t* pod_file, pod_string_t filename)
{
	if(pod_file == NULL || filename == NULL)
	{
		fprintf(stderr, "ERROR: pod_file or filename equals NULL!\n");
		return false;
	}
	
	size_t new_total_size = pod_file->size + pod_file->header->file_count * POD_DIR_ENTRY_POD1_SIZE;
	void* new_data = realloc(pod_file->data, new_total_size);
	if (!new_data) {
		fprintf(stderr, "ERROR: extending data entries!\n");
		return false;
	}
	pod_file->data = new_data;

	pod_file->header = (pod_header_pod1_t*)pod_file->data;
	pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_POD1_SIZE);

	// apply all entries to the end of the pod_file->data.

	memcpy(pod_file->data + pod_file->size, pod_file->entries, pod_file->header->file_count * POD_DIR_ENTRY_POD1_SIZE);
	pod_file->size += pod_file->header->file_count * POD_DIR_ENTRY_POD1_SIZE;

	pod_file->entries = (pod_entry_pod1_t*)(pod_file->data + POD_DIR_ENTRY_POD1_SIZE);

	FILE* file = fopen(filename, "wb");

	if (fwrite(pod_file->data, pod_file->size, 1, file) != 1)
	{
		fprintf(stderr, "ERROR: writing POD1 data!\n");
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

/* Extract POD1 file pod_file to directory dst                       */
/* @returns true on success otherwise false and leaves errno         */
bool pod_file_pod1_extract(pod_file_pod1_t* pod_file, pod_string_t pattern, pod_string_t dst)
{
	pod_bool_t absolute = false;
	if(pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_extract(pod_file == NULL)\n");
		return false;
	}

	if(dst == NULL) { dst = "./"; }

	pod_char_t cwd[POD_SYSTEM_PATH_SIZE];
	getcwd(cwd, sizeof(cwd));
	printf("cwd: %s\n",cwd);
	pod_path_t root = pod_path_system_root();

	printf("root: %s\n",root);
	pod_path_t path = NULL;

	if(absolute)
	{
		path = pod_path_append(root, dst);
		if(!path)
		{
			fprintf(stderr, "ERROR: pod_path_append(%s,%s)\n", root, dst);
			free(root);
			return false;
		}
	}
	else
	{
		path = pod_path_append(cwd, dst);
		if(!path)
		{
			fprintf(stderr, "ERROR: pod_path_append(%s,%s)\n", cwd, dst);
			free(root);
			return false;
		}
	}

	printf("path: %s\n", path);
	/* create and change to destination directory */
	if(mkdir_p(path, ACCESSPERMS) != 0)
	{
		fprintf(stderr, "mkdir_p(\"%s\", \'%c\') = %s\n", dst, '/', strerror(errno));
		return false;
	}

	if(chdir(path) != 0)
	{
		fprintf(stderr, "chdir(%s) = %s\n", path, strerror(errno));
		return false;
	}

	/* extract entries */
	for(int i = 0; i < pod_file->header->file_count; i++)
	{
		if (pattern && strstr(pod_file->entries[i].name, pattern) == NULL)
			continue;

		char* basename = strdup(pod_file->entries[i].name);
		if (!basename)
		{
			fprintf(stderr, "ERROR: strdup(filename) failed!\n");
			return false;
		}
		char* p = basename;
		while (*p)
		{
			if (*p == '/' || *p == '\\')
			{
				basename = p + 1;
			}
			p++;
		}

		/* open and create directories including parents */
		if(strlen(basename) > 0 && mkdir_p(pod_path_append(path, basename), ACCESSPERMS) != 0)
		{
			fprintf(stderr, "ERROR: mkdir_p(%s) failed: %s\n", basename, strerror(errno));
			return false;
		}
		FILE* file = fopen(pod_file->entries[i].name, "wb");
		if(file == NULL)
		{
			fprintf(stderr, "ERROR: pod_fopen_mkdir(%s) failed: %s\n", pod_file->entries[i].name, strerror(errno));
			return false;
		}
		if(fwrite(pod_file->data + pod_file->entries[i].offset, pod_file->entries[i].size, 1, file) != 1)
		{
			fprintf(stderr, "ERROR: fwrite failed!\n");
			fclose(file);
			return false;
		}

		/* Debug/Info print thingy */
		fprintf(stderr, "%s -> %s\n", pod_file->entries[i].name, pod_path_append(path, pod_file->entries[i].name));

		/* clean up and pop */
		fclose(file);
	}
	chdir(cwd);
	return true;
}

pod_file_pod1_t* pod_file_pod1_delete(pod_file_pod1_t* pod_file)
{
	if(pod_file)
	{
		if(pod_file->data)
		{
			free(pod_file->data);
			pod_file->data;
		}
		if(pod_file->filename)
		{
			free(pod_file->filename);
			pod_file->filename = NULL;
		}
		free(pod_file);
		pod_file = NULL;
	}
	else
		fprintf(stderr, "ERROR: could not free pod_file == NULL!\n");

	return pod_file;
}

bool pod_file_pod1_print(pod_file_pod1_t* pod_file, pod_char_t* pattern)
{
	if(pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod1_print() pod_file == NULL\n");
		return false;
	}

	/* print entries */
	printf("\nEntries:\n");
	for(pod_number_t i = 0; i < pod_file->header->file_count; i++)
	{
		pod_entry_pod1_t* entry = &pod_file->entries[i];
		if (pattern && strstr(entry->name, pattern) == NULL)
			continue;
		printf("%10u 0x%.8X/%.10u 0x%.8X/%10u %s\n",
		       	i,
			entry->offset,entry->offset,
			entry->size,entry->size,
			entry->name);
	}

	/* print file summary */
	printf("\nSummary:\nfile checksum      : 0x%.8X\nsize               : %zu\nfilename           : %s\nformat             : %s\ncomment            : %s\nfile entries       : 0x%.8X/%.10u\n",
		pod_file->checksum,
		pod_file->size,
		pod_file->filename,
		pod_type_desc_str(POD1),
		pod_file->header->comment,
		pod_file->header->file_count,pod_file->header->file_count);

	
	return true;
}
