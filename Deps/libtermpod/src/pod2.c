#include "pod_common.h" 
#include "pod2.h" 

const char pod2_audit_action_string[POD2_AUDIT_ACTION_SIZE][8] = { "Add", "Remove", "Change" };

pod_checksum_t pod_crc_pod2(pod_file_pod2_t* file)
{
	if(file == NULL || file->path_data == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_pod2() file == NULL!");
		return 0;
	}

	return pod_crc(file->data + POD_IDENT_SIZE + POD_HEADER_CHECKSUM_SIZE, file->size - POD_IDENT_SIZE - POD_HEADER_CHECKSUM_SIZE);
}

pod_checksum_t pod_crc_pod2_entry(pod_file_pod2_t* file, pod_number_t entry_index)
{
	if(file == NULL || file->entry_data == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_pod2() file == NULL!");
		return 0;
	}

	return pod_crc(file->data + file->entries[entry_index].offset, file->entries[entry_index].size);
}
 
pod_checksum_t   pod_file_pod2_chksum(pod_file_pod2_t* podfile)
{
	return pod_crc_pod2(podfile);
}

bool pod_is_pod2(char* ident)
{
  return (POD2 == pod_type(ident) >= 0);
}

bool pod_audit_entry_pod2_print(pod_audit_entry_pod2_t* audit)
{
	if(audit == NULL)
	{
		fprintf(stderr, "ERROR: pod_audit_entry_pod2_print(audit == NULL)!\n");
		return false;
	}

	printf("\n%s %s\n%s %s\n%u / %u\n%s / %s\n",
		audit->user,
		pod_ctime(&audit->timestamp),
		pod2_audit_action_string[audit->action],
		audit->path,
		audit->old_size,
		audit->new_size,
		pod_ctime(&audit->old_timestamp),
		pod_ctime(&audit->new_timestamp));
	return true;
}

bool pod_file_pod2_print(pod_file_pod2_t* pod_file, pod_char_t* pattern)
{
	if(pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_print() podfile == NULL\n");
		return false;
	}

	/* print entries */
	printf("\nEntries:\n");
	for(pod_number_t i = 0; i < pod_file->header->file_count; i++)
	{
		pod_entry_pod2_t* entry = &pod_file->entries[i];
		pod_char_t* name = pod_file->path_data + entry->path_offset;
		if (pattern && strstr(name, pattern) == NULL)
			continue;
		printf("%10u %10u %.8X/%.8X %10u %s %s %10u\n",
		       	i,
			entry->offset,
			entry->checksum,
			pod_crc_pod2_entry(pod_file, i),
			entry->size,
			pod_ctime(&entry->timestamp),
			name,
			entry->path_offset);
	}

	printf("\nAudit:\n");
	/* print audit trail */
	for(int i = 0; i < pod_file->header->audit_file_count; i++)
	{
		if(!pod_audit_entry_pod2_print(&pod_file->audit_trail[i]))
		{
			fprintf(stderr, "ERROR: pod_audit_entry_pod2_print() failed!");
			pod_file = pod_file_pod2_delete(pod_file);
			return false;
		}
	}

	/* print file summary */
	printf("\nSummary:\nfile checksum      : 0x%.8X\nsize               : %zu\nfilename           : %s\nformat             : %s\ncomment            : %s\ndata checksum      : 0x%.8X/0x%.8X\nfile entries       : 0x%.8X/%.10u\naudit entries      : 0x%.8X/%.10u\n",
		pod_file->checksum,
		pod_file->size,
		pod_file->filename,
		pod_type_desc_str(pod_type(pod_file->header->ident)),
		pod_file->header->comment,
		pod_file->header->checksum,
		pod_crc_pod2(pod_file),
		pod_file->header->file_count,pod_file->header->file_count,
		pod_file->header->audit_file_count,pod_file->header->audit_file_count);

	
	return true;
}

pod_file_pod2_t* pod_file_pod2_delete(pod_file_pod2_t* podfile)
{
	if(podfile != NULL)
	{
		if(podfile->data)
		{
			free(podfile->data);
			podfile->data = NULL;
			podfile->header = NULL;
			podfile->entries = NULL;
			podfile->path_data = NULL;
			podfile->entry_data = NULL;
			podfile->audit_trail = NULL;
		}
		if(podfile->filename)
		{
			free(podfile->filename);
			podfile->filename = NULL;
		}

		free(podfile);
		podfile = NULL;
	}
	else
		fprintf(stderr, "ERROR: pod_file_pod2_delete(podfile) podfile already equals NULL!\n");

	return podfile;
}

