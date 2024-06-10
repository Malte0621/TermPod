#include "pod_common.h"
#include "pod3.h"

const char pod3_audit_action_string[POD3_AUDIT_ACTION_SIZE][8] = { "Add", "Remove", "Change" };

bool pod_is_pod3(char* ident)
{
  return (POD3 == pod_type(ident));
}

pod_checksum_t pod_crc_pod3(pod_file_pod3_t* file)
{
	if(file == NULL || file->path_data == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_pod3() file == NULL!");
		return 0;
	}
	pod_size_t offset = 0x412;
	pod_size_t size = file->size - offset;
	fprintf(stderr, "CRC of data at %p of size %zu!\n", file->data + offset, size);
	return pod_crc(file->data + offset, size);
}

pod_checksum_t pod_crc_pod3_off(pod_file_pod3_t* file, pod_size_t offset)
{
	if(file == NULL || file->path_data == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_pod3() file == NULL!");
		return 0;
	}
	pod_size_t size = file->size - offset;
	fprintf(stderr, "CRC of data at %p of size %zu!\n", file->data + offset, size);
	return pod_crc(file->data + offset, size);
}

pod_checksum_t pod_crc_pod3_entry(pod_file_pod3_t* file, pod_number_t entry_index)
{
	if(file == NULL || file->entry_data == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_pod3() file == NULL!");
		return 0;
	}

	pod_byte_t* start = (pod_byte_t*)file->data + file->entries[entry_index].offset;
	pod_number_t size = file->entries[entry_index].size;

	return pod_crc(start, size);
}

pod_checksum_t pod_crc_pod3_audit_entry(pod_file_pod3_t* file, pod_number_t audit_entry_index)
{
	if(file == NULL || file->entry_data == NULL)
	{
		fprintf(stderr, "ERROR: pod_crc_pod3() file == NULL!");
		return 0;
	}

	pod_byte_t* start = (pod_byte_t*)&file->audit_trail[audit_entry_index];
	pod_number_t size = POD_AUDIT_ENTRY_POD3_SIZE;

	return pod_crc(start, size);
}

pod_checksum_t   pod_file_pod3_chksum(pod_file_pod3_t* podfile)
{
	return pod_crc_pod3(podfile);
}

pod_signed_number32_t pod_entry_pod3_adjacent_diff(const void* a, const void* b)
{
	
	const pod_entry_pod3_t *x = a;
	const pod_entry_pod3_t *y = b;
	pod_number_t dx = x->offset + x->size;
	pod_number_t dy = y->offset;
	return (dx == dy ? 0 : dx < dy ? -(dy-dx) : dx - dy);
}

pod_bool_t pod_file_pod3_update_sizes(pod_file_pod3_t* pod_file)
{
	/* check arguments and allocate memory */
	size_t num_entries = pod_file->header->file_count;

	if(pod_file->gap_sizes)
		free(pod_file->gap_sizes);

	pod_file->gap_sizes = calloc(num_entries, sizeof(pod_number_t));

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
	for(pod_number_t i = 1; i < num_entries; i++)
	{
		pod_file->gap_sizes[i] = ordered_offsets[i] - (ordered_offsets[i - 1] + offset_sizes[i - 1]);
		pod_file->gap_sizes[0] += pod_file->gap_sizes[i];
		printf("gap[%d]=%d\n", i, pod_file->gap_sizes[i]);
	}


	pod_size_t expected_size = pod_file->header->index_offset - POD_HEADER_POD3_SIZE;
	pod_size_t size = pod_file->entry_data_size + pod_file->gap_sizes[0];
	/* check data start */
	pod_file->data_start = pod_file->data + pod_file->entries[min_indices[0]].offset;

	/* cleanup */
	free(offset_sizes);
	free(ordered_offsets);
	free(min_indices);
	free(offsets);


	/* compare accumulated entry sizes + gap_sizes[0] to index_offset - header */
	pod_size_t sum_size = expected_size + POD_HEADER_POD3_SIZE + pod_file->header->size_index;
	/* status output */
	fprintf(stderr, "data_start: %p/%p\n", pod_file->data_start, pod_file->data + POD_HEADER_POD3_SIZE);
	fprintf(stderr, "size_diff: %zu/%u\n", size - expected_size, pod_file->gap_sizes[0]);
	fprintf(stderr, "index_offset + size_index: %u + %u = %zu\n", pod_file->header->index_offset, pod_file->header->size_index,
	sum_size);

	return size == expected_size;
}

