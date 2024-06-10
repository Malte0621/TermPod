#include "libtermpod.h"

int pod_file_typeid(pod_file_type_t file)
{
	return ((file.pod1 != NULL) ? pod_type((char*)file.pod1->header) : POD_IDENT_TYPE_SIZE);
}

pod_bool_t pod_file_is_pod(pod_path_t name)
{
	return pod_type_peek(name) < POD_IDENT_TYPE_SIZE;
}

pod_file_type_t pod_file_create(pod_path_t name, pod_ident_type_t type)
{
	pod_file_type_t pod = { NULL };
	if(name == NULL)
	{
		fprintf(stderr, "ERROR: path == NULL!");
		return pod;
	}

	if (type == -1)
		type = pod_type_peek(name);

	switch(type)
	{
		case POD1:
			pod.pod1 = pod_file_pod1_create(name);
			if(pod.pod1 == NULL)
				fprintf(stderr, "ERROR: cannot create pod1 file!\n");
			break;
		case POD2:
			pod.pod2 = pod_file_pod2_create(name);
			if(pod.pod2 == NULL)
				fprintf(stderr, "ERROR: cannot create pod2 file!\n");
			break;
		case POD3:
			pod.pod3 = pod_file_pod3_create(name);
			if(pod.pod3 == NULL)
				fprintf(stderr, "ERROR: cannot create pod3 file!\n");
			break;
		case POD4:
			pod.pod4 = pod_file_pod4_create(name);
			if(pod.pod4 == NULL)
				fprintf(stderr, "ERROR: cannot create pod4 file!\n");
			break;
		case POD5:
			pod.pod5 = pod_file_pod5_create(name);
			if(pod.pod5 == NULL)
				fprintf(stderr, "ERROR: cannot create pod5 file!\n");
			break;
		case POD6:
			pod.pod6 = pod_file_pod6_create(name);
			if(pod.pod6 == NULL)
				fprintf(stderr, "ERROR: cannot create pod6 file!\n");
			break;
		case EPD:
			pod.epd = pod_file_epd_create(name);
			if(pod.epd == NULL)
				fprintf(stderr, "ERROR: cannot create epd file!\n");
			break;
		default:
			fprintf(stderr, "ERROR: unknown file format!\n");
			break;
	}
	return pod;
}

pod_file_type_t pod_file_delete(pod_file_type_t file)
{
	switch(pod_file_typeid(file))
	{
		case POD1:
			file.pod1 = pod_file_pod1_delete(file.pod1);
			break;
		case POD2:
			file.pod2 = pod_file_pod2_delete(file.pod2);
			break;
		case POD3:
			file.pod3 = pod_file_pod3_delete(file.pod3);
			break;
		case POD4:
			file.pod4 = pod_file_pod4_delete(file.pod4);
			break;
		case POD5:
			file.pod5 = pod_file_pod5_delete(file.pod5);
			break;
		case POD6:
			file.pod6 = pod_file_pod6_delete(file.pod6);
			break;
		case EPD:
			file.epd = pod_file_epd_delete(file.epd);
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_delete() unknown file format!\n");
			free(file.pod1);
			file.pod1 = NULL;
			break;
	}
	return file;
}

pod_checksum_t   pod_file_chksum(pod_file_type_t file)
{
	pod_checksum_t checksum = 0xFFFFFFFF;
	switch(pod_file_typeid(file))
	{
		case POD1:
			checksum = pod_file_pod1_chksum(file.pod1);
			break;
		case POD2:
			checksum = pod_file_pod2_chksum(file.pod2);
			break;
		case POD3:
			checksum = pod_file_pod3_chksum(file.pod3);
			break;
		case POD4:
			checksum = pod_file_pod4_chksum(file.pod4);
			break;
		case POD5:
			checksum = pod_file_pod5_chksum(file.pod5);
			break;
		case POD6:
			checksum = pod_file_pod6_chksum(file.pod6);
			break;
		case EPD:
			checksum = pod_file_epd_chksum(file.epd);
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_chksum() unknown file format!\n");
			break;
	}
	return checksum;
}

void            pod_file_crc(pod_file_type_t file) {
	switch (pod_file_typeid(file))
	{
	case POD1:
		pod_crc_pod1(file.pod1);
		break;
	case POD2:
		pod_crc_pod2(file.pod2);
		break;
	case POD3:
		pod_crc_pod3(file.pod3);
		break;
	case POD4:
		pod_crc_pod4(file.pod4);
		break;
	case POD5:
		pod_crc_pod5(file.pod5);
		break;
	case POD6:
		pod_crc_pod6(file.pod6);
		break;
	case EPD:
		pod_crc_epd(file.epd);
		break;
	default:
		fprintf(stderr, "ERROR: pod_file_crc() unknown file format!\n");
		break;
	}
}

