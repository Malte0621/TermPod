#include "pod_common.h"
#include "epd.h"

bool pod_is_epd(char* ident)
{
	pod_ident_type_t type = pod_type(ident);
	return (type >= 0 && type == EPD);
}

pod_checksum_t pod_crc_epd(pod_file_epd_t* file)
{
	if(file == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_epd() file == NULL!");
		return 0;
	}
	pod_byte_t* start = (pod_byte_t*)&file->data + POD_HEADER_EPD_SIZE;
	pod_size_t size = file->size - POD_HEADER_EPD_SIZE;
	// fprintf(stderr, "CRC of data at %p of size %lu!\n", start, size);
	return pod_crc(start, size);
}

pod_checksum_t pod_crc_epd_entry(pod_file_epd_t* file, pod_number_t entry_index)
{
	if(file == NULL || file->entry_data == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_epd() file == NULL!");
		return 0;
	}

	pod_byte_t* start = (pod_byte_t*)file->data + file->entries[entry_index].offset;
	pod_number_t size = file->entries[entry_index].size;
	return pod_crc(start, size);
}

pod_checksum_t  pod_file_epd_chksum(pod_file_epd_t* podfile)
{
	return pod_crc_epd(podfile);
}

pod_signed_number32_t pod_entry_epd_adjacent_diff(const void* a, const void* b)
{
	
	const pod_entry_epd_t *x = a;
	const pod_entry_epd_t *y = b;
	pod_number_t dx = x->offset + x->size;
	pod_number_t dy = y->offset;
	return (dx == dy ? 0 : dx < dy ? -(dy-dx) : dx - dy);
}

pod_bool_t pod_file_epd_update_sizes(pod_file_epd_t* pod_file)
{
	/* check arguments and allocate memory */
	size_t num_entries = pod_file->header->file_count;

	if(pod_file->gap_sizes)
		free(pod_file->gap_sizes);

	pod_file->gap_sizes = calloc(num_entries + 1, sizeof(pod_number_t));

	if(pod_file->gap_sizes == NULL)
	{
		fprintf(stderr, "Unable to allocate memory for gap_sizes\n");
		return false;
	}

	pod_number_t* offsets = calloc(num_entries, sizeof(pod_number_t));
	pod_number_t* min_indices = calloc(num_entries, sizeof(pod_number_t));
	pod_number_t* ordered_offsets = calloc(num_entries, sizeof(pod_number_t));
	pod_number_t* offset_sizes = calloc(num_entries, sizeof(pod_number_t));

	/* sort offset indices and accumulate entry size */
	for(pod_number_t i = 0; i < num_entries; i++)
		offsets[i] = pod_file->entries[i].offset;

	for(pod_number_t i = 0; i < num_entries; i++)
	{
		for(pod_number_t j = i; j < num_entries; j++)
		{
			if(offsets[j] <= offsets[i])
				min_indices[i] = j;
		}
	}

	pod_file->entry_data_size = 0;
	for(pod_number_t i = 0; i < num_entries; i++)
	{
		ordered_offsets[i] = offsets[min_indices[i]];
		offset_sizes[i] = pod_file->entries[min_indices[i]].size;
		pod_file->entry_data_size += offset_sizes[i];
	}

	/* find gap sizes */ 
	for(pod_number_t i = 0; i < num_entries; i++)
	{
		pod_file->gap_sizes[i] = ((i == num_entries - 1) ? pod_file->size : ordered_offsets[i + 1]) - (ordered_offsets[i] + offset_sizes[i]);
		pod_file->gap_sizes[num_entries] += pod_file->gap_sizes[i];

		fprintf(stderr, "gap[%u]=%u accum:%u\n", i, pod_file->gap_sizes[i], pod_file->gap_sizes[num_entries]);
	}

	/* check data start */
	pod_file->data_start = pod_file->data + offsets[min_indices[0]];

	/* cleanup */
	free(offset_sizes);
	free(ordered_offsets);
	free(min_indices);
	free(offsets);


	/* compare accumulated entry sizes + gap_sizes[0] to index_offset - header */
	pod_number_t size = pod_file->entry_data_size + pod_file->gap_sizes[num_entries];
	pod_number_t expected_size = pod_file->size - POD_HEADER_EPD_SIZE - POD_DIR_ENTRY_EPD_SIZE * num_entries; 
	pod_number_t sum_size = size + POD_HEADER_EPD_SIZE;
	/* status output */
	fprintf(stderr, "data_start: %lu/%lu\n", pod_file->entry_data - pod_file->data, pod_file->data_start - pod_file->data);
	fprintf(stderr, "accumulated_size: %u/%u\naccumulated gap size: %u\n", size, expected_size, pod_file->gap_sizes[num_entries]);
	fprintf(stderr, "data_start + size: %u + %u = %u", (pod_number_t)POD_HEADER_EPD_SIZE, size, sum_size);

	return size == expected_size;
}