pod_file_pod2_t* pod_file_pod2_create(pod_string_t filename)
{
	pod_file_pod2_t* pod_file = calloc(1, sizeof(pod_file_pod2_t));

	if (dir_exists(filename))
	{
		pod_file->data = calloc(1, POD_HEADER_POD2_SIZE);
		pod_file->header = (pod_header_pod2_t*)pod_file->data;
		memcpy(pod_file->header, "POD2", POD_IDENT_SIZE);
		pod_file->header->checksum = 0;
		pod_file->header->file_count = 0;
		memset(pod_file->header->comment, 0, POD_COMMENT_SIZE);
		pod_file->header->comment[0] = '\0';
		pod_file->header->comment[POD_COMMENT_SIZE - 1] = '\0';
		pod_file->header->audit_file_count = 0;

		pod_file->entries = calloc(1, sizeof(pod_entry_pod2_t));
		pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_POD2_SIZE + pod_file->header->file_count * POD_DIR_ENTRY_POD2_SIZE);
		pod_file->entry_data_size = 0;
		pod_file->path_data = calloc(1, sizeof(pod_char_t));
		pod_file->audit_trail = calloc(1, sizeof(pod_audit_entry_pod2_t));
		pod_file->data = calloc(1, sizeof(pod_byte_t));
		pod_file->checksum = 0;
		pod_file->size = POD_HEADER_POD2_SIZE;
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
		return pod_file_pod2_delete(pod_file);
	}

	if(fread(pod_file->data, POD_BYTE_SIZE, pod_file->size, file) != pod_file->size * POD_BYTE_SIZE)
	{
		fprintf(stderr, "ERROR: Could not read file %s!\n", filename);
		fclose(file);
		return pod_file_pod2_delete(pod_file);
	}

	fclose(file);
	pod_file->checksum = pod_crc(pod_file->data, pod_file->size);

	size_t data_pos = 0;
	pod_file->header = (pod_header_pod2_t*)pod_file->data;
	data_pos += POD_HEADER_POD2_SIZE;
	pod_file->entries = (pod_entry_pod2_t*)(pod_file->data + data_pos);
	data_pos += pod_file->header->file_count * POD_DIR_ENTRY_POD2_SIZE;

	pod_number_t min_path_index = 0;
	pod_number_t max_path_index = 0;
	pod_number_t min_entry_index = 0;
	pod_number_t max_entry_index = 0;

	for(pod_number_t i = 0; i < pod_file->header->file_count; i++)
	{
		if(pod_file->entries[i].path_offset < pod_file->entries[min_path_index].path_offset)
		{
			min_path_index = i;
		}
		if(pod_file->entries[i].path_offset > pod_file->entries[max_path_index].path_offset)
		{
			max_path_index = i;
		}
		if(pod_file->entries[i].offset < pod_file->entries[min_entry_index].offset)
		{
			min_entry_index = i;
		}
		if(pod_file->entries[i].offset > pod_file->entries[max_entry_index].offset)
		{
			max_entry_index = i;
		}
	}


	pod_file->path_data = (pod_char_t*) (pod_file->data + data_pos);
	size_t max_path_len = strlen(pod_file->path_data + pod_file->entries[max_path_index].path_offset) + 1;

	pod_file->path_data_size = (pod_file->path_data + pod_file->entries[max_path_index].path_offset + max_path_len) - 
				(pod_file->path_data + pod_file->entries[min_entry_index].path_offset);

	size_t max_entry_len = pod_file->entries[max_entry_index].size;
	pod_file->entry_data_size = (pod_file->data + pod_file->entries[max_entry_index].offset + max_entry_len) - 
				 (pod_file->data + pod_file->entries[min_entry_index].offset);

	pod_file->entry_data = pod_file->data + pod_file->entries[min_entry_index].offset;

	data_pos += pod_file->path_data_size + pod_file->entry_data_size;

	pod_file->audit_trail = (pod_audit_entry_pod2_t*)(pod_file->data + data_pos);

	return pod_file;
}