pod_ssize_t pod_file_write(pod_file_type_t file, pod_path_t dst_name)
{
	// Ensure our file has been checksummed.
	pod_file_crc(file);

	pod_ssize_t size = 0;
	switch (pod_file_typeid(file))
	{
		case POD1:
			size = pod_file_pod1_write(file.pod1, dst_name);
			break;
		case POD2:
			size = pod_file_pod2_write(file.pod2, dst_name);
			break;
		case POD3:
			size = pod_file_pod3_write(file.pod3, dst_name);
			break;
		case POD4:
			size = pod_file_pod4_write(file.pod4, dst_name);
			break;
		case POD5:
			size = pod_file_pod5_write(file.pod5, dst_name);
			break;
		case POD6:
			size = pod_file_pod6_write(file.pod6, dst_name);
			break;
		case EPD:
			size = pod_file_epd_write(file.epd, dst_name);
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_write() unknown file format!\n");
			break;
	}
	return size;
}
pod_file_type_t pod_file_reset(pod_file_type_t file) {
	return pod_file_delete(file);
}
pod_file_type_t pod_file_merge(pod_file_type_t file, pod_file_type_t src) {
	// combine them.
	pod_number_t file_count = 0;
	switch (pod_file_typeid(file))
	{
		case POD1:
			file_count = file.pod1->header->file_count;
			break;
		case POD2:
			file_count = file.pod2->header->file_count;
			break;
		case POD3:
			file_count = file.pod3->header->file_count;
			break;
		case POD4:
			file_count = file.pod4->header->file_count;
			break;
		case POD5:
			file_count = file.pod5->header->file_count;
			break;
		case POD6:
			file_count = file.pod6->header->file_count;
			break;
		case EPD:
			file_count = file.epd->header->file_count;
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_merge() unknown file format!\n");
			break;
	}

	switch (pod_file_typeid(src))
	{
		case POD1:
			for (int i = 0; i < src.pod1->header->file_count; i++)
			{
				pod_entry_pod1_t* entry = &src.pod1->entries[i];
				pod_file_entry_data_add(file, entry->name, file_count++, src.pod1->entry_data + entry->offset);
			}
			break;
		case POD2:
			for (int i = 0; i < src.pod2->header->file_count; i++)
			{
				pod_entry_pod2_t* entry = &src.pod2->entries[i];
				pod_char_t* name = src.pod2->path_data + entry->path_offset;
				pod_file_entry_data_add(file, name, file_count++, src.pod2->entry_data + entry->offset);
			}
			break;
		case POD3:
			for (int i = 0; i < src.pod3->header->file_count; i++)
			{
				pod_entry_pod3_t* entry = &src.pod3->entries[i];
				pod_char_t* name = src.pod3->path_data + entry->path_offset;
				pod_file_entry_data_add(file, name, file_count++, src.pod3->entry_data + entry->offset);
			}
			break;
		case POD4:
			for (int i = 0; i < src.pod4->header->file_count; i++)
			{
				pod_entry_pod4_t* entry = &src.pod4->entries[i];
				pod_char_t* name = src.pod4->path_data + entry->path_offset;
				pod_file_entry_data_add(file, name, file_count++, src.pod4->entry_data + entry->offset);
			}
			break;
		case POD5:
			for (int i = 0; i < src.pod5->header->file_count; i++)
			{
				pod_entry_pod5_t* entry = &src.pod5->entries[i];
				pod_char_t* name = src.pod5->path_data + entry->path_offset;
				pod_file_entry_data_add(file, name, file_count++, src.pod5->entry_data + entry->offset);
			}
			break;
		case POD6:
			for (int i = 0; i < src.pod6->header->file_count; i++)
			{
				pod_entry_pod6_t* entry = &src.pod6->entries[i];
				pod_char_t* name = src.pod6->path_data + entry->path_offset;
				pod_file_entry_data_add(file, name, file_count++, src.pod6->entry_data + entry->offset);
			}
			break;
		case EPD:
			for (int i = 0; i < src.epd->header->file_count; i++)
			{
				pod_entry_epd_t* entry = &src.epd->entries[i];
				pod_file_entry_data_add(file, entry->name, file_count++, src.epd->entry_data + entry->offset);
			}
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_merge() unknown file format!\n");
			break;
	}
}

