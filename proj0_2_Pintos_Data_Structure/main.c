#include "list.h"
#include "hash.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum DS { LIST, HASH, BITMAP };

// The number of list, hash table and bitmap is less than 10
struct list all_list[10];
struct hash all_hash[10];
struct bitmap *all_bitmap[10];

int all_list_idx = 0;
int all_hash_idx = 0;
int all_bitmap_idx = 0;

char all_list_name[10][31];
char all_hash_name[10][31];
char all_bitmap_name[10][31];

enum DS findDS(char cmnd[][31], int *idx);

void create_list(char cmnd[][31]);
void create_hash(char cmnd[][31]);
void create_bitmap(char cmnd[][31]);

void delete_list(int *idx);

void dumpdata_list(int *idx);
void dumpdata_hash(int *idx);
void dumpdata_bitmap(int *idx);

void fn_list(char cmnd[][31], int *idx, int cmnd_num);
void fn_hash(char cmnd[][31], int *idx);
void fn_bitmap(char cmnd[][31], int *idx);

int main(void) {
	enum DS now_ds; // 0: list, 1: hash, 2:bitmap
	char cmnd_line[183];
	char cmnd[6][31];
	int cmnd_num, idx, i, j; // idx: ds_index, i,j: elem_index
	int quit_flag = 0;

	while (quit_flag != 1) {
		// initialize
		now_ds = -1;
		idx = -1;
		quit_flag = 0;
		
		fgets(cmnd_line, sizeof(cmnd_line), stdin);
		cmnd_num = sscanf(cmnd_line, "%s %s %s %s %s %s", cmnd[0], cmnd[1], cmnd[2], cmnd[3], cmnd[4], cmnd[5]);

		if (strcmp(cmnd[0], "create") == 0) {
			if (strcmp(cmnd[1], "list") == 0) {
				create_list(cmnd);
			}
			else if (strcmp(cmnd[1], "hashtable") == 0) {
				create_hash(cmnd);
			}
			else if (strcmp(cmnd[1], "bitmap") == 0) {
				create_bitmap(cmnd);
			}
		}
		else if (strcmp(cmnd[0], "quit") == 0) {
			quit_flag = 1;
		}
		else {
			now_ds = findDS(cmnd, &idx);
			if (strcmp(cmnd[0], "delete") == 0) {
				switch (now_ds) {
				case LIST:			
					delete_list(&idx);
					break;
				case HASH:
					hash_destroy(&all_hash[idx], hash_free);
					break;
				case BITMAP:
					bitmap_destroy(all_bitmap[idx]);
					break;
				default:
					printf("Error!\n");
				}
			}
			else if (strcmp(cmnd[0], "dumpdata") == 0) {
				switch (now_ds) {
				case LIST:		
					dumpdata_list(&idx);
					break;
				case HASH:
					dumpdata_hash(&idx);
					break;
				case BITMAP:
					dumpdata_bitmap(&idx);
					break;
				default:
					printf("Error!\n");
				}
			}
			else {
				switch (now_ds) {
				case LIST:
					fn_list(cmnd, &idx, cmnd_num);
					break;
				case HASH:
					fn_hash(cmnd, &idx);
					break;
				case BITMAP:
					fn_bitmap(cmnd, &idx);
					break;
				default:
					printf("Error!\n");
				}
			}
		}
	}

  return 0;
}

enum DS findDS(char cmnd[][31], int *idx) {
	enum DS ds;
	for (int i = 0; i < 10; i++) {
		if (strcmp(cmnd[1], all_list_name[i]) == 0) {
			ds = LIST;
			*idx = i;
			break;
		}
		else if (strcmp(cmnd[1], all_hash_name[i]) == 0) {
			ds = HASH;
			*idx = i;
			break;
		}
		else if (strcmp(cmnd[1], all_bitmap_name[i]) == 0) {
			ds = BITMAP;
			*idx = i;
			break;
		}
	}

	return ds;
}

void create_list(char cmnd[][31]) {
	list_init(&all_list[all_list_idx]);
	strcpy(all_list_name[all_list_idx], cmnd[2]);
	all_list_idx++;
}

