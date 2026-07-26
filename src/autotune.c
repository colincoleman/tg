/*
    tg
    Copyright (C) 2015 Marcello Mamino

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* Automatic audio filter tuning.
 *
 * Captures a few seconds of the running watch with the filter chain bypassed,
 * then searches offline for a high-pass (+ optional low-pass) filter that helps
 * the detector.
 *
 * Each candidate is scored by lock *reliability*: the capture is split into
 * several overlapping sub-windows and detection is run on each with the
 * candidate applied; the score is how many of them lock the way the live
 * detector requires (a low-jitter period, sigma < period/10000), with the
 * detection SNR as a tie-break.  This matches what the live detector actually
 * accepts, so the chosen filter both locks reliably and produces clean, steady
 * readings.  A moderate passband-width floor keeps the result reasonable across
 * horological positions.  */

#include "tg.h"

/* Length of the captured recording. */
#define AUTOTUNE_SECONDS 15
/* Each scored sub-window is this long (long enough to see several beats). */
#define AUTOTUNE_SUBWINDOW 6
/* The low-pass cutoff must be at least this many times the high-pass cutoff -
 * wide enough to stay robust when the watch is moved between positions. */
#define WIDTH_RATIO 2.0

/* Candidate cutoff frequencies (Hz), log-spaced.  Out-of-range entries are
 * skipped at runtime based on the actual sample rate. */
static const unsigned hp_grid[] = {
	300, 500, 750, 1000, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 8000, 10000, 12000
};
static const unsigned lp_grid[] = {
	3000, 4000, 5000, 6000, 8000, 10000, 12000, 15000, 18000, 21000,
	24000, 32000, 48000, 64000
};

static void mkfilt(struct biquad_filter *f, enum bitype type, unsigned freq, double q)
{
	memset(f, 0, sizeof(*f));
	f->type = type;
	f->frequency = freq;
	f->bw = q;
	f->gain = 0;
	f->enabled = true;
}

/* Compare two chains by what actually affects the audio (type, frequency,
 * enabled state).  Used to decide whether the recommendation is worth applying. */
static bool chains_equal(const struct biquad_filter *a, int na,
			 const struct biquad_filter *b, int nb)
{
	int i;
	if(na != nb)
		return false;
	for(i = 0; i < na; i++) {
		if(a[i].type != b[i].type) return false;
		if(a[i].frequency != b[i].frequency) return false;
		if(!a[i].enabled != !b[i].enabled) return false;
	}
	return true;
}

/* Read the current chain into an array, for baseline scoring / comparison. */
static int current_chain(struct filter_chain *chain, struct biquad_filter *out)
{
	int n = 0;
	unsigned cc = filter_chain_count(chain), i;
	for(i = 0; i < cc && n < AUTOTUNE_MAX_FILTERS; i++)
		out[n++] = *filter_chain_get(chain, i);
	return n;
}

/* Score a candidate by how reliably it locks: run the detector on `nwin`
 * overlapping sub-windows and count how many lock the way the live detector
 * keeps a lock.  score_filter_chain() returns > 0 whenever detection is
 * "ready", but compute_update() only holds a lock when the period is low-jitter
 * (sigma < period/10000); counting only those makes the search optimise for
 * results the live detector will actually accept.  Returns the lock count;
 * *snr accumulates the SNR of the windows that locked (a tie-break). */
static int reliability(struct processing_buffers *b, const float *raw, int nwin,
		unsigned step, const struct biquad_filter *filters, int nfilters,
		int bph, double la, double *snr)
{
	int locks = 0;
	*snr = 0;
	for(int k = 0; k < nwin; k++) {
		double s = score_filter_chain(b, raw + (size_t)k * step,
				filters, nfilters, bph, la);
		if(s > 0 && b->period > 0 && b->sigma < b->period / 10000) {
			locks++;
			*snr += s;
		}
	}
	return locks;
}

/* Is (locks_a, snr_a) a better result than (locks_b, snr_b)?  More locks wins;
 * SNR breaks a tie. */
static bool more_reliable(int locks_a, double snr_a, int locks_b, double snr_b)
{
	return locks_a > locks_b || (locks_a == locks_b && snr_a > snr_b);
}

struct autotune_args {
	struct filter_chain *chain;
	int bph;
	double la;
	void (*done)(const struct autotune_result *result, void *user);
	void *user;
};

struct idle_data {
	struct autotune_result result;
	void (*done)(const struct autotune_result *result, void *user);
	void *user;
};

/* Invoked on the GTK main thread to deliver the result. */
static gboolean autotune_idle(gpointer p)
{
	struct idle_data *d = p;
	d->done(&d->result, d->user);
	free(d);
	return G_SOURCE_REMOVE;
}

