#ifndef MAIN_HH
#define MAIN_HH

#define tsc_read() (_tsc_read())

void radar_read(void *hc_device, void *b, void *c);
int save_distance(uint32_t timestamp, uint32_t distance);
void write_distance_eeprom(void *device, void *buffer, void *page);
int cmd_enable_distance_sensors(int argc, char *argv[]);
int cmd_start_recording(int argc, char *argv[]);
int cmd_dump_distances(int argc, char *argv[]);
int cmd_test(int argc, char *argv[]);
void init_shell();

typedef struct thread_index_card {
	k_tid_t hcsr_thread_0_tid;
	k_tid_t hcsr_thread_1_tid;
	k_tid_t eeprom_writer_thread_tid;
	struct k_thread hcsr_thread_0;
	struct k_thread hcsr_thread_1;
	struct k_thread eeprom_writer_thread;
} thread_index_card;

typedef struct eeprom_entry {
	u32_t timestamp;
	u32_t distance;
} eeprom_entry;

typedef struct eeprom_buffers {
	eeprom_entry *in_buffer;
	eeprom_entry *out_buffer;
	size_t num_entries;
	size_t num_pages;
	size_t max_pages;
	bool is_writing;
} eeprom_buffers;

static const struct shell_cmd commands[] = {
		{"Enable", cmd_enable_distance_sensors},
		{"Start",  cmd_start_recording},
		{"Dump",   cmd_dump_distances},
		{"Test",   cmd_test},
		{NULL, NULL}
};

#endif