void create_hash(char cmnd[][31]) {
	hash_init(&all_hash[all_hash_idx], hash_hash, hash_less, NULL);
	strcpy(all_hash_name[all_hash_idx], cmnd[2]);
	all_hash_idx++;
}

void create_bitmap(char cmnd[][31]) {
	all_bitmap[all_bitmap_idx] = bitmap_create(atoi(cmnd[3]));
	strcpy(all_bitmap_name[all_bitmap_idx], cmnd[2]);
	all_bitmap_idx++;
}

void delete_list(int *idx) {
	struct list_elem *cur;

	while (!list_empty(&all_list[*idx])) {
		cur = list_pop_front(&all_list[*idx]);
		free(cur);
	}
}

void dumpdata_list(int *idx) {
	struct list_elem *cur;
	LI *tmp;

	for (cur = list_begin(&all_list[*idx]); cur != list_end(&all_list[*idx]);
		cur = list_next(cur)) {
		tmp = list_entry(cur, LI, elem);
		printf("%d ", tmp->data);
	}
	printf("\n");
}

void dumpdata_hash(int *idx) {
	struct hash_iterator hash_i;

	hash_first(&hash_i, &all_hash[*idx]);
	while (hash_next(&hash_i)) {
		HI *h = hash_entry(hash_cur(&hash_i), HI, elem);
		printf("%d ", h->data);
	}
	printf("\n");
}

void dumpdata_bitmap(int *idx) {
	for (int i = 0; i < bitmap_size(all_bitmap[*idx]); i++) {
		if (bitmap_test(all_bitmap[*idx], i))
			printf("1");
		else
			printf("0");
	}
	printf("\n");
}

void fn_list(char cmnd[][31], int *idx, int cmnd_num) {
	int i, idx2;
	LI *new_li, *tmp;
	struct list_elem *cur, *before, *first, *last;
	
	if (strcmp(cmnd[0], "list_push_back") == 0) {
		new_li = (LI *)malloc(sizeof(LI));
		new_li->data = atoi(cmnd[2]);
		list_push_back(&all_list[*idx], &new_li->elem);
	}
	else if (strcmp(cmnd[0], "list_insert") == 0) {
		new_li = (LI *)malloc(sizeof(LI));
		new_li->data = atoi(cmnd[3]);
		cur = find_list(&all_list[*idx], atoi(cmnd[2]));
		list_insert(cur, &new_li->elem);
	}
	else if (strcmp(cmnd[0], "list_front") == 0) {
		tmp = list_entry(list_front(&all_list[*idx]), LI, elem);
		printf("%d\n", tmp->data);
	}
	else if (strcmp(cmnd[0], "list_back") == 0) {
		tmp = list_entry(list_back(&all_list[*idx]), LI, elem);
		printf("%d\n", tmp->data);
	}
	else if (strcmp(cmnd[0], "list_pop_back") == 0) {
		list_pop_back(&all_list[*idx]);
	}
	else if (strcmp(cmnd[0], "list_pop_front") == 0) {
		list_pop_front(&all_list[*idx]);
	}
	else if (strcmp(cmnd[0], "list_insert_ordered") == 0) {
		new_li = (LI *)malloc(sizeof(LI));
		new_li->data = atoi(cmnd[2]);
		list_sort(&all_list[*idx], list_less, NULL);
		list_insert_ordered(&all_list[*idx], &new_li->elem, list_less, NULL);
	}
	else if (strcmp(cmnd[0], "list_empty") == 0) {
		if (list_empty(&all_list[*idx]))
			printf("true\n");
		else
			printf("false\n");
	}
	else if (strcmp(cmnd[0], "list_size") == 0) {
		printf("%zu\n", list_size(&all_list[*idx]));
	}
	else if (strcmp(cmnd[0], "list_max") == 0) {
		tmp = list_entry(list_max(&all_list[*idx], list_less, NULL), LI, elem);
		printf("%d\n", tmp->data);
	}
	else if (strcmp(cmnd[0], "list_min") == 0) {
		tmp = list_entry(list_min(&all_list[*idx], list_less, NULL), LI, elem);
		printf("%d\n", tmp->data);
	}
	else if (strcmp(cmnd[0], "list_push_front") == 0) {
		new_li = (LI *)malloc(sizeof(LI));
		new_li->data = atoi(cmnd[2]);
		list_push_front(&all_list[*idx], &new_li->elem);
	}
	else if (strcmp(cmnd[0], "list_remove") == 0) {
		cur = find_list(&all_list[*idx], atoi(cmnd[2]));
		list_remove(cur);
	}
	else if (strcmp(cmnd[0], "list_reverse") == 0) {
		list_reverse(&all_list[*idx]);
	}
	else if (strcmp(cmnd[0], "list_sort") == 0) {
		list_sort(&all_list[*idx], list_less, NULL);
	}
	else if (strcmp(cmnd[0], "list_splice") == 0) {
		for (i = 0; i < 10; i++) {
			if (strcmp(cmnd[3], all_list_name[i]) == 0) {
				idx2 = i;
				break;
			}
		}
		before = find_list(&all_list[*idx], atoi(cmnd[2]));
		first = find_list(&all_list[idx2], atoi(cmnd[4]));
		last = find_list(&all_list[idx2], atoi(cmnd[5]));
		list_splice(before, first, last);
	}
	else if (strcmp(cmnd[0], "list_swap") == 0) {
		before = find_list(&all_list[*idx], atoi(cmnd[2]));
		last = find_list(&all_list[*idx], atoi(cmnd[3]));
		list_swap(before, last);
	}
	else if (strcmp(cmnd[0], "list_unique") == 0) {
		if (cmnd_num == 3) {
			for (i = 0; i < 10; i++) {
				if (strcmp(cmnd[2], all_list_name[i]) == 0) {
					idx2 = i;
					break;
				}
			}
			list_unique(&all_list[*idx], &all_list[idx2], list_less, NULL);
		}
		else if (cmnd_num == 2) {
			list_unique(&all_list[*idx], NULL, list_less, NULL);
		}
	}
}