bool pod_file_print(pod_file_type_t file, pod_char_t* pattern)
{
	switch(pod_file_typeid(file))
	{
		case POD1:
			return pod_file_pod1_print(file.pod1, pattern);
		case POD2:
			return pod_file_pod2_print(file.pod2, pattern);
		case POD3:
			return pod_file_pod3_print(file.pod3, pattern);
		case POD4:
			return pod_file_pod4_print(file.pod4, pattern);
		case POD5:
			return pod_file_pod5_print(file.pod5, pattern);
		case POD6:
			return pod_file_pod6_print(file.pod6, pattern);
		case EPD:
			return pod_file_epd_print(file.epd, pattern);
		default:
			fprintf(stderr, "ERROR: pod_file_print() unknown file format!\n");
			return false;
	}
	return true;
}

pod_ssize_t pod_file_extract(pod_file_type_t file, pod_char_t* pattern, pod_path_t dst)
{
	pod_ssize_t size = 0;
	switch (pod_file_typeid(file))
	{
		case POD1:
			size = pod_file_pod1_extract(file.pod1, pattern, dst);
			break;
		case POD2:
			size = pod_file_pod2_extract(file.pod2, pattern, dst);
			break;
		case POD3:
			size = pod_file_pod3_extract(file.pod3, pattern, dst);
			break;
		case POD4:
			size = pod_file_pod4_extract(file.pod4, pattern, dst);
			break;
		case POD5:
			size = pod_file_pod5_extract(file.pod5, pattern, dst);
			while (file.pod5->header->next_archive[0] != '\0') {
				pod_file_type_t next = pod_file_create(file.pod5->header->next_archive, -1);
				fprintf(stderr, "INFO: pod_file_extract() next archive: %s\n", file.pod5->header->next_archive);
				size += pod_file_extract(next, pattern, dst);
				pod_file_delete(file);
				file = next;
			}
			break;
		case POD6:
			size = pod_file_pod6_extract(file.pod6, pattern, dst);
			while (file.pod6->header->next_archive[0] != '\0') {
				pod_file_type_t next = pod_file_create(file.pod6->header->next_archive, -1);
				fprintf(stderr, "INFO: pod_file_extract() next archive: %s\n", file.pod6->header->next_archive);
				size += pod_file_extract(next, pattern, dst);
				pod_file_delete(file);
				file = next;
			}
			break;
		case EPD:
			size = pod_file_epd_extract(file.epd, pattern, dst);
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_extract() unknown file format!\n");
			break;
	}
	return size;
}

pod_ssize_t pod_file_count(pod_file_type_t file, pod_char_t* pattern)
{
	pod_ssize_t size = 0;
	switch (pod_file_typeid(file))
	{
		case POD1:
			for (int i = 0; i < file.pod1->header->file_count; i++) {
				pod_entry_pod1_t* entry = &file.pod1->entries[i];
				if (pattern == NULL || strstr(entry->name, pattern) != NULL) {
					size++;
				}
			}
			break;
		case POD2:
			for (int i = 0; i < file.pod2->header->file_count; i++) {
				pod_entry_pod2_t* entry = &file.pod2->entries[i];
				pod_char_t* name = file.pod2->path_data + entry->path_offset;
				if (pattern == NULL || strstr(name, pattern) != NULL) {
					size++;
				}
			}
			break;
		case POD3:
			for (int i = 0; i < file.pod3->header->file_count; i++) {
				pod_entry_pod3_t* entry = &file.pod3->entries[i];
				pod_char_t* name = file.pod3->path_data + entry->path_offset;
				if (pattern == NULL || strstr(name, pattern) != NULL) {
					size++;
				}
			}
			break;
		case POD4:
			for (int i = 0; i < file.pod4->header->file_count; i++) {
				pod_entry_pod4_t* entry = &file.pod4->entries[i];
				pod_char_t* name = file.pod4->path_data + entry->path_offset;
				if (pattern == NULL || strstr(name, pattern) != NULL) {
					size++;
				}
			}
			break;
		case POD5:
			for (int i = 0; i < file.pod5->header->file_count; i++) {
				pod_entry_pod5_t* entry = &file.pod5->entries[i];
				pod_char_t* name = file.pod5->path_data + entry->path_offset;
				if (pattern == NULL || strstr(name, pattern) != NULL) {
					size++;
				}
			}
			break;
		case POD6:
			for (int i = 0; i < file.pod6->header->file_count; i++) {
				pod_entry_pod6_t* entry = &file.pod6->entries[i];
				pod_char_t* name = file.pod6->path_data + entry->path_offset;
				if (pattern == NULL || strstr(name, pattern) != NULL) {
					size++;
				}
			}
			break;
		case EPD:
			for (int i = 0; i < file.epd->header->file_count; i++) {
				pod_entry_epd_t* entry = &file.epd->entries[i];
				if (pattern == NULL || strstr(entry->name, pattern) != NULL) {
					size++;
				}
			}
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_extract() unknown file format!\n");
			break;
	}
	return size;
}