pod_file_epd_t* pod_file_epd_create(pod_string_t filename)
{
	pod_file_epd_t* pod_file = calloc(1, sizeof(pod_file_epd_t));

	if (dir_exists(filename))
	{
		pod_file->data = calloc(1, POD_HEADER_EPD_SIZE);
		pod_file->header = (pod_header_epd_t*)pod_file->data;
		memcpy(pod_file->header, "dtxe", POD_IDENT_SIZE);
		pod_file->header->file_count = 0;
		pod_file->header->version = 0;
		pod_file->header->checksum = 0;
		memset(pod_file->header->comment, 0, POD_COMMENT_SIZE);
		pod_file->header->comment[0] = '\0';
		pod_file->header->comment[POD_COMMENT_SIZE - 1] = '\0';

		pod_file->entries = calloc(1, sizeof(pod_entry_epd_t));
		pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_EPD_SIZE + pod_file->header->file_count * POD_DIR_ENTRY_EPD_SIZE);
		pod_file->entry_data_size = 0;
		pod_file->audit_trail = calloc(1, sizeof(pod_audit_entry_epd_t));
		pod_file->data = calloc(1, sizeof(pod_byte_t));
		pod_file->size = 0;
		pod_file->checksum = 0;
		return pod_file;
	}

	struct stat sb;
	if(stat(filename, &sb) != 0 || sb.st_size == 0)
	{
		perror("stat");
		return NULL;
	}

	pod_file->filename = strdup(filename);
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
		return pod_file_epd_delete(pod_file);
	}

	if(fread(pod_file->data, POD_BYTE_SIZE, POD_IDENT_SIZE, file) != POD_IDENT_SIZE * POD_BYTE_SIZE)
	{
		fprintf(stderr, "ERROR: Could not read file magic %s!\n", filename);
		fclose(file);
		return pod_file_epd_delete(pod_file);
	}
	if(!pod_is_epd(pod_file->data))
	{
		fprintf(stderr, "ERROR: POD file format is not EPD %s!\n", filename);
		fclose(file);
		return pod_file_epd_delete(pod_file);
	}

	if(fread(pod_file->data + POD_IDENT_SIZE, POD_BYTE_SIZE, EPD_COMMENT_SIZE, file) != EPD_COMMENT_SIZE * POD_BYTE_SIZE)
	{
		fprintf(stderr, "ERROR: Could not read file comment %s!\n", filename);
		fclose(file);
		return pod_file_epd_delete(pod_file);
	}
	if(fread(pod_file->data + POD_IDENT_SIZE + EPD_COMMENT_SIZE , POD_NUMBER_SIZE, 3, file) != 3)
	{
		fprintf(stderr, "ERROR: Could not read file header %s!\n", filename);
		fclose(file);
		return pod_file_epd_delete(pod_file);
	}
	if(fread(pod_file->data + POD_HEADER_EPD_SIZE, POD_BYTE_SIZE, pod_file->size - POD_HEADER_EPD_SIZE, file) != (pod_file->size - POD_HEADER_EPD_SIZE ) * POD_BYTE_SIZE)
	{
		fprintf(stderr, "ERROR: Could not read file %s!\n", filename);
		fclose(file);
		return pod_file_epd_delete(pod_file);
	}

	fclose(file);
	pod_file->checksum = pod_crc(pod_file->data, pod_file->size);

	pod_file->header = (pod_header_epd_t*)pod_file->data;
	pod_file->entries = (pod_entry_epd_t*)(pod_file->data + POD_HEADER_EPD_SIZE);
	pod_number_t num_entries = pod_file->header->file_count;

	pod_number_t min_entry_index = 0;
	pod_number_t max_entry_index = 0;

	for(pod_number_t i = 0; i < num_entries; i++)
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
	pod_file->entry_data = (pod_byte_t*)(pod_file->data + pod_file->entries[min_entry_index].offset);

	size_t max_entry_len = pod_file->entries[max_entry_index].size;

	if(!pod_file_epd_update_sizes(pod_file))
	{
		fprintf(stderr, "ERROR: Could not update EPD file entry sizes\n");
		pod_file = pod_file_epd_delete(pod_file);
	}

	return pod_file;
}