/*
pod_file_pod3_t* pod_file_pod3_create(pod_string_t filename)
{
	pod_file_pod3_t* pod_file = calloc(1, sizeof(pod_file_pod3_t));
	struct stat sb;
	if(stat(filename, &sb) != 0 || sb.st_size == 0)
	{
		perror("stat");
		return NULL;
	}

	pod_file->filename = strdup(filename);
	pod_file->size = sb.st_size;
	size_t read = 0;
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
		pod_file_pod3_destroy(pod_file);
		return NULL;
	}

	if( (read = fread(pod_file->data, POD_BYTE_SIZE, pod_file->size, file)) != pod_file->size)
	{
		fprintf(stderr, "ERROR: Stopped reading file %s at %zu of %zu bytes!\n", filename, read, pod_file->size);
		fclose(file);
		pod_file_pod3_destroy(pod_file);
		return NULL;
	}

	pod_file->header = (pod_header_pod3_t*)pod_file->data;

	if(!pod_is_pod3(pod_file->header->ident))
	{
		fprintf(stderr, "ERROR: File %s is missing POD3 file magic!\n", filename);
		fclose(file);
		pod_file_pod3_destroy(pod_file);
		return NULL;
	}

	fclose(file);

	pod_file->checksum = pod_crc(pod_file->data, pod_file->size);

	pod_number_t num_entries = pod_file->header->file_count;
	pod_file->entries = (pod_entry_pod3_t*)(pod_file->data +  pod_file->header->index_offset);

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

	pod_file->entry_data = (pod_byte_t*)pod_file->data + pod_file->entries[min_entry_index].offset;
	pod_file->path_data = (pod_char_t*)((pod_byte_t*)pod_file->entries + (POD_DIR_ENTRY_POD3_SIZE * pod_file->header->file_count));

	for(int i = 0; i < pod_file->header->file_count; i++)
		pod_file->path_data + pod_file->entries[i].path_offset;

	size_t max_path_len = strlen((pod_char_t*)(pod_file->entries + pod_file->entries[max_path_index].path_offset)) + 1;
	size_t max_entry_len = pod_file->entries[max_entry_index].size;


	if(pod_file->header->audit_file_count > 0)
	{
		pod_file->audit_trail = (pod_audit_entry_pod3_t*)((pod_byte_t*)pod_file->path_data + pod_file->header->size_index);
		pod_file->audit_data_size = pod_file->header->audit_file_count * POD_AUDIT_ENTRY_POD3_SIZE;
	}

	if(!pod_file_pod3_update_sizes(pod_file))
	{
		fprintf(stderr, "ERROR: Could not update POD3 file entry sizes\n");
		pod_file_pod3_destroy(pod_file);
		return NULL;
	}

	return pod_file;
}
*/

