/* positional_test.h */

#ifndef POSITIONAL_TEST_H
#define POSITIONAL_TEST_H

#include <gtk/gtk.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define POS_COUNT 6
#define POS_MIN_EVENTS 10
#define POS_DEFAULT_DURATION  60   /* seconds */
#define POS_DEFAULT_SETTLING  15   /* seconds */
#define POS_EVENTS_INITIAL 4096
#define POS_AMPS_INITIAL 256

struct main_window;

/** Standard horological positions */
enum position_id {
    POS_DIAL_UP = 0,
    POS_DIAL_DOWN,
    POS_CROWN_UP,
    POS_CROWN_DOWN,
    POS_CROWN_LEFT,
    POS_CROWN_RIGHT
};

/** Test state machine states */
enum pos_test_state {
    POS_STATE_IDLE,
    POS_STATE_ACTIVE,
    POS_STATE_TRANSITION,
    POS_STATE_COMPLETE
};

/** Per-position collected data */
struct position_data {
    uint64_t *events;           /* Tick event timestamps */
    unsigned char *events_tictoc; /* Tic/toc classification */
    int event_count;            /* Number of stored events */
    int event_capacity;         /* Allocated capacity */

    float *amps;                /* Amplitude samples */
    uint64_t *amps_time;        /* Amplitude timestamps */
    int amp_count;
    int amp_capacity;

    /* Sampled live readings (rate/BE/amp from snapshot, stored each update) */
    double *rate_samples;       /* rate readings (s/d) sampled ~10Hz */
    double *be_samples;         /* beat error readings (ms) */
    double *amp_samples;        /* amplitude readings (deg) */
    int64_t *sample_times;      /* monotonic time of each sample */
    int sample_count;
    int sample_capacity;
#define POS_SAMPLES_INITIAL 1024

    int64_t phase_start_time;   /* g_get_monotonic_time at phase start */
    int64_t settling_end_time;  /* phase_start_time + settling_us */
    double elapsed_measurement; /* Seconds of measurement time accumulated */

    /* Results (computed at phase end) */
    double rate;                /* s/d */
    double beat_error;          /* ms */
    double amplitude;           /* degrees */
    bool valid;                 /* true if >= POS_MIN_EVENTS in window */
};

/** Signal loss tracking */
struct signal_loss_state {
    bool signal_lost;           /* Currently in signal loss */
    int64_t loss_start_time;    /* When signal dropped to 0 */
    bool timer_paused;          /* Timer is paused (>10s loss) */
    int64_t pause_start_time;   /* When timer was paused */
    double total_pause_secs;    /* Accumulated pause time in current phase */
    bool dialog_shown;          /* 60s dialog already shown */
    int zero_signal_count;      /* Consecutive update calls with signal==0 */
};

/** Main positional test context */
struct positional_test {
    enum pos_test_state state;
    int current_position;       /* Index 0-5 into position_id */

    /* Configuration (set at start) */
    int position_duration;      /* Seconds per position */
    int settling_time;          /* Seconds of settling */
    int bph;                    /* Frozen at test start */
    double la;                  /* Frozen at test start */
    int cal;                    /* Frozen at test start */
    int nominal_sr;             /* Sample rate */
    char *watch_name;           /* User-entered watch name (may be NULL) */

    /* Wall-clock time when state became COMPLETE; 0 if not yet complete */
    time_t completion_time;

    /* Per-position data */
    struct position_data positions[POS_COUNT];

    /* Signal loss tracking */
    struct signal_loss_state sig_loss;

    /* Last-seen timestamps for deduplication */
    uint64_t last_event_seen;
    uint64_t last_amp_seen;

    /* UI widgets */
    GtkWidget *notebook_tab;    /* The swim-lane tab container */
    GtkWidget *drawing_area;    /* Cairo swim-lane canvas */
    GtkWidget *status_label;    /* Position name + countdown */
    GtkWidget *continue_button; /* Shown during TRANSITION */
    GtkWidget *save_button;     /* Shown during COMPLETE */
    GtkWidget *results_label;   /* Summary text */

    /* Back-reference */
    struct main_window *main_win;
};

/* Lifecycle */
struct positional_test *pos_test_create(struct main_window *w,
                                        int duration, int settling);
void pos_test_destroy(struct positional_test *pt);
void pos_test_create_tab(struct positional_test *pt);

/* State transitions */
void pos_test_start(struct positional_test *pt);
void pos_test_continue(struct positional_test *pt);
void pos_test_cancel(struct positional_test *pt);
void pos_test_skip_position(struct positional_test *pt);

/* Called from main timer callback */
void pos_test_update(struct positional_test *pt,
                     const uint64_t *events, int events_wp, int events_count,
                     const float *amps, const uint64_t *amps_time,
                     int amps_wp, int amps_count,
                     int signal, int sample_rate);

/* Report generation */
char *pos_test_generate_report(const struct positional_test *pt);

/* Results computation (exposed for testing) */
void pos_test_compute_position_results(struct position_data *pd,
                                        int bph, double la, int cal,
                                        int nominal_sr, int settling_time);
double pos_test_compute_delta(const struct positional_test *pt);
double pos_test_compute_average_rate(const struct positional_test *pt);

/* Position name lookup */
const char *pos_test_position_name(enum position_id pos);
const char *pos_test_position_abbrev(enum position_id pos);

#endif /* POSITIONAL_TEST_H */