bool pod_file_epd_add_entry(pod_file_epd_t* pod_file, pod_entry_epd_t* entry, pod_string_t filename, pod_byte_t* data)
{
	if (pod_file == NULL || entry == NULL || data == NULL)
	{
		fprintf(stderr, "ERROR: pod_file, entry or data equals NULL!\n");
		return false;
	}

	size_t new_entry_size = POD_DIR_ENTRY_EPD_SIZE;
	size_t new_entry_data_size = entry->size;
	size_t new_path_data_size = strlen(filename) + 1;
	size_t new_total_size = pod_file->size + new_entry_data_size;

	void* new_data = realloc(pod_file->data, new_total_size);
	if (!new_data) {
		return false;
	}
	pod_file->data = new_data;

	pod_file->header = (pod_header_epd_t*)pod_file->data;
	pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_EPD_SIZE);

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

	//pod_file->checksum = pod_crc_epd(pod_file);

	return true;
}

bool pod_file_epd_del_entry(pod_file_epd_t* pod_file, pod_number_t entry_index)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_epd_del_entry(pod_file == NULL)\n");
		return false;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_epd_del_entry(entry_index >= pod_file->header->file_count)\n");
		return false;
	}

	pod_entry_epd_t* entry = &pod_file->entries[entry_index];
	pod_file->entry_data_size -= entry->size;
	pod_file->entry_data = realloc(pod_file->entry_data, pod_file->entry_data_size);

	if (entry_index < pod_file->header->file_count - 1)
	{
		memmove(pod_file->entries + entry_index, pod_file->entries + entry_index + 1, (pod_file->header->file_count - entry_index - 1) * POD_DIR_ENTRY_EPD_SIZE);
	}

	pod_file->header->file_count--;
	pod_file->entries = realloc(pod_file->entries, pod_file->header->file_count * POD_DIR_ENTRY_EPD_SIZE);

	//pod_file->checksum = pod_crc_epd(pod_file);

	return true;
}

pod_entry_epd_t* pod_file_epd_get_entry(pod_file_epd_t* pod_file, pod_number_t entry_index)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_epd_get_entry(pod_file == NULL)\n");
		return NULL;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_epd_get_entry(entry_index >= pod_file->header->file_count)\n");
		return NULL;
	}

	return &pod_file->entries[entry_index];
}