/* access entries by name and/or number */
pod_byte_t* pod_file_entry_data_get(pod_file_type_t file, pod_path_t entry_name, pod_number_t entry_number) {
	switch (pod_file_typeid(file))
	{
		case POD1:
			if (entry_number == NULL) {
				for (int i = 0; i < file.pod1->header->file_count; i++) {
					pod_entry_pod1_t* entry = &file.pod1->entries[i];
					if (strcmp(entry->name, entry_name) == 0) {
						return file.pod1->entry_data + entry->offset;
					}
				}
			}
			else {
				pod_entry_pod1_t* entry = &file.pod1->entries[entry_number];
				return file.pod1->entry_data + entry->offset;
			}
		case POD2:
			if (entry_number == NULL) {
				for (int i = 0; i < file.pod2->header->file_count; i++) {
					pod_entry_pod2_t* entry = &file.pod2->entries[i];
					pod_char_t* name = file.pod2->path_data + entry->path_offset;
					if (strcmp(name, entry_name) == 0) {
						return file.pod2->entry_data + entry->offset;
					}
				}
			}
			else {
				pod_entry_pod2_t* entry = &file.pod2->entries[entry_number];
				return file.pod2->entry_data + entry->offset;
			}
		case POD3:
			if (entry_number == NULL) {
				for (int i = 0; i < file.pod3->header->file_count; i++) {
					pod_entry_pod3_t* entry = &file.pod3->entries[i];
					pod_char_t* name = file.pod3->path_data + entry->path_offset;
					if (strcmp(name, entry_name) == 0) {
						return file.pod3->entry_data + entry->offset;
					}
				}
			}
			else {
				pod_entry_pod3_t* entry = &file.pod3->entries[entry_number];
				return file.pod3->entry_data + entry->offset;
			}
		case POD4:
			if (entry_number == NULL) {
				for (int i = 0; i < file.pod4->header->file_count; i++) {
					pod_entry_pod4_t* entry = &file.pod4->entries[i];
					pod_char_t* name = file.pod4->path_data + entry->path_offset;
					if (strcmp(name, entry_name) == 0) {
						return file.pod4->entry_data + entry->offset;
					}
				}
			}
			else {
				pod_entry_pod4_t* entry = &file.pod4->entries[entry_number];
				return file.pod4->entry_data + entry->offset;
			}
		case POD5:
			if (entry_number == NULL) {
				for (int i = 0; i < file.pod5->header->file_count; i++) {
					pod_entry_pod5_t* entry = &file.pod5->entries[i];
					pod_char_t* name = file.pod5->path_data + entry->path_offset;
					if (strcmp(name, entry_name) == 0) {
						return file.pod5->entry_data + entry->offset;
					}
				}
			}
			else {
				pod_entry_pod5_t* entry = &file.pod5->entries[entry_number];
				return file.pod5->entry_data + entry->offset;
			}
		case POD6:
			if (entry_number == NULL) {
				for (int i = 0; i < file.pod6->header->file_count; i++) {
					pod_entry_pod6_t* entry = &file.pod6->entries[i];
					pod_char_t* name = file.pod6->path_data + entry->path_offset;
					if (strcmp(name, entry_name) == 0) {
						return file.pod6->entry_data + entry->offset;
					}
				}
			}
			else {
				pod_entry_pod6_t* entry = &file.pod6->entries[entry_number];
				return file.pod6->entry_data + entry->offset;
			}
		case EPD:
			if (entry_number == NULL) {
				for (int i = 0; i < file.epd->header->file_count; i++) {
					pod_entry_epd_t* entry = &file.epd->entries[i];
					if (strcmp(entry->name, entry_name) == 0) {
						return file.epd->entry_data + entry->offset;
					}
				}
			}
			else {
				pod_entry_epd_t* entry = &file.epd->entries[entry_number];
				return file.epd->entry_data + entry->offset;
			}
		default:
			fprintf(stderr, "ERROR: pod_file_entry_data_get() unknown file format!\n");
			return NULL;
	}
	return NULL;
}
pod_file_type_t  pod_file_entry_data_add(pod_file_type_t file, void* entry, pod_string_t filename, pod_byte_t* data) {
	switch (pod_file_typeid(file))
	{
		case POD1:
			file.pod1 = pod_file_pod1_add_entry(file.pod1, entry, filename, data);
			break;
		case POD2:
			file.pod2 = pod_file_pod2_add_entry(file.pod2, entry, filename, data);
			break;
		case POD3:
			file.pod3 = pod_file_pod3_add_entry(file.pod3, entry, filename, data);
			break;
		case POD4:
			file.pod4 = pod_file_pod4_add_entry(file.pod4, entry, filename, data);
			break;
		case POD5:
			file.pod5 = pod_file_pod5_add_entry(file.pod5, entry, filename, data);
			break;
		case POD6:
			file.pod6 = pod_file_pod6_add_entry(file.pod6, entry, filename, data);
			break;
		case EPD:
			file.epd = pod_file_epd_add_entry(file.epd, entry, filename, data);
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_entry_data_add() unknown file format!\n");
			break;
	}
	return file;
}
pod_file_type_t  pod_file_entry_data_del(pod_file_type_t file, pod_number_t entry_number) {
	switch (pod_file_typeid(file))
	{
		case POD1:
			file.pod1 = pod_file_pod1_del_entry(file.pod1, entry_number);
			break;
		case POD2:
			file.pod2 = pod_file_pod2_del_entry(file.pod2, entry_number);
			break;
		case POD3:
			file.pod3 = pod_file_pod3_del_entry(file.pod3, entry_number);
			break;
		case POD4:
			file.pod4 = pod_file_pod4_del_entry(file.pod4, entry_number);
			break;
		case POD5:
			file.pod5 = pod_file_pod5_del_entry(file.pod5, entry_number);
			break;
		case POD6:
			file.pod6 = pod_file_pod6_del_entry(file.pod6, entry_number);
			break;
		case EPD:
			file.epd = pod_file_epd_del_entry(file.epd, entry_number);
			break;
		default:
			fprintf(stderr, "ERROR: pod_file_entry_data_del() unknown file format!\n");
			break;
	}
	return file;
}
pod_checksum_t   pod_file_entry_data_chk(pod_file_type_t file, pod_number_t entry_number) {
	switch (pod_file_typeid(file))
	{
		case POD1:
			return pod_crc_pod1_entry(file.pod1, entry_number);
		case POD2:
			return pod_crc_pod2_entry(file.pod2, entry_number);
		case POD3:
			return pod_crc_pod3_entry(file.pod3, entry_number);
		case POD4:
			return pod_crc_pod4_entry(file.pod4, entry_number);
		case POD5:
			return pod_crc_pod5_entry(file.pod5, entry_number);
		case POD6:
			return pod_crc_pod6_entry(file.pod6, entry_number);
		case EPD:
			return pod_crc_epd_entry(file.epd, entry_number);
		default:
			fprintf(stderr, "ERROR: pod_file_entry_data_chk() unknown file format!\n");
			break;
	}
	return 0;
}
pod_ssize_t      pod_file_entry_data_ext(pod_file_type_t file, pod_number_t entry_number, pod_path_t dst) {
	switch (pod_file_typeid(file))
	{
		case POD1:
			return pod_file_pod1_extract_entry(file.pod1, entry_number, dst);
		case POD2:
			return pod_file_pod2_extract_entry(file.pod2, entry_number, dst);
		case POD3:
			return pod_file_pod3_extract_entry(file.pod3, entry_number, dst);
		case POD4:
			return pod_file_pod4_extract_entry(file.pod4, entry_number, dst);
		case POD5:
			return pod_file_pod5_extract_entry(file.pod5, entry_number, dst);
		case POD6:
			return pod_file_pod6_extract_entry(file.pod6, entry_number, dst);
		case EPD:
			return pod_file_epd_extract_entry(file.epd, entry_number, dst);
		default:
			fprintf(stderr, "ERROR: pod_file_entry_data_ext() unknown file format!\n");
			break;
	}
	return 0;
}