static void *autotune_thread(void *vp)
{
	struct autotune_args *a = vp;

	struct idle_data *d = calloc(1, sizeof(*d));
	d->done = a->done;
	d->user = a->user;
	struct autotune_result *r = &d->result; /* zeroed: locked=false by default */

	const int sr = get_audio_sample_rate();
	const unsigned len = (unsigned)AUTOTUNE_SECONDS * sr;
	const double nyquist_margin = 0.45 * sr;

	float *raw = NULL;
	if(sr <= 0 || capture_raw_audio(len, &raw) || !raw) {
		g_idle_add(autotune_idle, d);
		free(a);
		return NULL;
	}

	const unsigned sub_len = (unsigned)AUTOTUNE_SUBWINDOW * sr;
	const unsigned step = sub_len / 2; /* 50% overlap */
	const int nwin = (sub_len <= len) ? (int)(1 + (len - sub_len) / step) : 0;
	if(nwin < 1) {
		r->locked = false;
		free(raw);
		g_idle_add(autotune_idle, d);
		free(a);
		return NULL;
	}

	struct processing_buffers b;
	memset(&b, 0, sizeof(b));
	b.sample_rate = sr;
	b.sample_count = b.interval_count = sub_len;
	setup_buffers(&b);

	/* Baseline: how reliably does the chain already in use lock? */
	struct biquad_filter cur[AUTOTUNE_MAX_FILTERS];
	int ncur = current_chain(a->chain, cur);
	double base_snr;
	int base_locks = reliability(&b, raw, nwin, step, cur, ncur, a->bph, a->la, &base_snr);
	r->baseline = base_locks;
	debug("auto-tune: baseline %d/%d locks\n", base_locks, nwin);

	struct biquad_filter work[AUTOTUNE_MAX_FILTERS];
	unsigned i;

	/* Stage 1: high-pass cutoff. */
	int best_locks = 0;
	double best_snr = 0;
	unsigned best_hp = 0;
	for(i = 0; i < ARRAY_SIZE(hp_grid); i++) {
		if(hp_grid[i] >= nyquist_margin) break;
		mkfilt(&work[0], HIGHPASS, hp_grid[i], M_SQRT1_2);
		double snr;
		int locks = reliability(&b, raw, nwin, step, work, 1, a->bph, a->la, &snr);
		debug("auto-tune:   high-pass %5u Hz -> %d/%d locks\n", hp_grid[i], locks, nwin);
		if(more_reliable(locks, snr, best_locks, best_snr)) {
			best_locks = locks; best_snr = snr; best_hp = hp_grid[i];
		}
	}

	if(best_locks == 0) {
		/* Nothing locked cleanly: no watch ticking, or signal too weak. */
		r->locked = false;
		pb_destroy(&b);
		free(raw);
		g_idle_add(autotune_idle, d);
		free(a);
		return NULL;
	}

	r->locked = true;
	mkfilt(&work[0], HIGHPASS, best_hp, M_SQRT1_2);
	int nwork = 1;
	int cur_locks = best_locks;
	double cur_snr = best_snr;
	debug("auto-tune: best high-pass %u Hz (%d/%d locks)\n", best_hp, best_locks, nwin);

	/* Stage 2: low-pass, kept wide enough (WIDTH_RATIO) to stay robust, and only
	 * added if it locks at least as reliably as the high-pass alone. */
	int lp_locks = 0;
	double lp_snr = 0;
	unsigned best_lp = 0;
	for(i = 0; i < ARRAY_SIZE(lp_grid); i++) {
		if(lp_grid[i] >= nyquist_margin) break;
		if(lp_grid[i] < WIDTH_RATIO * best_hp) continue;
		mkfilt(&work[1], LOWPASS, lp_grid[i], M_SQRT1_2);
		double snr;
		int locks = reliability(&b, raw, nwin, step, work, 2, a->bph, a->la, &snr);
		debug("auto-tune:   band %u-%u -> %d/%d locks\n", best_hp, lp_grid[i], locks, nwin);
		if(more_reliable(locks, snr, lp_locks, lp_snr)) {
			lp_locks = locks; lp_snr = snr; best_lp = lp_grid[i];
		}
	}
	if(best_lp && more_reliable(lp_locks, lp_snr, cur_locks, cur_snr)) {
		mkfilt(&work[1], LOWPASS, best_lp, M_SQRT1_2);
		nwork = 2;
		cur_locks = lp_locks; cur_snr = lp_snr;
		debug("auto-tune: added low-pass %u Hz (%d/%d locks)\n", best_lp, lp_locks, nwin);
	}

	for(i = 0; i < (unsigned)nwork; i++)
		r->filters[i] = work[i];
	r->nfilters = nwork;
	r->score = cur_locks;
	/* Apply only if it locks more reliably (or as reliably but cleaner) than the
	 * current chain, and actually differs from it. */
	r->improved = more_reliable(cur_locks, cur_snr, base_locks, base_snr)
		&& !chains_equal(work, nwork, cur, ncur);
	debug("auto-tune: recommended chain %s current\n",
		r->improved ? "differs from / better than" : "no better than");

	pb_destroy(&b);
	free(raw);
	g_idle_add(autotune_idle, d);
	free(a);
	return NULL;
}

void autotune_start(struct filter_chain *chain, int bph, double la,
		void (*done)(const struct autotune_result *result, void *user), void *user)
{
	struct autotune_args *a = malloc(sizeof(*a));
	a->chain = chain;
	a->bph = bph;
	a->la = la;
	a->done = done;
	a->user = user;

	pthread_t th;
	if(pthread_create(&th, NULL, autotune_thread, a)) {
		/* Report failure as "not locked" so the UI restores itself. */
		free(a);
		struct idle_data *d = calloc(1, sizeof(*d));
		d->done = done;
		d->user = user;
		g_idle_add(autotune_idle, d);
		return;
	}
	pthread_detach(th);
}