pod_file_pod3_t* pod_file_pod3_create(pod_string_t filename) {
    pod_number_t ecx4;
    pod_number_t edx5;
    pod_number_t edx6;
    pod_number_t esi7;
    pod_number_t v8;
    pod_number_t v9;

    /* read POD data */
    struct stat sb;
    size_t bytes = 0;
    size_t read = 0;

    uint8_t rotor = 0;

	/* allocate memory */
	pod_file_pod3_t* pod_file = calloc(1, sizeof(pod_file_pod3_t));
	if (!pod_file)
	{
		return NULL;
	}

	if (dir_exists(filename))
	{
		pod_file->data = calloc(1, POD_HEADER_POD3_SIZE);
		pod_file->header = (pod_header_pod3_t*)pod_file->data;
		memcpy(pod_file->header, "POD3", POD_IDENT_SIZE);
		pod_file->header->checksum = 0;
		pod_file->header->file_count = 0;
		pod_file->header->index_offset = POD_HEADER_POD3_SIZE;
		memset(pod_file->header->comment, 0, POD_COMMENT_SIZE);
		pod_file->header->comment[0] = '\0';
		pod_file->header->comment[POD_COMMENT_SIZE - 1] = '\0';
		pod_file->header->audit_file_count = 0;
		
		pod_file->entries = calloc(1, sizeof(pod_entry_pod3_t));
		pod_file->entry_data = (pod_byte_t*)(pod_file->data + pod_file->header->index_offset + pod_file->header->file_count * POD_DIR_ENTRY_POD3_SIZE);
		pod_file->entry_data_size = 0;
		pod_file->path_data = calloc(1, sizeof(pod_char_t));
		pod_file->audit_trail = calloc(1, sizeof(pod_audit_entry_pod3_t));
		pod_file->audit_data_size = 0;
		pod_file->gap_sizes = calloc(1, sizeof(pod_number_t));
		pod_file->data = calloc(1, sizeof(pod_byte_t));
		pod_file->checksum = 0;
		pod_file->size = pod_file->header->index_offset;
		return pod_file;
	}

	if (stat(filename, &sb) != 0) {
		free(pod_file);
	    perror("stat");
	    return NULL;
	}

    FILE* file = fopen(filename, "rb");
    if(!file)
	    return NULL;

    pod_file->data = calloc(sb.st_size,sizeof(uint8_t));
    if(!pod_file->data)
    {
    	fclose(file);
	free(pod_file);
    	return NULL;
    }

    pod_file->header = (pod_header_pod3_t*)pod_file->data;
    pod_file->filename = strdup(filename);
    pod_file->size = sb.st_size;


    /* read data as sizeof(pod_file_block_t) steps */
    pod_file_block_t* ptr = (pod_file_block_t*)&pod_file->data[0];

    for(read = 0; read < sb.st_size; read += fread(ptr++, sizeof(uint8_t), sizeof(pod_file_block_t), file))
    {
	    fprintf(stdout, "\rLoading POD file... %lu/%lu (%c)", read, sb.st_size, rotorchar[rotor]); 
	    fflush(stdout);
	    rotor++;
	    rotor&=3;
    }

    if( read != pod_file->size)
    {
	    fprintf(stdout, "\rLoading POD file... %lu/%lu FAILED!\n", read, pod_file->size);
	    fflush(stdout);
	    fclose(file);
	    return pod_file_pod3_delete(pod_file);
    }

    fprintf(stdout, "\rLoading POD file... %lu/%lu SUCCESS!\n", read, sb.st_size, rotorchar[rotor]);

    pod_file->checksum = pod_crc(pod_file->data, pod_file->size);

/*
    for(pod_number_t i = 0; i < pod_file->size; i+=4)
    {
    	uint32_t chksum = pod_crc(pod_file->data + i, pod_file->size - i);
	printf("Checksum offset: %X\n", i);
	if(chksum == pod_file->header->checksum)
		printf("MATCH: %X\n", i);
    }
*/

    fprintf(stdout, "\rCreating POD file checksum... %08x SUCCESS!\n", pod_file->checksum);
    /* read header */
    uint8_t iudiff = *(uint8_t*)&pod_file->header->index_offset - *(uint8_t*)&pod_file->header->size_index;
    uint8_t isdiff = *(int8_t*)&pod_file->header->index_offset - *(int8_t*)&pod_file->header->size_index;
    size_t size = 0;

    if(!pod_is_pod3(pod_file->header->ident))
    {
	    fprintf(stdout, "\rReading POD file magic... %u/%u FAILED!\n", 0,4);
	    fflush(stdout);
	    fclose(file);
	    return pod_file_pod3_delete(pod_file);
    }
    else
    	fprintf(stdout, "\rReading POD file magic... %u/%u SUCCESS!\n", 4,4);

    if((pod_file->header->checksum = *(pod_number_t*)(pod_file->data + 4)) == 0)
	    pod_file->header->checksum = 0xfffffffe;
    else if(*(int8_t*)&pod_file->header->index_offset <= *(int8_t*)&pod_file->header->size_index)
    {
	    printf("Loading index_offset: %02x/%08x size_index: %02x/%08x\n", *(int8_t*)&pod_file->header->index_offset, pod_file->header->index_offset, *(int8_t*)&pod_file->header->size_index, pod_file->header->size_index);
	    pod_file->header->checksum = 0xffffffff;
    }

    if( *(int8_t*)&size > *(int8_t*)&iudiff)
	    size = iudiff;

    pod_file->data_offset = *(uint8_t*)&pod_file->header->size_index + *(uint8_t*)&pod_file->header->pad10c;
    (*(uint16_t*)&pod_file->data_offset) &= 0xf000;
    ecx4 = pod_file->data_offset - *(uint8_t*)&pod_file->header->pad10c;
    edx5 = *(uint8_t*)&pod_file->header->size_index - *(uint8_t*)&ecx4;
    pod_file->header->pad124 = edx5;
    edx6 = edx5 + 0xfff;
    pod_file->header->pad11c = ecx4;
    (*(uint16_t*)&edx6) &= 0xf000;
    esi7 = pod_file->header->pad120;
    pod_file->header->pad124 = edx6;
    if (*(int8_t*)&edx6 > *(int8_t*)&pod_file->header->pad120)
        pod_file->header->pad124 = esi7;

    pod_file->header->pad124 = pod_file->header->checksum;
    /*
    SetFilePointer(header->checksum, data_start, 0, 0);
    v9 = header->checksum;
    ReadFile();
    header->pad124 = v9;
*/
    if (*(int8_t*)&v9 < 1)
        pod_file->header->pad124 = 0;
    
    pod_number_t num_entries = pod_file->header->file_count;
    pod_file->entries = (pod_entry_pod3_t*)(pod_file->data +  pod_file->header->index_offset);

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

    pod_file->entry_data = (pod_byte_t*)pod_file->data + pod_file->entries[min_entry_index].offset;
    pod_file->path_data = (pod_char_t*)((pod_byte_t*)pod_file->entries + (POD_DIR_ENTRY_POD3_SIZE * pod_file->header->file_count));

    for(int i = 0; i < pod_file->header->file_count; i++)
	    pod_file->path_data + pod_file->entries[i].path_offset;

    size_t max_path_len = strlen((pod_char_t*)(pod_file->entries + pod_file->entries[max_path_index].path_offset)) + 1;
    size_t max_entry_len = pod_file->entries[max_entry_index].size;


    if(pod_file->header->audit_file_count > 0)
    {
	    pod_file->audit_trail = (pod_audit_entry_pod3_t*)((pod_byte_t*)pod_file->path_data + pod_file->header->size_index);
	    pod_file->audit_data_size = pod_file->header->audit_file_count * POD_AUDIT_ENTRY_POD3_SIZE;
    }

    if(!pod_file_pod3_update_sizes(pod_file))
    {
	    fprintf(stderr, "ERROR: Could not update POD3 file entry sizes\n");
	    return pod_file_pod3_delete(pod_file);
    }
    return pod_file;
}

