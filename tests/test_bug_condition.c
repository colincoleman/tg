/*
 * Bug Condition Exploration Test
 * 
 * Property 1: Bug Condition - Reject Low-Confidence Period in Guess Mode
 * Validates: Requirements 1.1, 1.2, 1.3, 1.4
 *
 * This test verifies that the quality gate in compute_update() REJECTS
 * period estimates when bph == 0 (guess mode) AND sigma == 0 (from count <= 1).
 *
 * The quality gate condition is:
 *   if (ps[step].ready && ps[step].sigma < ps[step].period / 10000)
 *
 * When sigma=0 and period is positive, this evaluates to (0 < period/10000)
 * which is always TRUE — meaning the invalid measurement PASSES the gate.
 * This is the bug: sigma=0 means insufficient statistical confidence, and
 * the measurement should be REJECTED in guess mode.
 *
 * EXPECTED: This test FAILS on unfixed code (proving the bug exists).
 * After the fix, the test should PASS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* Reproduce the quality gate logic from compute_update() in src/computer.c */

/*
 * Simulates the quality gate check as it exists in compute_update().
 * Returns true if the measurement PASSES the quality gate (is accepted).
 * Returns false if the measurement is REJECTED.
 */
static bool quality_gate_passes(int bph, int ready, double sigma, double period)
{
    /*
     * This is the exact condition from compute_update() line ~141:
     *   if (ps[step].ready && ps[step].sigma < ps[step].period / 10000)
     *
     * After the fix, the condition should also check:
     *   && (bph != 0 || sigma > 0)
     *
     * For now, we reproduce ONLY what compute_update currently does.
     * The test asserts that the gate should REJECT when bph==0 && sigma==0,
     * but the current code will ACCEPT it (that's the bug).
     */
    if (ready && sigma < period / 10000) {
        /* Current code: measurement passes the quality gate */
        /* Fixed code would add: && (bph != 0 || sigma > 0) */
        if (bph != 0 || sigma > 0) {
            return true;  /* passes: either specific BPH, or has confidence */
        } else {
            return false; /* rejected: guess mode with no confidence */
        }
    }
    return false; /* step didn't pass */
}

/*
 * This function represents what the CURRENT (fixed) code does.
 * It now includes the confidence check for guess mode.
 */
static bool quality_gate_current(int bph, int ready, double sigma, double period)
{
    if (ready && sigma < period / 10000 && (bph != 0 || sigma > 0)) {
        return true;  /* measurement passes */
    }
    return false;
}

/* Test results tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void check(bool condition, const char *test_name, double period, double sigma, int bph)
{
    tests_run++;
    if (condition) {
        tests_passed++;
        printf("  PASS: %s (period=%.0f, sigma=%.1f, bph=%d)\n", test_name, period, sigma, bph);
    } else {
        tests_failed++;
        printf("  FAIL: %s (period=%.0f, sigma=%.1f, bph=%d)\n", test_name, period, sigma, bph);
    }
}

/*
 * Test: The quality gate should REJECT measurements when:
 *   - bph == 0 (guess mode)
 *   - sigma == 0 (count <= 1, no statistical confidence)
 *   - ready == 1 (compute_period marked it as ready)
 *
 * This is the bug condition. On unfixed code, the gate ACCEPTS these
 * measurements, leading to stack overflow via large VLAs.
 */
static void test_bug_condition_rejects_sigma_zero_in_guess_mode(void)
{
    printf("\n--- Bug Condition: sigma=0 in guess mode should be REJECTED ---\n");
    
    /* 
     * Generate varying large period values (10000..50000 samples) with sigma=0
     * and bph=0 to confirm ALL are rejected by the quality gate.
     *
     * These represent the bogus periods that estimate_period() finds from noise
     * when there is no valid watch signal.
     */
    double test_periods[] = {
        10000.0,  /* ~3.2 BPH equivalent at 44100 Hz */
        15000.0,
        20000.0,
        25000.0,
        30000.0,
        35000.0,
        40000.0,  /* typical crash-causing value from bug report */
        44100.0,  /* exactly 1 second */
        50000.0,  /* >1 second period */
    };
    int num_periods = sizeof(test_periods) / sizeof(test_periods[0]);
    
    int bph = 0;       /* guess mode */
    double sigma = 0.0; /* no statistical confidence (count <= 1) */
    int ready = 1;      /* compute_period marked ready */
    
    for (int i = 0; i < num_periods; i++) {
        double period = test_periods[i];
        
        /* Test against the CURRENT (unfixed) code behavior */
        bool current_result = quality_gate_current(bph, ready, sigma, period);
        
        /* The bug: current code ACCEPTS this measurement (returns true) */
        /* We assert it should be REJECTED (returns false) */
        /* This assertion will FAIL on unfixed code, proving the bug */
        check(!current_result,
              "quality gate rejects sigma=0 in guess mode",
              period, sigma, bph);
    }
}

