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
 * then performs an offline, greedy search for a filter chain that maximises the
 * detector's lock quality.  Because every candidate is scored against the same
 * captured recording, the search is fast and the comparison is fair.
 *
 * To avoid over-fitting to the horological position the watch happened to be in
 * during capture, the search is regularised towards wide passbands: it builds
 * gentle Butterworth high-pass + low-pass brackets (not a tight band-pass),
 * enforces a minimum passband width, and only narrows the band when the
 * improvement clearly exceeds a margin.  */

#include "tg.h"

/* Length of the captured recording.  Long enough for the detector to see
 * several beats so the period jitter (sigma) is meaningful. */
#define AUTOTUNE_SECONDS 8

/* Regularisation: the low-pass cutoff must be at least this many times the
 * high-pass cutoff, keeping the passband wide enough that a position-induced
 * spectral shift stays inside it. */
#define WIDTH_RATIO 2.5

/* Conservative high-pass ceiling.  A noisy room and an HF-rich tick can make an
 * aggressive (high) cutoff score the best raw SNR, but that throws away the body
 * of the tick and over-fits to one watch/position.  Capping the high-pass keeps
 * the full tick and stays robust across positions; we pick the best SNR at or
 * below this cutoff rather than the global optimum. */
#define HP_MAX 4000

/* A later stage is only accepted if it improves the score by at least this
 * factor over the chain so far.  Kept small: the wide-band regularisation
 * (WIDTH_RATIO) already guards against over-fitting, so a low-pass that helps
 * SNR at all is worth adding. */
#define LP_MARGIN 1.01

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
	struct autotune_result *r = &d->result;

	const int sr = get_audio_sample_rate();
	const unsigned len = (unsigned)AUTOTUNE_SECONDS * sr;
	const double nyquist_margin = 0.45 * sr;

	float *raw = NULL;
	if(sr <= 0 || capture_raw_audio(len, &raw) || !raw) {
		g_idle_add(autotune_idle, d);
		free(a);
		return NULL;
	}

	struct processing_buffers b;
	memset(&b, 0, sizeof(b));
	b.sample_rate = sr;
	b.sample_count = b.interval_count = len;
	setup_buffers(&b);

	/* Baseline: how well does the chain already in use perform? */
	struct biquad_filter cur[AUTOTUNE_MAX_FILTERS];
	int ncur = 0;
	const unsigned cc = filter_chain_count(a->chain);
	unsigned i;
	for(i = 0; i < cc && ncur < AUTOTUNE_MAX_FILTERS; i++)
		cur[ncur++] = *filter_chain_get(a->chain, i);
	r->baseline = score_filter_chain(&b, raw, cur, ncur, a->bph, a->la);
	debug("auto-tune: captured %u samples @ %d Hz; baseline score %.4g\n",
		len, sr, r->baseline);

	struct biquad_filter work[AUTOTUNE_MAX_FILTERS];

	/* Stage 1: high-pass cutoff (conservatively capped at HP_MAX). */
	double best = -1;
	unsigned best_hp = 0;
	for(i = 0; i < ARRAY_SIZE(hp_grid); i++) {
		if(hp_grid[i] > HP_MAX) break;
		if(hp_grid[i] >= nyquist_margin) break;
		mkfilt(&work[0], HIGHPASS, hp_grid[i], M_SQRT1_2);
		double s = score_filter_chain(&b, raw, work, 1, a->bph, a->la);
		debug("auto-tune:   high-pass %5u Hz -> %.4g\n", hp_grid[i], s);
		if(s > best) {
			best = s;
			best_hp = hp_grid[i];
		}
	}

	if(best <= 0) {
		/* Nothing locked: no watch ticking, or signal too weak. */
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
	double cur_score = best;
	debug("auto-tune: best high-pass %u Hz (score %.4g)\n", best_hp, best);

	/* Stage 2: low-pass cutoff, bracketing the tick from above.  Constrained
	 * to keep the passband wide (WIDTH_RATIO) and only kept if it clears the
	 * acceptance margin. */
	double best_lp_score = -1;
	unsigned best_lp = 0;
	for(i = 0; i < ARRAY_SIZE(lp_grid); i++) {
		if(lp_grid[i] >= nyquist_margin) break;
		if(lp_grid[i] < WIDTH_RATIO * best_hp) continue;
		mkfilt(&work[1], LOWPASS, lp_grid[i], M_SQRT1_2);
		double s = score_filter_chain(&b, raw, work, 2, a->bph, a->la);
		debug("auto-tune:   low-pass %5u Hz -> %.4g\n", lp_grid[i], s);
		if(s > best_lp_score) {
			best_lp_score = s;
			best_lp = lp_grid[i];
		}
	}
	if(best_lp && best_lp_score > cur_score * LP_MARGIN) {
		mkfilt(&work[1], LOWPASS, best_lp, M_SQRT1_2);
		nwork = 2;
		cur_score = best_lp_score;
		debug("auto-tune: added low-pass %u Hz (score %.4g)\n", best_lp, best_lp_score);
	} else {
		debug("auto-tune: no low-pass added (best candidate %u Hz score %.4g, need > %.4g)\n",
			best_lp, best_lp_score, cur_score * LP_MARGIN);
	}

	for(i = 0; i < (unsigned)nwork; i++)
		r->filters[i] = work[i];
	r->nfilters = nwork;
	r->score = cur_score;
	/* Apply whenever the recommendation differs from what is already set.  We
	 * deliberately do not gate on raw SNR: a previously-applied aggressive
	 * filter may score higher, but the conservative choice is what we want. */
	r->improved = !chains_equal(work, nwork, cur, ncur);
	debug("auto-tune: recommended chain %s current (%s)\n",
		r->improved ? "differs from" : "matches",
		r->improved ? "applying" : "no change");

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