bool pod_file_pod3_add_entry(pod_file_pod3_t* pod_file, pod_entry_pod3_t* entry, pod_string_t filename, pod_byte_t* data)
{
	if (pod_file == NULL || entry == NULL || data == NULL)
	{
		fprintf(stderr, "ERROR: pod_file, entry or data equals NULL!\n");
		return false;
	}

	size_t new_entry_size = POD_DIR_ENTRY_POD3_SIZE;
	size_t new_entry_data_size = entry->size;
	size_t new_path_data_size = strlen(filename) + 1;
	size_t new_total_size = pod_file->size + new_entry_data_size;

	void* new_data = realloc(pod_file->data, new_total_size);
	if (!new_data) {
		return false;
	}
	pod_file->data = new_data;

	pod_file->header = (pod_header_pod3_t*)pod_file->data;
	pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_POD3_SIZE);

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

	//pod_file->checksum = pod_crc_pod3(pod_file);

	return true;
}

bool pod_file_pod3_del_entry(pod_file_pod3_t* pod_file, pod_number_t entry_index)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_del_entry(pod_file == NULL)\n");
		return false;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_del_entry(entry_index >= pod_file->header->file_count)\n");
		return false;
	}

	pod_entry_pod3_t* entry = &pod_file->entries[entry_index];
	pod_file->entry_data_size -= entry->size;
	pod_file->entry_data = realloc(pod_file->entry_data, pod_file->entry_data_size);

	if (entry_index < pod_file->header->file_count - 1)
	{
		memmove(pod_file->entries + entry_index, pod_file->entries + entry_index + 1, (pod_file->header->file_count - entry_index - 1) * POD_DIR_ENTRY_POD3_SIZE);
	}

	pod_file->header->file_count--;
	pod_file->entries = realloc(pod_file->entries, pod_file->header->file_count * POD_DIR_ENTRY_POD3_SIZE);

	//pod_file->checksum = pod_crc_pod3(pod_file);

	return true;
}

pod_entry_pod3_t* pod_file_pod3_get_entry(pod_file_pod3_t* pod_file, pod_number_t entry_index)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_get_entry(pod_file == NULL)\n");
		return NULL;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_get_entry(entry_index >= pod_file->header->file_count)\n");
		return NULL;
	}

	return &pod_file->entries[entry_index];
}

