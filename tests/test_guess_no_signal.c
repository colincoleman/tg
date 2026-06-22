/*
    tg - regression test for issue #15

    In guess mode (bph == 0) a noisy or absent signal must not produce a
    confident period.  Before the fix, a single-cycle correlation peak was
    accepted (sigma == 0), yielding a bogus period that drove the oversized
    VLAs in compute_amplitude()/compute_parameters() and overflowed the
    computing thread's stack.

    This test runs the real detection (analyze_processing_data -> process ->
    compute_period) on deterministic noise, for every analysis window, and
    requires it to be rejected.  It exercises the actual algorithm rather than
    a copy of it.
*/

#include "tg.h"

/* algo.c references fill_buffers() only from its calibration path, which this
 * test never calls.  Provide a stub so algo.c can be linked on its own. */
void fill_buffers(struct processing_buffers *ps) { (void)ps; }

#ifdef DEBUG
/* debug() expands to print_debug() in DEBUG builds; provide a stub. */
void print_debug(char *format, ...) { (void)format; }
#endif

int main(void)
{
	const int sr = PA_SAMPLE_RATE;
	struct processing_buffers buffers[NSTEPS];
	struct processing_data pd = {
		.buffers = buffers,
		.last_tic = 0,
		.last_step = 0,
		.is_light = 0,
	};
	int i, step;

	for(i = 0; i < NSTEPS; i++) {
		buffers[i].sample_rate = sr;
		buffers[i].sample_count = buffers[i].interval_count = sr * (1 << (i + FIRST_STEP));
		setup_buffers(&buffers[i]);
	}

	/* Deterministic white-ish noise (fixed-seed LCG), reset per window. */
	for(i = 0; i < NSTEPS; i++) {
		unsigned seed = 0x1234567u;
		for(int j = 0; j < buffers[i].sample_count; j++) {
			seed = seed * 1103515245u + 12345u;
			buffers[i].samples[j] = (float)((seed >> 8) & 0xffff) / 32768.0f - 1.0f;
		}
	}

	int failures = 0;
	for(step = 0; step < NSTEPS; step++) {
		bool ready = analyze_processing_data(&pd, step, /*bph=*/0, DEFAULT_LA, 0);
		if(ready) {
			fprintf(stderr, "FAIL: window %d locked onto noise in guess mode "
				"(period=%f sigma=%f)\n",
				step, buffers[step].period, buffers[step].sigma);
			failures++;
		}
	}

	for(i = 0; i < NSTEPS; i++)
		pb_destroy(&buffers[i]);

	if(failures) {
		fprintf(stderr, "%d/%d windows wrongly accepted noise\n", failures, NSTEPS);
		return 1;
	}
	printf("ok: noise rejected in guess mode for all %d windows\n", NSTEPS);
	return 0;
}