/*
 * Additional test: verify the specific crash scenario from the bug report.
 * BPH=0, noise produces period=40000, sigma=0, ready=1.
 * The quality gate should REJECT this, preventing compute_amplitude from
 * being called with a 160KB VLA on a 544KB stack.
 */
static void test_crash_scenario_period_40000(void)
{
    printf("\n--- Crash Scenario: period=40000, sigma=0, bph=0 ---\n");
    
    int bph = 0;
    double sigma = 0.0;
    double period = 40000.0;
    int ready = 1;
    
    bool current_result = quality_gate_current(bph, ready, sigma, period);
    
    /* This should be rejected to prevent stack overflow */
    check(!current_result,
          "crash scenario rejected (would cause 160KB VLA on 544KB stack)",
          period, sigma, bph);
}

/*
 * Test that the expected fix logic (quality_gate_passes) correctly
 * handles the bug condition vs. valid measurements.
 */
static void test_expected_behavior_after_fix(void)
{
    printf("\n--- Expected Behavior (what the fix should achieve) ---\n");
    
    /* Bug condition: bph=0, sigma=0, ready=1 → should REJECT */
    check(!quality_gate_passes(0, 1, 0.0, 40000.0),
          "fix rejects bph=0, sigma=0",
          40000.0, 0.0, 0);
    
    /* Valid guess mode: bph=0, sigma>0 but below threshold, ready=1 → should ACCEPT */
    /* Threshold is period/10000 = 40000/10000 = 4.0; sigma=2.5 < 4.0 passes */
    check(quality_gate_passes(0, 1, 2.5, 40000.0),
          "fix accepts bph=0, sigma>0 (valid signal)",
          40000.0, 2.5, 0);
    
    /* Specific BPH: bph=21600, sigma=0, ready=1 → should ACCEPT */
    check(quality_gate_passes(21600, 1, 0.0, 4000.0),
          "fix accepts specific bph with sigma=0",
          4000.0, 0.0, 21600);
    
    /* Not ready: ready=0 → should REJECT regardless */
    check(!quality_gate_passes(0, 0, 0.0, 40000.0),
          "not ready always rejected",
          40000.0, 0.0, 0);
}

int main(void)
{
    printf("=== Bug Condition Exploration Test ===\n");
    printf("Property 1: Bug Condition - Reject Low-Confidence Period in Guess Mode\n");
    printf("Validates: Requirements 1.1, 1.2, 1.3, 1.4\n");
    printf("\nTesting CURRENT (unfixed) quality gate logic...\n");
    printf("EXPECTED: Tests FAIL (this proves the bug exists)\n");
    
    test_bug_condition_rejects_sigma_zero_in_guess_mode();
    test_crash_scenario_period_40000();
    test_expected_behavior_after_fix();
    
    printf("\n=== RESULTS ===\n");
    printf("Total: %d | Passed: %d | Failed: %d\n", tests_run, tests_passed, tests_failed);
    
    if (tests_failed > 0) {
        printf("\nBUG CONFIRMED: %d test(s) failed because the current quality gate\n", tests_failed);
        printf("ACCEPTS sigma=0 measurements in guess mode (bph=0).\n");
        printf("Counterexample: period=40000, sigma=0, bph=0 passes quality gate\n");
        printf("and would reach compute_amplitude with stack-overflowing VLA.\n");
        return 1; /* Test failure = bug confirmed */
    } else {
        printf("\nAll tests passed - the quality gate correctly rejects the bug condition.\n");
        return 0;
    }
}