void fn_hash(char cmnd[][31], int *idx) {
	HI *new_hi, *tmp;
	struct hash_elem *cur;

	if (strcmp(cmnd[0], "hash_insert") == 0) {
		new_hi = (HI *)malloc(sizeof(HI));
		new_hi->data = atoi(cmnd[2]);
		hash_insert(&all_hash[*idx], &new_hi->elem);
	}
	else if (strcmp(cmnd[0], "hash_apply") == 0) {
		if (strcmp(cmnd[2], "square") == 0) {
			hash_apply(&all_hash[*idx], hash_square);
		}
		else if (strcmp(cmnd[2], "triple") == 0) {
			hash_apply(&all_hash[*idx], hash_triple);
		}
	}
	else if (strcmp(cmnd[0], "hash_delete") == 0) {
		tmp = (HI *)malloc(sizeof(HI));
		tmp->data = atoi(cmnd[2]);
		hash_delete(&all_hash[*idx], &tmp->elem);
	}
	else if (strcmp(cmnd[0], "hash_empty") == 0) {
		if (hash_empty(&all_hash[*idx]) == true)
			printf("true\n");
		else
			printf("false\n");
	}
	else if (strcmp(cmnd[0], "hash_size") == 0) {
		printf("%zu\n", hash_size(&all_hash[*idx]));
	}
	else if (strcmp(cmnd[0], "hash_clear") == 0) {
		hash_clear(&all_hash[*idx], hash_free);
	}
	else if (strcmp(cmnd[0], "hash_find") == 0) {
		tmp = (HI *)malloc(sizeof(HI));
		tmp->data = atoi(cmnd[2]);
		cur = hash_find(&all_hash[*idx], &tmp->elem);
		if (cur != NULL) {
			printf("%d\n", tmp->data);
		}
	}
	else if (strcmp(cmnd[0], "hash_replace") == 0) {
		new_hi = (HI *)malloc(sizeof(HI));
		new_hi->data = atoi(cmnd[2]);
		hash_replace(&all_hash[*idx], &new_hi->elem);
	}
}

