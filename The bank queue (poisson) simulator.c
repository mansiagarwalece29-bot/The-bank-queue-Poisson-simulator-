/* bank_queue_simulator.c
   Clash of Coders - Bank Queue (Poisson) Simulator
   - Simulates an 8-hour (480 minutes) bank day
   - Uses Poisson arrivals (lambda input)
   - Uses a linked-list queue (dynamic allocation)
   - Supports multiple tellers and random service times (2-3 minutes)
   - Records wait times and computes mean, median, mode, std dev, max
   Compile: gcc bank_queue_simulator.c -o bank_queue_simulator -lm
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SIMULATION_TIME 480  /* minutes in 8 hours */
#define SERVICE_MIN 2        /* minimum service time (minutes) */
#define SERVICE_MAX 3        /* maximum service time (minutes) */

/* ---------- Customer Node (Linked List) ---------- */
typedef struct Customer {
    int arrival_time;
    int service_start_time;
    struct Customer* next;
} Customer;

/* ---------- Queue ---------- */
typedef struct {
    Customer* front;
    Customer* rear;
    int size;
} Queue;

void init_queue(Queue* q) {
    q->front = q->rear = NULL;
    q->size = 0;
}

void enqueue(Queue* q, int arrival_time) {
    Customer* node = (Customer*)malloc(sizeof(Customer));
    if (!node) {
        fprintf(stderr, "Memory allocation failed in enqueue.\n");
        exit(EXIT_FAILURE);
    }
    node->arrival_time = arrival_time;
    node->service_start_time = -1;
    node->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->size++;
}

Customer* dequeue(Queue* q) {
    if (q->front == NULL) return NULL;
    Customer* tmp = q->front;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    q->size--;
    tmp->next = NULL;
    return tmp;
}

