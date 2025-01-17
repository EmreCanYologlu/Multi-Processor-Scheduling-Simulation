#define MAX_NUMBER_OF_THREADS 64
#define MAX_FILE_NAME_LENGTH 128
struct Process {
   int pid;
   int cpu_id;
   int burstlen;
   int arv;
   int finish;
   int turnaround;
   int wait;
   struct Process* next;
};
struct LL {
   int load;
   struct Process* head;
   struct Process* tail;
};