bool pod_file_epd_extract_entry(pod_file_epd_t* pod_file, pod_number_t entry_index, pod_string_t dst)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_epd_extract_entry(pod_file == NULL)\n");
		return false;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_epd_extract_entry(entry_index >= pod_file->header->file_count)\n");
		return false;
	}

	FILE* file = fopen(dst, "wb");

	if (file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_epd_extract_entry(fopen(%s, \"wb\") failed: %s\n", dst, strerror(errno));
		return false;
	}

	pod_entry_epd_t* entry = &pod_file->entries[entry_index];
	if (fwrite(pod_file->data + entry->offset, entry->size, 1, file) != 1)
	{
		fprintf(stderr, "ERROR: pod_file_epd_extract_entry(fwrite failed!\n");
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

pod_file_epd_t* pod_file_epd_delete(pod_file_epd_t* podfile)
{
	if(podfile)
	{
		if(podfile->gap_sizes)
		{
			free(podfile->gap_sizes);
			podfile->gap_sizes = NULL;
		}
		if(podfile->data)
		{
			free(podfile->data);
			podfile->data = NULL;
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
		fprintf(stderr, "ERROR: could not free podfile == NULL!\n");

	return podfile;
}

bool pod_file_epd_print(pod_file_epd_t* pod_file, pod_char_t* pattern)
{
	if(pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_epd_print() podfile == NULL\n");
		return false;
	}

	/* print entries */
	printf("\nEntries:\n");
	for(pod_number_t i = 0; i < pod_file->header->file_count; i++)
	{
		pod_entry_epd_t* entry = &pod_file->entries[i];
		pod_char_t* name = pod_file->entries[i].name;
		if (pattern && strstr(name, pattern) == NULL)
			continue;
		printf("%10u %10u %10u 0x%.8X/0x%.8X %s %s\n",
		       	i,
			entry->offset,
			entry->size,
			entry->checksum, pod_crc_epd_entry(pod_file, i),
			pod_ctime(&entry->timestamp),
			name);
	}

	/* print file summary */
	printf("\nSummary:\n \
	        file checksum      : 0x%.8X/%.10u\n \
	        size               : %zu\n \
		filename           : %s\n \
		format             : %s\n \
		comment            : %s\n \
		data checksum      : 0x%.8X/0x%.8X\n \
		file entries       : 0x%.8X/%.10u\n \
		version            : 0x%.8X/%.10u\n",
		pod_file->checksum,pod_file->checksum,
		pod_file->size,
		pod_file->filename,
		pod_type_desc_str(pod_type(pod_file->header->ident)),
		pod_file->header->comment,
	        pod_file->header->checksum, pod_file->header->checksum,
		pod_file->header->file_count,pod_file->header->file_count,
		pod_file->header->version,pod_file->header->version);
	
	return true;
}

bool pod_file_epd_write(pod_file_epd_t* pod_file, pod_string_t filename)
{
	if (pod_file == NULL || filename == NULL)
	{
		fprintf(stderr, "ERROR: pod_file or filename equals NULL!\n");
		return false;
	}
	
	size_t new_total_size = pod_file->size + pod_file->header->file_count * POD_DIR_ENTRY_EPD_SIZE;
	void* new_data = realloc(pod_file->data, new_total_size);
	if (!new_data) {
		fprintf(stderr, "ERROR: extending data entries!\n");
		return false;
	}
	pod_file->data = new_data;

	pod_file->header = (pod_header_epd_t*)pod_file->data;
	pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_EPD_SIZE);

	// apply all entries to the end of the pod_file->data.

	memcpy(pod_file->data + pod_file->size, pod_file->entries, pod_file->header->file_count * POD_DIR_ENTRY_EPD_SIZE);
	pod_file->size += pod_file->header->file_count * POD_DIR_ENTRY_EPD_SIZE;

	pod_file->entries = (pod_entry_epd_t*)(pod_file->data + POD_DIR_ENTRY_EPD_SIZE);

	FILE* file = fopen(filename, "wb");

	if (fwrite(pod_file->data, pod_file->size, 1, file) != 1)
	{
		fprintf(stderr, "ERROR: writing EPD data!\n");
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

/* Extract EPD file pod_file to directory dst                       */
/* @returns true on success otherwise false and leaves errno         */
bool pod_file_epd_extract(pod_file_epd_t* pod_file, pod_string_t pattern, pod_string_t dst)
{
	pod_bool_t absolute = false;
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_epd_extract(pod_file == NULL)\n");
		return false;
	}

	if (dst == NULL) { dst = "./"; }

	pod_char_t cwd[POD_SYSTEM_PATH_SIZE];
	getcwd(cwd, sizeof(cwd));
	printf("cwd: %s\n", cwd);
	pod_path_t root = pod_path_system_root();

	printf("root: %s\n", root);
	pod_path_t path = NULL;

	if (absolute)
	{
		path = pod_path_append_win32(root, dst);
		if (!path)
		{
			fprintf(stderr, "ERROR: pod_path_append_win32(%s,%s)\n", root, dst);
			free(root);
			return false;
		}
	}
	else
	{
		path = pod_path_append_win32(cwd, dst);
		if (!path)
		{
			fprintf(stderr, "ERROR: pod_path_append_win32(%s,%s)\n", cwd, dst);
			free(root);
			return false;
		}
	}

	printf("path: %s\n", path);
	/* create and change to destination directory */
	if (mkdir_p(path, ACCESSPERMS) != 0)
	{
		fprintf(stderr, "mkdir_p(\"%s\", \'%c\') = %s\n", dst, '/', strerror(errno));
		return false;
	}

	if (chdir(path) != 0)
	{
		fprintf(stderr, "chdir(%s) = %s\n", path, strerror(errno));
		return false;
	}

	/* extract entries */
	for (int i = 0; i < pod_file->header->file_count; i++)
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
		if (strlen(basename) > 0 && mkdir_p(pod_path_append(path, basename), ACCESSPERMS) != 0)
		{
			fprintf(stderr, "ERROR: mkdir_p(%s) failed: %s\n", basename, strerror(errno));
			return false;
		}
		FILE* file = fopen(pod_file->entries[i].name, "wb");
		if (file == NULL)
		{
			fprintf(stderr, "ERROR: pod_fopen_mkdir(%s) failed: %s\n", pod_file->entries[i].name, strerror(errno));
			return false;
		}
		if (fwrite(pod_file->data + pod_file->entries[i].offset, pod_file->entries[i].size, 1, file) != 1)
		{
			fprintf(stderr, "ERROR: fwrite failed!\n");
			fclose(file);
			return false;
		}

		/* Debug/Info print thingy */
		fprintf(stderr, "%s -> %s\n", pod_file->entries[i].name, pod_path_append_win32(path, pod_file->entries[i].name));

		/* clean up and pop */
		fclose(file);
	}
	chdir(cwd);
	return true;
}