bool pod_file_pod2_add_entry(pod_file_pod2_t* pod_file, pod_entry_pod2_t* entry, pod_string_t filename, pod_byte_t* data)
{
	if(pod_file == NULL || entry == NULL || data == NULL)
	{
		fprintf(stderr, "ERROR: pod_file, entry or data equals NULL!\n");
		return false;
	}

	size_t new_entry_size = POD_DIR_ENTRY_POD2_SIZE;
	size_t new_entry_data_size = entry->size;
	size_t new_path_data_size = strlen(filename) + 1;
	size_t new_total_size = pod_file->size + new_entry_data_size;

	void* new_data = realloc(pod_file->data, new_total_size);
	if (!new_data) {
		return false;
	}
	pod_file->data = new_data;

	pod_file->header = (pod_header_pod2_t*)pod_file->data;
	pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_POD2_SIZE);

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

	// re-allocate path_data
	void* new_path_data = realloc(pod_file->path_data, pod_file->path_data_size + new_path_data_size);
	if (!new_path_data) {
		return false;
	}
	pod_file->path_data = new_path_data;
	// copy new path data to end of path_data
	memcpy(pod_file->path_data + pod_file->path_data_size, filename, strlen(filename) + 1);
	pod_file->path_data_size += new_path_data_size;

	//pod_file->checksum = pod_crc_pod2(pod_file);

	return true;
}

bool pod_file_pod2_del_entry(pod_file_pod2_t* pod_file, pod_number_t entry_index)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_del_entry(pod_file == NULL)\n");
		return false;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_del_entry(entry_index >= pod_file->header->file_count)\n");
		return false;
	}

	pod_entry_pod2_t* entry = &pod_file->entries[entry_index];
	pod_file->entry_data_size -= entry->size;
	pod_file->entry_data = realloc(pod_file->entry_data, pod_file->entry_data_size);

	if (entry_index < pod_file->header->file_count - 1)
	{
		memmove(pod_file->entries + entry_index, pod_file->entries + entry_index + 1, (pod_file->header->file_count - entry_index - 1) * POD_DIR_ENTRY_POD2_SIZE);
	}

	pod_file->header->file_count--;
	pod_file->entries = realloc(pod_file->entries, pod_file->header->file_count * POD_DIR_ENTRY_POD2_SIZE);

	//pod_filek->checksum = pod_crc_pod2(pod_file);

	return true;
}

pod_entry_pod2_t* pod_file_pod2_get_entry(pod_file_pod2_t* pod_file, pod_number_t entry_index)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_get_entry(pod_file == NULL)\n");
		return NULL;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_get_entry(entry_index >= pod_file->header->file_count)\n");
		return NULL;
	}

	return &pod_file->entries[entry_index];
}