void fn_bitmap(char cmnd[][31], int *idx) {
	bool tf;

	if (strcmp(cmnd[0], "bitmap_mark") == 0) {
		bitmap_mark(all_bitmap[*idx], atoi(cmnd[2]));
	}
	else if (strcmp(cmnd[0], "bitmap_none") == 0) {
		if (bitmap_none(all_bitmap[*idx], atoi(cmnd[2]), atoi(cmnd[3])))
			printf("true\n");
		else
			printf("false\n");
	}
	else if (strcmp(cmnd[0], "bitmap_all") == 0) {
		if (bitmap_all(all_bitmap[*idx], atoi(cmnd[2]), atoi(cmnd[3])))
			printf("true\n");
		else
			printf("false\n");
	}
	else if (strcmp(cmnd[0], "bitmap_any") == 0) {
		if (bitmap_any(all_bitmap[*idx], atoi(cmnd[2]), atoi(cmnd[3])))
			printf("true\n");
		else
			printf("false\n");
	}
	else if (strcmp(cmnd[0], "bitmap_contains") == 0) {
		tf = (strcmp(cmnd[4], "true") == 0) ? true : false;
		if (bitmap_contains(all_bitmap[*idx], atoi(cmnd[2]), atoi(cmnd[3]), tf))
			printf("true\n");
		else
			printf("false\n");
	}
	else if (strcmp(cmnd[0], "bitmap_count") == 0) {
		tf = (strcmp(cmnd[4], "true") == 0) ? true : false;
		printf("%zu\n", bitmap_count(all_bitmap[*idx], atoi(cmnd[2]), atoi(cmnd[3]), tf));
	}
	else if (strcmp(cmnd[0], "bitmap_dump") == 0) {
		bitmap_dump(all_bitmap[*idx]);
	}
	else if (strcmp(cmnd[0], "bitmap_expand") == 0) {
		all_bitmap[*idx] = bitmap_expand(all_bitmap[*idx], atoi(cmnd[2]));
	}
	else if (strcmp(cmnd[0], "bitmap_flip") == 0) {
		bitmap_flip(all_bitmap[*idx], atoi(cmnd[2]));
	}
	else if (strcmp(cmnd[0], "bitmap_reset") == 0) {
		bitmap_reset(all_bitmap[*idx], atoi(cmnd[2]));
	}
	else if (strcmp(cmnd[0], "bitmap_scan") == 0) {
		// use %u instead of %zu
		tf = (strcmp(cmnd[4], "true") == 0) ? true : false;
		printf("%u\n", bitmap_scan(all_bitmap[*idx], atoi(cmnd[2]), atoi(cmnd[3]), tf));
	}
	else if (strcmp(cmnd[0], "bitmap_scan_and_flip") == 0) {
		// use %u insted of %zu
		tf = (strcmp(cmnd[4], "true") == 0) ? true : false;
		printf("%u\n", bitmap_scan_and_flip(all_bitmap[*idx], atoi(cmnd[2]), atoi(cmnd[3]), tf));
	}
	else if (strcmp(cmnd[0], "bitmap_set") == 0) {
		tf = (strcmp(cmnd[3], "true") == 0) ? true : false;
		bitmap_set(all_bitmap[*idx], atoi(cmnd[2]), tf);
	}
	else if (strcmp(cmnd[0], "bitmap_set_all") == 0) {
		tf = (strcmp(cmnd[2], "true") == 0) ? true : false;
		bitmap_set_all(all_bitmap[*idx], tf);
	}
	else if (strcmp(cmnd[0], "bitmap_set_multiple") == 0) {
		tf = (strcmp(cmnd[4], "true") == 0) ? true : false;
		bitmap_set_multiple(all_bitmap[*idx], atoi(cmnd[2]), atoi(cmnd[3]), tf);
	}
	else if (strcmp(cmnd[0], "bitmap_size") == 0) {
		printf("%zu\n", bitmap_size(all_bitmap[*idx]));
	}
	else if (strcmp(cmnd[0], "bitmap_test") == 0) {
		if (bitmap_test(all_bitmap[*idx], atoi(cmnd[2])))
			printf("true\n");
		else
			printf("false\n");
	}
}