bool pod_file_pod3_extract_entry(pod_file_pod3_t* pod_file, pod_number_t entry_index, pod_string_t dst)
{
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_extract_entry(pod_file == NULL)\n");
		return false;
	}

	if (entry_index >= pod_file->header->file_count)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_extract_entry(entry_index >= pod_file->header->file_count)\n");
		return false;
	}

	FILE* file = fopen(dst, "wb");

	if (file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_extract_entry(fopen(%s, \"wb\") failed: %s\n", dst, strerror(errno));
		return false;
	}

	pod_entry_pod3_t* entry = &pod_file->entries[entry_index];
	if (fwrite(pod_file->data + entry->offset, entry->size, 1, file) != 1)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_extract_entry(fwrite failed!\n");
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

pod_file_pod3_t* pod_file_pod3_delete(pod_file_pod3_t* podfile)
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
			podfile->data = NULL;
		}
		free(podfile);
		podfile = NULL;
	}
	else
		fprintf(stderr, "ERROR: pod_file_pod3_delete(podfile) podfile already equals NULL!\n");

	return podfile;
}

bool pod_audit_entry_pod3_print(pod_audit_entry_pod3_t* audit)
{
	if(audit == NULL)
	{
		fprintf(stderr, "ERROR: pod_audit_entry_pod3_print(audit == NULL)!\n");
		return false;
	}
	printf("\n%s %s\n%s %s\n%u / %u\n%s / %s\n",
		audit->user,
		pod_ctime(&audit->timestamp),
		pod3_audit_action_string[audit->action],
		audit->path,
		audit->old_size,
		audit->new_size,
		pod_ctime(&audit->old_timestamp),
		pod_ctime(&audit->new_timestamp));

	return true;
}

bool pod_file_pod3_print(pod_file_pod3_t* pod_file, pod_char_t* pattern)
{
	if(pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_print() podfile == NULL\n");
		return false;
	}

	/* print entries */
	printf("\nEntries:\n");
	for(pod_number_t i = 0; i < pod_file->header->file_count; i++)
	{
		pod_entry_pod3_t* entry = &pod_file->entries[i];
		pod_char_t* name = pod_file->path_data + entry->path_offset;
		if (pattern && strstr(name, pattern) == NULL)
			continue;
		printf("%.10u %.10u %.8X/%.8X %10u %.32s %.32s %10u\n",
		       	i,
			entry->offset,
			entry->checksum,
			pod_crc_pod3_entry(pod_file, i),
			entry->size,
			pod_ctime(&entry->timestamp),
			name,
			entry->path_offset);
	}
/*
	if(pod_file->audit_trail != NULL || pod_file->audit_data_size > 0)
	{
		printf("\nAudit:\n");
		for(int i = 0; i < pod_file->header->audit_file_count; i++)
		{
			if(!pod_audit_entry_pod3_print(&pod_file->audit_trail[i]))
			{
				fprintf(stderr, "ERROR: pod_audit_entry_pod3_print() failed!");
				pod_file_pod3_destroy(pod_file);
				return false;
			}
		}
	}
*/
	/* print file summary */
	printf("\nSummary:\n \
	        file checksum      : 0x%.8X/% 11d\n \
	        size               : 0x%.8zX/% 11zd\n \
		filename           : %s\n \
		format             : %s\n \
		comment            : %s\n \
		data checksum      : 0x%.8X/ 0x%.8X\n \
		file entries       : 0x%.8X/% 11d\n \
		audit entries      : 0x%.8X/% 11d\n \
		revision           : 0x%.8X/% 11d\n \
		priority           : 0x%.8X/% 11d\n \
		author             : %s\n \
		copyright          : %s\n \
		index_offset       : 0x%.8X/% 11d\n \
		pad_10C            : 0x%.8X/% 11d\n \
		size_index         : 0x%.8X/% 11d\n \
		flag0              : 0x%.8X/% 11d\n \
		flag1              : 0x%.8X/% 11d\n \
		pad_11c            : 0x%.8X/% 11d\n \
		pad_120            : 0x%.8X/% 11d\n \
		pad_124            : 0x%.8X/% 11d\n \
		data_offset        : 0x%.8X/% 11d\n",
		pod_file->checksum, pod_file->checksum,
		pod_file->size, pod_file->size,
		pod_file->filename,
		pod_type_desc_str(pod_type(pod_file->header->ident)),
		pod_file->header->comment,
		pod_file->header->checksum,
		pod_crc_pod3(pod_file),
		pod_file->header->file_count,pod_file->header->file_count,
		pod_file->header->audit_file_count,pod_file->header->audit_file_count,
		pod_file->header->revision,pod_file->header->revision,
		pod_file->header->priority,pod_file->header->priority,
		pod_file->header->author,
		pod_file->header->copyright,
		pod_file->header->index_offset,pod_file->header->index_offset,
		pod_file->header->pad10c,
		pod_file->header->pad10c,
		pod_file->header->size_index,pod_file->header->size_index,
		pod_file->header->flag0,pod_file->header->flag0,
		pod_file->header->flag1,pod_file->header->flag1,
		pod_file->header->pad11c,pod_file->header->pad11c,
		pod_file->header->pad120,pod_file->header->pad120,
		pod_file->header->pad124,pod_file->header->pad124,
		pod_file->data_offset,pod_file->data_offset);
	
	return true;
}