void clear_queue(Queue* q) {
    Customer* cur = q->front;
    while (cur) {
        Customer* nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    q->front = q->rear = NULL;
    q->size = 0;
}

/* ---------- Poisson random generator (Knuth) ---------- */
int poisson(double lambda) {
    double L = exp(-lambda);
    double p = 1.0;
    int k = 0;
    while (1) {
        k++;
        p *= (double)rand() / RAND_MAX;
        if (p <= L) break;
    }
    return k - 1;
}

/* ---------- Statistics helpers ---------- */
double mean(const double arr[], int n) {
    if (n == 0) return 0.0;
    double s = 0.0;
    for (int i = 0; i < n; ++i) s += arr[i];
    return s / n;
}

double stddev(const double arr[], int n, double mu) {
    if (n == 0) return 0.0;
    double s = 0.0;
    for (int i = 0; i < n; ++i) s += (arr[i] - mu) * (arr[i] - mu);
    return sqrt(s / n);
}

/* In-place simple sort (qsort would be fine too) */
int cmp_double(const void* a, const void* b) {
    double da = *(double*)a;
    double db = *(double*)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

double median(double arr[], int n) {
    if (n == 0) return 0.0;
    qsort(arr, n, sizeof(double), cmp_double);
    if (n % 2 == 0) return (arr[n/2 - 1] + arr[n/2]) / 2.0;
    else return arr[n/2];
}

/* Mode by integer rounding (returns integer mode) */
int mode_int(const double arr[], int n) {
    if (n == 0) return 0;
    /* find min & max for frequency array */
    int minv = (int)floor(arr[0]), maxv = (int)ceil(arr[0]);
    for (int i = 1; i < n; ++i) {
        int v = (int)round(arr[i]);
        if (v < minv) minv = v;
        if (v > maxv) maxv = v;
    }
    int range = maxv - minv + 1;
    int *freq = (int*)calloc(range, sizeof(int));
    if (!freq) return 0;
    for (int i = 0; i < n; ++i) {
        int v = (int)round(arr[i]) - minv;
        if (v >= 0 && v < range) freq[v]++;
    }
    int maxf = 0, mode_v = minv;
    for (int i = 0; i < range; ++i) {
        if (freq[i] > maxf) {
            maxf = freq[i];
            mode_v = i + minv;
        }
    }
    free(freq);
    return mode_v;
}

/* ---------- Main Simulation ---------- */
int main() {
    srand((unsigned)time(NULL));

    double lambda;
    int teller_count;
    printf("Bank Queue Simulator (8 hours = %d minutes)\n", SIMULATION_TIME);
    printf("Enter average arrivals per minute (lambda, e.g. 0.5): ");
    if (scanf("%lf", &lambda) != 1) return 0;
    printf("Enter number of tellers (e.g. 1): ");
    if (scanf("%d", &teller_count) != 1 || teller_count < 1) teller_count = 1;

    Queue queue;
    init_queue(&queue);

    /* For each teller, track busy flag, remaining service time, and current customer pointer */
    int *teller_busy = (int*)calloc(teller_count, sizeof(int));
    int *teller_timer = (int*)calloc(teller_count, sizeof(int));
    Customer **teller_customer = (Customer**)calloc(teller_count, sizeof(Customer*));
    if (!teller_busy || !teller_timer || !teller_customer) {
        fprintf(stderr, "Memory allocation failed for tellers.\n");
        return EXIT_FAILURE;
    }

    /* dynamic array for wait times */
    double *wait_times = NULL;
    int wait_capacity = 0;
    int wait_count = 0;

    int total_arrived = 0;
    int total_served = 0;
    double max_wait = 0.0;

    for (int minute = 0; minute < SIMULATION_TIME; ++minute) {
        /* 1) arrivals this minute */
        int arrivals = poisson(lambda);
        for (int i = 0; i < arrivals; ++i) {
            enqueue(&queue, minute);
            total_arrived++;
        }

        /* 2) advance each teller */
        for (int t = 0; t < teller_count; ++t) {
            if (teller_busy[t]) {
                teller_timer[t]--;
                if (teller_timer[t] <= 0) {
                    /* service finished for teller t */
                    Customer *c = teller_customer[t];
                    if (c) {
                        double wait = (double)(c->service_start_time - c->arrival_time);
                        /* store wait in dynamic array */
                        if (wait_count >= wait_capacity) {
                            wait_capacity = (wait_capacity == 0) ? 64 : wait_capacity * 2;
                            wait_times = (double*)realloc(wait_times, wait_capacity * sizeof(double));
                            if (!wait_times) {
                                fprintf(stderr, "Memory allocation failed for wait_times.\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                        wait_times[wait_count++] = wait;
                        if (wait > max_wait) max_wait = wait;
                        total_served++;
                        free(c);
                        teller_customer[t] = NULL;
                    }
                    teller_busy[t] = 0;
                }
            }
        }

        /* 3) assign available tellers from queue */
        for (int t = 0; t < teller_count; ++t) {
            if (!teller_busy[t] && queue.front != NULL) {
                Customer *c = dequeue(&queue);
                c->service_start_time = minute;
                /* random service time between SERVICE_MIN and SERVICE_MAX (inclusive) */
                teller_timer[t] = SERVICE_MIN + (rand() % (SERVICE_MAX - SERVICE_MIN + 1));
                teller_busy[t] = 1;
                teller_customer[t] = c;
            }
        }
    }

    /* After closing time: finish serving remaining customers in tellers (no new arrivals) */
    /* Continue advancing time until all busy tellers finish. No new arrivals will be created. */
    while (1) {
        int any_busy = 0;
        for (int t = 0; t < teller_count; ++t) {
            if (teller_busy[t]) {
                teller_timer[t]--;
                any_busy = 1;
                if (teller_timer[t] <= 0) {
                    Customer *c = teller_customer[t];
                    if (c) {
                        double wait = (double)(c->service_start_time - c->arrival_time);
                        if (wait_count >= wait_capacity) {
                            wait_capacity = (wait_capacity == 0) ? 64 : wait_capacity * 2;
                            wait_times = (double*)realloc(wait_times, wait_capacity * sizeof(double));
                            if (!wait_times) {
                                fprintf(stderr, "Memory allocation failed for wait_times.\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                        wait_times[wait_count++] = wait;
                        if (wait > max_wait) max_wait = wait;
                        total_served++;
                        free(c);
                        teller_customer[t] = NULL;
                    }
                    teller_busy[t] = 0;
                }
            }
        }
        /* if no teller busy, but queue still has customers, assign them */
        for (int t = 0; t < teller_count; ++t) {
            if (!teller_busy[t] && queue.front != NULL) {
                Customer *c = dequeue(&queue);
                c->service_start_time = SIMULATION_TIME; /* start after close for bookkeeping */
                teller_timer[t] = SERVICE_MIN + (rand() % (SERVICE_MAX - SERVICE_MIN + 1));
                teller_busy[t] = 1;
                teller_customer[t] = c;
                any_busy = 1;
            }
        }
        if (!any_busy && queue.front == NULL) break;
    }

    /* All done: compute statistics */
    if (wait_count == 0) {
        printf("No customers were served during the simulation.\n");
    } else {
        double mu = mean(wait_times, wait_count);
        /* median sorts the array in place, make a copy if you need original order later. It's okay here. */
        double *copy_for_median = (double*)malloc(wait_count * sizeof(double));
        if (!copy_for_median) copy_for_median = wait_times; /* fallback */
        else for (int i = 0; i < wait_count; ++i) copy_for_median[i] = wait_times[i];

        double med = median(copy_for_median, wait_count);
        if (copy_for_median != wait_times) free(copy_for_median);
        double sd = stddev(wait_times, wait_count, mu);
        int mo = mode_int(wait_times, wait_count);

        printf("\n===== BANK QUEUE SIMULATION REPORT =====\n");
        printf("Simulation length           : %d minutes (8 hours)\n", SIMULATION_TIME);
        printf("Lambda (arrivals / minute) : %.3f\n", lambda);
        printf("Tellers                    : %d\n", teller_count);
        printf("Total customers arrived    : %d\n", total_arrived);
        printf("Total customers served     : %d\n", total_served);
        printf("Recorded wait samples      : %d\n", wait_count);
        printf("-----------------------------------------\n");
        printf("Mean wait time             : %.2f minutes\n", mu);
        printf("Median wait time           : %.2f minutes\n", med);
        printf("Mode wait time (rounded)   : %d minutes\n", mo);
        printf("Std. Deviation of waits    : %.2f minutes\n", sd);
        printf("Longest wait time          : %.2f minutes\n", max_wait);
        printf("=========================================\n");
    }

    /* cleanup */
    clear_queue(&queue);
    free(teller_busy);
    free(teller_timer);
    free(teller_customer);
    free(wait_times);

    return 0;
}
