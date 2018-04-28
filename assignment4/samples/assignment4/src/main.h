#ifndef MAIN_HH
#define MAIN_HH

#define tsc_read() (_tsc_read())

void radar_read(void *hc_device, void *b, void *c);
int cmd_enable_distance_sensors(int argc, char *argv[]);
int cmd_start_recording(int argc, char *argv[]);
int cmd_dump_distances(int argc, char *argv[]);
void init_shell();

typedef struct thread_index_card {
	k_tid_t hcsr_thread_0_tid;
	k_tid_t hcsr_thread_1_tid;
	struct k_thread hcsr_thread_0;
	struct k_thread hcsr_thread_1;
} thread_index_card;

static const struct shell_cmd commands[] = {
		{"Enable", cmd_enable_distance_sensors},
		{"Start",  cmd_start_recording},
		{"Dump",   cmd_dump_distances},
		{NULL, NULL}
};

#endif