/* Extract POD3 file pod_file to directory dst                       */
/* @returns true on success otherwise false and leaves errno         */
bool pod_file_pod3_extract(pod_file_pod3_t* pod_file, pod_string_t pattern, pod_string_t dst)
{
	pod_bool_t absolute = false;
	if (pod_file == NULL)
	{
		fprintf(stderr, "ERROR: pod_file_pod3_extract(pod_file == NULL)\n");
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
		path = pod_path_append(root, dst);
		if (!path)
		{
			fprintf(stderr, "ERROR: pod_path_append(%s,%s)\n", root, dst);
			free(root);
			return false;
		}
	}
	else
	{
		path = pod_path_append(cwd, dst);
		if (!path)
		{
			fprintf(stderr, "ERROR: pod_path_append(%s,%s)\n", cwd, dst);
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
		pod_string_t filename = strdup(pod_file->path_data + pod_file->entries[i].path_offset);
		if (!filename)
		{
			fprintf(stderr, "ERROR: pod_file_pod3_extract(filename == NULL)\n");
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
		if (strlen(basename) > 0 && mkdir_p(pod_path_append(path, basename), ACCESSPERMS) != 0)
		{
			fprintf(stderr, "ERROR: mkdir_p(%s) failed: %s\n", basename, strerror(errno));
			return false;
		}
		FILE* file = fopen(filename, "wb");
		if (file == NULL)
		{
			fprintf(stderr, "ERROR: pod_fopen_mkdir(%s) failed: %s\n", filename, strerror(errno));
			return false;
		}
		if (fwrite(pod_file->data + pod_file->entries[i].offset, pod_file->entries[i].size, 1, file) != 1)
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

bool pod_file_pod3_write(pod_file_pod3_t* pod_file, pod_string_t filename)
{
	if (pod_file == NULL || filename == NULL)
	{
		fprintf(stderr, "ERROR: pod_file or filename equals NULL!\n");
		return false;
	}

	// apply all path_data to the end of the pod_file->data.
	// start at the end of the file and work backwards

	pod_file->path_data_size = 0;
	for (int i = pod_file->header->file_count - 1; i >= 0; i--)
	{
		pod_entry_pod3_t* entry = &pod_file->entries[i];

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

	size_t new_total_size = pod_file->size + pod_file->path_data_size + pod_file->header->file_count * POD_DIR_ENTRY_POD3_SIZE;
	void* new_data = realloc(pod_file->data, new_total_size);
	if (!new_data) {
		fprintf(stderr, "ERROR: extending data entries!\n");
		return false;
	}
	pod_file->data = new_data;

	pod_file->header = (pod_header_pod3_t*)pod_file->data;

	// apply all entries to the end of the pod_file->data.

	pod_file->header->index_offset = pod_file->size;

	memcpy(pod_file->data + pod_file->size, pod_file->entries, pod_file->header->file_count * POD_DIR_ENTRY_POD3_SIZE);
	pod_file->size += pod_file->header->file_count * POD_DIR_ENTRY_POD3_SIZE;

	pod_file->entries = (pod_entry_pod3_t*)(pod_file->data + pod_file->header->index_offset);

	pod_file->entry_data = (pod_byte_t*)(pod_file->data + POD_HEADER_POD3_SIZE);

	size_t path_data_size2 = 0;

	for (int i = pod_file->header->file_count - 1; i >= 0; i--)
	{
		pod_entry_pod3_t* entry = &pod_file->entries[i];

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
	pod_file->header->size_index = pod_file->path_data_size;

	FILE* file = fopen(filename, "wb");

	if (fwrite(pod_file->data, pod_file->size, 1, file) != 1)
	{
		fprintf(stderr, "ERROR: writing POD3 data!\n");
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}