bool pod_file_pod2_extract_entry(pod_file_pod2_t* pod_file, pod_number_t entry_index, pod_string_t dst)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_extract_entry(pod_file == NULL)\n");
		return false;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_extract_entry(entry_index >= pod_file->header->file_count)\n");
		return false;
	}

	FILE* file = fopen(dst, "wb");

	if (file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_extract_entry(fopen(%s, \"wb\") failed: %s\n", dst, strerror(errno));
		return false;
	}

	pod_entry_pod2_t* entry = &pod_file->entries[entry_index];
	if (fwrite(pod_file->data + entry->offset, entry->size, 1, file) != 1)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_extract_entry(fwrite failed!\n");
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

bool pod_file_pod2_write(pod_file_pod2_t* pod_file, pod_string_t filename)
{
	if(pod_file == NULL || filename == NULL)
	{
		fprintf(stderr, "ERROR: pod_file or filename equals NULL!\n");
		return false;
	}

	// apply all path_data to the end of the pod_file->data.
	// start at the end of the file and work backwards

	pod_file->path_data_size = 0;
	for (int i = pod_file->header->file_count - 1; i >= 0; i--)
	{
		pod_entry_pod2_t* entry = &pod_file->entries[i];

		pod_char_t* name = NULL;
		size_t name_cursor = 0;
		for (int j = 0; j < i + 1; j++)
		{
			size_t name_size = strlen(pod_file->path_data + name_cursor) + 1;
			if (j == i)
			{
				name = pod_file->path_data + name_cursor;
				break;
			}
			name_cursor += name_size;
		}

		size_t name_size = strlen(name) + 1;
		pod_file->path_data_size += name_size;
	}

	size_t new_total_size = pod_file->size + pod_file->path_data_size + pod_file->header->file_count * POD_DIR_ENTRY_POD2_SIZE;
	void* new_data = realloc(pod_file->data, new_total_size);
	if (!new_data) {
		fprintf(stderr, "ERROR: extending data entries!\n");
		return false;
	}
	pod_file->data = new_data;

	pod_file->header = (pod_header_pod2_t*)pod_file->data;

	// apply all entries to the end of the pod_file->data.

	memcpy(pod_file->data + pod_file->size, pod_file->entries, pod_file->header->file_count * POD_DIR_ENTRY_POD2_SIZE);
	pod_file->size += pod_file->header->file_count * POD_DIR_ENTRY_POD2_SIZE;

	pod_file->entries = (pod_entry_pod2_t*)(pod_file->data + POD_DIR_ENTRY_POD2_SIZE);

	pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_POD2_SIZE);

	size_t path_data_size2 = 0;

	for (int i = pod_file->header->file_count - 1; i >= 0; i--)
	{
		pod_entry_pod2_t* entry = &pod_file->entries[i];

		pod_char_t* name = NULL;
		size_t name_cursor = 0;
		for (int j = 0; j < i + 1; j++)
		{
			size_t name_size = strlen(pod_file->path_data + name_cursor) + 1;
			if (j == i)
			{
				name = pod_file->path_data + name_cursor;
				break;
			}
			name_cursor += name_size;
		}

		size_t name_size = strlen(name) + 1;
		entry->path_offset = path_data_size2;
		memcpy(pod_file->data + pod_file->size + path_data_size2, name, name_size);
		path_data_size2 += name_size;
	}
	pod_file->size += pod_file->path_data_size;
	pod_file->path_data = (pod_char_t*)(pod_file->data + pod_file->size - pod_file->path_data_size);

	FILE* file = fopen(filename, "wb");

	if (fwrite(pod_file->data, pod_file->size, 1, file) != 1)
	{
		fprintf(stderr, "ERROR: writing POD2 data!\n");
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

/* Extract POD2 file pod_file to directory dst                       */
/* @returns true on success otherwise false and leaves errno         */
bool pod_file_pod2_extract(pod_file_pod2_t* pod_file, pod_string_t pattern, pod_string_t dst)
{
	pod_bool_t absolute = false;
	if(pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod2_extract(pod_file == NULL)\n");
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
		pod_string_t filename = strdup(pod_file->path_data + pod_file->entries[i].path_offset);
		if(!filename)
		{
			fprintf(stderr, "ERROR: pod_file_pod2_extract(filename == NULL)\n");
			return false;
		}
		if (pattern && strstr(filename, pattern) == NULL)
		{
			free(filename);
			continue;
		}

		pod_string_t basename = pod_path_dirname(filename);
		if (!basename)
		{
			fprintf(stderr, "ERROR: pod_path_dirname(%s) failed: %s\n", filename, strerror(errno));
			free(filename);
			return false;
		}

		/* open and create directories including parents */
		if(strlen(basename) > 0 && mkdir_p(pod_path_append(path, basename), ACCESSPERMS) != 0)
		{
			fprintf(stderr, "ERROR: mkdir_p(%s) failed: %s\n", basename, strerror(errno));
			return false;
		}
		FILE* file = fopen(filename, "wb");
		if(file == NULL)
		{
			fprintf(stderr, "ERROR: pod_fopen_mkdir(%s) failed: %s\n", filename, strerror(errno));
			return false;
		}
		if(fwrite(pod_file->data + pod_file->entries[i].offset, pod_file->entries[i].size, 1, file) != 1)
		{
			fprintf(stderr, "ERROR: fwrite failed!\n");
			fclose(file);
			return false;
		}

		/* Debug/Info print thingy */
		fprintf(stderr, "%s -> %s\n", filename, pod_path_append(path, filename));

		/* clean up and pop */
		fclose(file);
		free(filename);
	}
	chdir(cwd);
	return true;
}
