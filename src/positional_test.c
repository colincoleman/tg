#include "positional_test.h"
#include "tg.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

/* Forward declarations for static functions */
static void pos_test_show_results(struct positional_test *pt);

struct positional_test *pos_test_create(struct main_window *w,
                                        int duration, int settling)
{
    struct positional_test *pt = calloc(1, sizeof(*pt));
    if (!pt)
        return NULL;

    pt->state = POS_STATE_IDLE;
    pt->current_position = 0;

    /* Configuration from parameters and main_window */
    pt->position_duration = duration;
    pt->settling_time = settling;
    pt->bph = w->bph;
    pt->la = w->la;
    pt->cal = w->cal;
    pt->nominal_sr = w->nominal_sr;

    /* Back-reference */
    pt->main_win = w;

    /* Allocate per-position data arrays */
    for (int i = 0; i < POS_COUNT; i++) {
        struct position_data *pd = &pt->positions[i];

        pd->events = malloc(POS_EVENTS_INITIAL * sizeof(uint64_t));
        pd->events_tictoc = malloc(POS_EVENTS_INITIAL * sizeof(unsigned char));
        pd->event_count = 0;
        pd->event_capacity = POS_EVENTS_INITIAL;

        pd->amps = malloc(POS_AMPS_INITIAL * sizeof(float));
        pd->amps_time = malloc(POS_AMPS_INITIAL * sizeof(uint64_t));
        pd->amp_count = 0;
        pd->amp_capacity = POS_AMPS_INITIAL;

        pd->rate_samples = malloc(POS_SAMPLES_INITIAL * sizeof(double));
        pd->be_samples = malloc(POS_SAMPLES_INITIAL * sizeof(double));
        pd->amp_samples = malloc(POS_SAMPLES_INITIAL * sizeof(double));
        pd->sample_times = malloc(POS_SAMPLES_INITIAL * sizeof(int64_t));
        pd->sample_count = 0;
        pd->sample_capacity = POS_SAMPLES_INITIAL;

        if (!pd->events || !pd->events_tictoc || !pd->amps || !pd->amps_time
            || !pd->rate_samples || !pd->be_samples || !pd->amp_samples || !pd->sample_times) {
            for (int j = 0; j <= i; j++) {
                free(pt->positions[j].events);
                free(pt->positions[j].events_tictoc);
                free(pt->positions[j].amps);
                free(pt->positions[j].amps_time);
                free(pt->positions[j].rate_samples);
                free(pt->positions[j].be_samples);
                free(pt->positions[j].amp_samples);
                free(pt->positions[j].sample_times);
            }
            free(pt);
            return NULL;
        }
    }

    return pt;
}

/* --- Swim-lane tab UI --- */

/* Left margin for position name labels (pixels) */
#define LANE_LABEL_MARGIN 140
/* Maximum number of dots to render per lane (performance limit) */
#define MAX_DOTS_PER_LANE 2000

/**
 * Full Cairo swim-lane draw callback.
 *
 * Renders 6 horizontal lanes (one per position), each showing tick event
 * dots in a paperstrip-style display. The X axis represents time progression,
 * and the Y offset within a lane represents deviation from expected tick period.
 */
static gboolean pos_test_draw_callback(GtkWidget *widget, cairo_t *cr,
                                        gpointer data)
{
    struct positional_test *pt = (struct positional_test *)data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    int width = alloc.width;
    int height = alloc.height;

    /* 1. Clear background (dark) */
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    if (!pt)
        return FALSE;

    /* 2. Calculate lane dimensions */
    double lane_height = (double)height / POS_COUNT;
    double chart_left = LANE_LABEL_MARGIN;

    /* Reserve right-side space for the text report (always present) */
    double report_width = 280;

    double chart_width = width - chart_left - report_width;

    if (chart_width < 10)
        return FALSE;

    /* Expected tick period in samples */
    double period = 0.0;
    if (pt->bph > 0 && pt->nominal_sr > 0)
        period = (double)pt->nominal_sr * 7200.0 / pt->bph;

    /* Font setup */
    cairo_select_font_face(cr, "monospace",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    int font_size = (int)(lane_height * 0.18);
    if (font_size < 10) font_size = 10;
    if (font_size > 16) font_size = 16;
    cairo_set_font_size(cr, font_size);

    /* 3. Draw each lane */
    for (int lane = 0; lane < POS_COUNT; lane++) {
        double lane_y = lane * lane_height;
        double lane_center_y = lane_y + lane_height / 2.0;
        struct position_data *pd = &pt->positions[lane];
        bool is_active = (pt->state == POS_STATE_ACTIVE &&
                          lane == pt->current_position);
        bool is_completed = (lane < pt->current_position ||
                             pt->state == POS_STATE_COMPLETE);

        /* 3a. Lane background: highlight active or completed lanes */
        if (is_active || (is_completed && pd->event_count > 0)) {
            cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);
            cairo_rectangle(cr, 0, lane_y, width, lane_height);
            cairo_fill(cr);
        }

        /* 3b. Lane separator line (subtle gray horizontal line at bottom) */
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, 0, lane_y + lane_height);
        cairo_line_to(cr, width, lane_y + lane_height);
        cairo_stroke(cr);

        /* 3c. Position name label on left margin */
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        const char *name = pos_test_position_abbrev((enum position_id)lane);
        cairo_move_to(cr, 8, lane_y + font_size + 2);
        cairo_show_text(cr, name);

        /* Show full name below abbreviation */
        cairo_set_font_size(cr, font_size - 2);
        cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
        const char *full_name = pos_test_position_name((enum position_id)lane);
        cairo_move_to(cr, 8, lane_y + 2 * font_size + 2);
        cairo_show_text(cr, full_name);

        /* Show rate/BE/amplitude for this position */
        int small_font = font_size - 3;
        if (small_font < 8) small_font = 8;
        cairo_set_font_size(cr, small_font);

        if (pd->valid) {
            /* Completed position with valid results */
            char metric_buf[64];
            snprintf(metric_buf, sizeof(metric_buf), "%+.1f s/d", pd->rate);
            cairo_set_source_rgb(cr, 0.8, 1.0, 0.8);
            cairo_move_to(cr, 8, lane_y + 3 * font_size + 2);
            cairo_show_text(cr, metric_buf);

            snprintf(metric_buf, sizeof(metric_buf), "%.1f ms", pd->beat_error);
            cairo_set_source_rgb(cr, 1.0, 1.0, 0.8);
            cairo_move_to(cr, 8, lane_y + 3 * font_size + small_font + 4);
            cairo_show_text(cr, metric_buf);

            snprintf(metric_buf, sizeof(metric_buf), "%.0f\u00b0", pd->amplitude);
            cairo_set_source_rgb(cr, 0.8, 0.8, 1.0);
            cairo_move_to(cr, 8, lane_y + 3 * font_size + 2 * small_font + 6);
            cairo_show_text(cr, metric_buf);
        } else if (is_active && pd->event_count > 0 && pt->main_win) {
            /* Active position — show live values from main snapshot */
            struct snapshot *snap = pt->main_win->active_snapshot;
            if (snap && snap->pb) {
                char metric_buf[64];
                int rate = (int)round(snap->rate);
                snprintf(metric_buf, sizeof(metric_buf), "%+d s/d", rate);
                cairo_set_source_rgb(cr, 0.8, 1.0, 0.8);
                cairo_move_to(cr, 8, lane_y + 3 * font_size + 2);
                cairo_show_text(cr, metric_buf);

                snprintf(metric_buf, sizeof(metric_buf), "%.1f ms", snap->be);
                cairo_set_source_rgb(cr, 1.0, 1.0, 0.8);
                cairo_move_to(cr, 8, lane_y + 3 * font_size + small_font + 4);
                cairo_show_text(cr, metric_buf);

                if (snap->amp > 0) {
                    snprintf(metric_buf, sizeof(metric_buf), "%.0f\u00b0", snap->amp);
                } else {
                    snprintf(metric_buf, sizeof(metric_buf), "---");
                }
                cairo_set_source_rgb(cr, 0.8, 0.8, 1.0);
                cairo_move_to(cr, 8, lane_y + 3 * font_size + 2 * small_font + 6);
                cairo_show_text(cr, metric_buf);
            }
        } else if (!pd->valid && pd->event_count > 0) {
            /* Completed but insufficient data */
            cairo_set_source_rgb(cr, 0.7, 0.4, 0.4);
            cairo_move_to(cr, 8, lane_y + 3 * font_size + 2);
            cairo_show_text(cr, "Insuff. data");
        }

        cairo_set_font_size(cr, font_size);

        /* 3d. Signal loss red overlay on active lane */
        if (is_active && pt->sig_loss.signal_lost) {
            cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.15);
            cairo_rectangle(cr, chart_left, lane_y, chart_width, lane_height);
            cairo_fill(cr);
        }

        /* Draw centre reference line (where a perfect watch trace sits) */
        cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 0.4);
        cairo_set_line_width(cr, 0.5);
        cairo_move_to(cr, chart_left, lane_center_y);
        cairo_line_to(cr, width, lane_center_y);
        cairo_stroke(cr);

        /* 3e. Render tick dots if position has events (paperstrip-style) */
        if (pd->event_count > 1 && period > 0.0) {
            /* Expected beat length in samples (full tic-toc cycle) */
            double beat_length = (double)pt->nominal_sr * 7200.0 / pt->bph;
            if (beat_length <= 0) goto skip_lane;

            uint64_t first_ts = pd->events[0];
            uint64_t last_ts = pd->events[pd->event_count - 1];
            double total_span = (double)(last_ts - first_ts);
            UNUSED(total_span);

            /* 3f. Draw measurement window shading.
             * Shade the last settling_time seconds (the averaging window)
             * on the right side of the lane to show where the final score
             * is computed from. */
            double measure_frac = (double)pt->settling_time / (double)pt->position_duration;
            if (measure_frac > 1.0) measure_frac = 1.0;
            double measure_width = measure_frac * chart_width;
            cairo_set_source_rgba(cr, 0.0, 0.3, 0.0, 0.2);
            cairo_rectangle(cr, chart_left + chart_width - measure_width, lane_y,
                            measure_width, lane_height);
            cairo_fill(cr);

            /* Compute accumulated offsets using FULL period intervals.
             * The events alternate tic/toc. To show pure rate drift without
             * beat-error wobble, compute residuals between same-type events
             * (tic-to-tic or toc-to-toc = full period intervals).
             * Each event gets the offset of its same-type predecessor. */
            double *offsets = malloc(pd->event_count * sizeof(double));
            if (!offsets) goto skip_lane;

            /* half_beat = expected half-period (tic-to-toc or toc-to-tic) */
            double half_beat = beat_length / 2.0;

            /* Accumulate using half-beat residuals but track tic and toc
             * chains separately. This way beat error doesn't affect the
             * accumulated offset — only rate drift does. */
            double tic_offset = 0.0;
            double toc_offset = 0.0;
            int last_tic_idx = -1;
            int last_toc_idx = -1;

            for (int i = 0; i < pd->event_count; i++) {
                if (pd->events_tictoc[i] == 0) {
                    /* Tic event */
                    if (last_tic_idx >= 0) {
                        double diff = (double)(pd->events[i] - pd->events[last_tic_idx]);
                        double residual = fmod(diff, beat_length);
                        if (residual > beat_length / 2.0) residual -= beat_length;
                        tic_offset += residual;
                    }
                    offsets[i] = tic_offset;
                    last_tic_idx = i;
                } else {
                    /* Toc event */
                    if (last_toc_idx >= 0) {
                        double diff = (double)(pd->events[i] - pd->events[last_toc_idx]);
                        double residual = fmod(diff, beat_length);
                        if (residual > beat_length / 2.0) residual -= beat_length;
                        toc_offset += residual;
                    } else if (last_tic_idx >= 0) {
                        /* First toc: offset relative to first tic using half-beat */
                        double diff = (double)(pd->events[i] - pd->events[last_tic_idx]);
                        double residual = fmod(diff, half_beat);
                        if (residual > half_beat / 2.0) residual -= half_beat;
                        toc_offset = tic_offset + residual;
                    }
                    offsets[i] = toc_offset;
                    last_toc_idx = i;
                }
            }

            /* Fixed Y scale: lane height represents 10ms of timing offset.
             * This gives a readable scale where wrapping occurs at ±5ms from centre.
             * Convert samples to ms: offset_in_ms = offset_in_samples / nominal_sr * 1000 */
            double ms_per_lane = 10.0; /* 10ms of range visible before wrap */
            double samples_per_lane = ms_per_lane * pt->nominal_sr / 1000.0;
            double y_scale = lane_height / samples_per_lane;

            /* X axis: fixed time window = position_duration mapped to chart_width.
             * Left edge = first event, right edge = first_ts + duration * sample_rate.
             * The trace progresses left-to-right as time passes. */
            double fixed_span = (double)pt->position_duration * pt->nominal_sr;
            double view_start_ts = (double)first_ts;
            double view_span = fixed_span;
            int start_idx = 0;

            /* X scaling: chart_width pixels = position_duration in samples */
            double x_scale = chart_width / view_span;

            /* Draw each event as a small dot */
            int dots_drawn = 0;

            for (int i = start_idx; i < pd->event_count && dots_drawn < MAX_DOTS_PER_LANE; i++) {
                double ts_d = (double)pd->events[i];

                /* Skip events outside visible window */
                if (ts_d < view_start_ts)
                    continue;
                if (view_span > 0.0 && (ts_d - view_start_ts) > view_span)
                    break;

                double x = chart_left + (ts_d - view_start_ts) * x_scale;
                /* Y position: offset from lane center, anchored at first event.
                 * First event has offset 0 = lane center.
                 * Positive offset (slow watch) moves DOWN.
                 * Negative offset (fast watch) moves UP.
                 * Wrap modulo lane height. */
                double raw_y = offsets[i] * y_scale;
                double y_range = lane_height - 4;
                /* Centre the initial position (offset=0 maps to middle of range) */
                double y_in_range = fmod(raw_y + y_range / 2.0, y_range);
                if (y_in_range < 0) y_in_range += y_range;
                double y = lane_y + 2 + y_in_range;

                /* Color: tic = white, toc = goldenrod */
                if (pd->events_tictoc[i])
                    cairo_set_source_rgb(cr, 0.85, 0.65, 0.13); /* goldenrod */
                else
                    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); /* white */

                cairo_rectangle(cr, x - 0.5, y - 0.5, 1.5, 1.5);
                cairo_fill(cr);
                dots_drawn++;
            }

            free(offsets);
        }
        skip_lane:

        /* 3d (continued): Draw signal loss indicator more prominently */
        if (is_active && pt->sig_loss.signal_lost) {
            /* Draw "NO SIGNAL" text */
            cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 0.8);
            cairo_set_font_size(cr, font_size + 4);
            cairo_move_to(cr, chart_left + chart_width / 2.0 - 40,
                          lane_center_y + (font_size + 4) / 3.0);
            cairo_show_text(cr, "NO SIGNAL");
            cairo_set_font_size(cr, font_size);
        }
    }

    /* --- Draw text report panel on the right side --- */
    {
        double rx = width - report_width + 10;
        int rfont = 11;

        cairo_select_font_face(cr, "monospace",
                               CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, rfont);

        /* Separator line (always visible) */
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, width - report_width, 0);
        cairo_line_to(cr, width - report_width, height);
        cairo_stroke(cr);

        /* Report panel — always shown with header; rows fill in as positions complete */
        {
            double ry = 20;
            char rbuf[128];

            /* Title */
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, rx, ry);
            cairo_show_text(cr, "POSITIONAL TEST REPORT");
            ry += rfont + 6;

            /* Watch name (if set) */
            if (pt->watch_name && pt->watch_name[0]) {
                cairo_set_source_rgb(cr, 1.0, 1.0, 0.7);
                cairo_move_to(cr, rx, ry);
                cairo_show_text(cr, pt->watch_name);
                ry += rfont + 4;
            }

            /* Date/time */
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char datetime[64];
            strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M", tm_info);
            cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
            cairo_move_to(cr, rx, ry);
            cairo_show_text(cr, datetime);
            ry += rfont + 6;

            /* Config */
            snprintf(rbuf, sizeof(rbuf), "BPH: %d  LA: %.0f\u00b0  Cal: %d",
                     pt->bph, pt->la, pt->cal);
            cairo_move_to(cr, rx, ry);
            cairo_show_text(cr, rbuf);
            ry += rfont + 4;

            snprintf(rbuf, sizeof(rbuf), "Duration: %ds  Avg: %ds",
                     pt->position_duration, pt->settling_time);
            cairo_move_to(cr, rx, ry);
            cairo_show_text(cr, rbuf);
            ry += rfont + 10;

            /* Column headers */
            cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
            cairo_move_to(cr, rx, ry);
            cairo_show_text(cr, "Pos   Rate   BE   Amp");
            ry += rfont + 4;

            /* Position rows — show results for completed positions, blank for others */
            for (int i = 0; i < POS_COUNT; i++) {
                const struct position_data *pd2 = &pt->positions[i];
                const char *abbr = pos_test_position_abbrev((enum position_id)i);

                if (pd2->valid) {
                    snprintf(rbuf, sizeof(rbuf), "%-4s %+5.1f %4.1f %5.0f",
                             abbr, pd2->rate, pd2->beat_error, pd2->amplitude);
                    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
                } else if (pd2->sample_count > 0 && !pd2->valid && i < pt->current_position) {
                    /* Completed but insufficient data */
                    snprintf(rbuf, sizeof(rbuf), "%-4s  ---   ---   ---", abbr);
                    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
                } else {
                    /* Not yet tested */
                    snprintf(rbuf, sizeof(rbuf), "%-4s", abbr);
                    cairo_set_source_rgb(cr, 0.35, 0.35, 0.35);
                }
                cairo_move_to(cr, rx, ry);
                cairo_show_text(cr, rbuf);
                ry += rfont + 3;
            }

            /* Summary — only show when test is complete */
            ry += 6;
            if (pt->state == POS_STATE_COMPLETE) {
                double delta = pos_test_compute_delta(pt);
                double avg = pos_test_compute_average_rate(pt);

                cairo_set_source_rgb(cr, 0.6, 1.0, 0.6);
                snprintf(rbuf, sizeof(rbuf), "Delta:   %.1f s/d", delta);
                cairo_move_to(cr, rx, ry);
                cairo_show_text(cr, rbuf);
                ry += rfont + 3;

                snprintf(rbuf, sizeof(rbuf), "Average: %+.1f s/d", avg);
                cairo_move_to(cr, rx, ry);
                cairo_show_text(cr, rbuf);
            }
        }
    }

    return FALSE;
}

/* Continue button clicked handler */
static void pos_test_continue_clicked(GtkButton *button, gpointer data)
{
    UNUSED(button);
    struct positional_test *pt = (struct positional_test *)data;
    pos_test_continue(pt);
}

/* Save Report button clicked handler */
static void pos_test_save_clicked(GtkButton *button, gpointer data)
{
    UNUSED(button);
    struct positional_test *pt = (struct positional_test *)data;

    /* 1. Generate the report text */
    char *report = pos_test_generate_report(pt);
    if (!report) {
        GtkWidget *err = gtk_message_dialog_new(
            GTK_WINDOW(gtk_widget_get_toplevel(pt->drawing_area)),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Failed to generate report.");
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
        return;
    }

    /* 2. Create a file chooser dialog for saving */
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Save Positional Test Report",
        GTK_WINDOW(gtk_widget_get_toplevel(pt->drawing_area)),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL);

    /* 3. Enable overwrite confirmation */
    gtk_file_chooser_set_do_overwrite_confirmation(
        GTK_FILE_CHOOSER(dialog), TRUE);

    /* 4. Generate default filename with current date */
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char default_name[64];
    strftime(default_name, sizeof(default_name),
             "positional_test_%Y-%m-%d.txt", tm_now);

    /* 5. Set the default filename */
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), default_name);

    /* 6. Run dialog */
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        /* Write report to the chosen file */
        GError *error = NULL;
        gboolean ok = g_file_set_contents(filename, report, -1, &error);
        if (!ok) {
            /* Show error dialog on file write failure */
            GtkWidget *err_dialog = gtk_message_dialog_new(
                GTK_WINDOW(gtk_widget_get_toplevel(pt->drawing_area)),
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Error saving report to %s:\n%s",
                filename,
                error ? error->message : "Unknown error");
            gtk_dialog_run(GTK_DIALOG(err_dialog));
            gtk_widget_destroy(err_dialog);
            if (error)
                g_error_free(error);
        }

        g_free(filename);
    }

    /* 7. Destroy the file chooser dialog */
    gtk_widget_destroy(dialog);

    /* 8. Free the report string */
    free(report);
}

/* Tab close button clicked handler */
static void pos_test_tab_close_clicked(GtkButton *button, gpointer data)
{
    UNUSED(button);
    struct positional_test *pt = (struct positional_test *)data;
    struct main_window *w = pt->main_win;

    /* If the test is still running (ACTIVE or TRANSITION), treat as cancel:
     * re-enable controls and reset the button label. */
    if (pt->state == POS_STATE_ACTIVE || pt->state == POS_STATE_TRANSITION) {
        if (w) {
            w->controls_active = 1;
            gtk_widget_set_sensitive(w->bph_combo_box, TRUE);
            gtk_widget_set_sensitive(w->la_spin_button, TRUE);
            gtk_widget_set_sensitive(w->cal_spin_button, TRUE);
            gtk_widget_set_sensitive(w->cal_button, TRUE);
            gtk_widget_show(w->snapshot_button);
            gtk_widget_hide(w->snapshot_name);
            if (w->full_test_button)
                gtk_button_set_label(GTK_BUTTON(w->full_test_button),
                                     "Full Test");
        }
    }

    pos_test_destroy(pt);

    if (w)
        w->pos_test = NULL;
}

void pos_test_create_tab(struct positional_test *pt)
{
    if (!pt || !pt->main_win)
        return;

    struct main_window *w = pt->main_win;

    /* 1. Main vertical container */
    pt->notebook_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* 2. Drawing area for swim lanes — takes all available space */
    pt->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(pt->drawing_area, 600, 400);
    gtk_box_pack_start(GTK_BOX(pt->notebook_tab), pt->drawing_area,
                       TRUE, TRUE, 0);
    g_signal_connect(pt->drawing_area, "draw",
                     G_CALLBACK(pos_test_draw_callback), pt);

    /* 3. Fixed-height bottom bar: status label, continue button, save button
     *    all packed into one horizontal row. They take turns being visible
     *    but the bar itself never changes size. */
    GtkWidget *bottom_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_size_request(bottom_bar, -1, 36);
    gtk_box_pack_start(GTK_BOX(pt->notebook_tab), bottom_bar, FALSE, FALSE, 0);

    /* Status label — shows countdown during ACTIVE, position info during TRANSITION */
    pt->status_label = gtk_label_new("Positional Test");
    gtk_widget_set_halign(pt->status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(bottom_bar), pt->status_label, TRUE, TRUE, 8);

    /* Continue button — shown during TRANSITION, hidden otherwise */
    pt->continue_button = gtk_button_new_with_label("Continue");
    gtk_box_pack_end(GTK_BOX(bottom_bar), pt->continue_button, FALSE, FALSE, 8);
    gtk_widget_set_no_show_all(pt->continue_button, TRUE);
    gtk_widget_hide(pt->continue_button);
    g_signal_connect(pt->continue_button, "clicked",
                     G_CALLBACK(pos_test_continue_clicked), pt);

    /* Save Report button — shown during COMPLETE, hidden otherwise */
    pt->save_button = gtk_button_new_with_label("Save Report");
    gtk_box_pack_end(GTK_BOX(bottom_bar), pt->save_button, FALSE, FALSE, 8);
    gtk_widget_set_no_show_all(pt->save_button, TRUE);
    gtk_widget_hide(pt->save_button);
    g_signal_connect(pt->save_button, "clicked",
                     G_CALLBACK(pos_test_save_clicked), pt);

    /* Results label — hidden, only used for markup storage (shown in COMPLETE) */
    pt->results_label = gtk_label_new("");
    gtk_widget_set_no_show_all(pt->results_label, TRUE);
    gtk_widget_hide(pt->results_label);
    /* Don't pack it — results are displayed in the swim lane left column */

    /* 4. Create tab label with "Positional Test" text and close button */
    GtkWidget *tab_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *tab_label = gtk_label_new("Positional Test");
    gtk_box_pack_start(GTK_BOX(tab_hbox), tab_label, FALSE, FALSE, 5);

    GtkWidget *close_image = gtk_image_new_from_icon_name(
        "window-close-symbolic", GTK_ICON_SIZE_MENU);
    GtkWidget *close_button = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(close_button), close_image);
    gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
    g_signal_connect(close_button, "clicked",
                     G_CALLBACK(pos_test_tab_close_clicked), pt);
    gtk_box_pack_start(GTK_BOX(tab_hbox), close_button, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_hbox);

    /* 5. Add the container as a new notebook tab */
    gtk_widget_show_all(pt->notebook_tab);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w->notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(w->notebook), TRUE);
    gtk_notebook_append_page(GTK_NOTEBOOK(w->notebook),
                             pt->notebook_tab, tab_hbox);

    /* 6. Switch to the new tab */
    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(w->notebook),
                                          pt->notebook_tab);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(w->notebook), page_num);
}

void pos_test_destroy(struct positional_test *pt)
{
    if (!pt)
        return;

    /* Free per-position data arrays */
    for (int i = 0; i < POS_COUNT; i++) {
        struct position_data *pd = &pt->positions[i];
        free(pd->events);
        free(pd->events_tictoc);
        free(pd->amps);
        free(pd->amps_time);
        free(pd->rate_samples);
        free(pd->be_samples);
        free(pd->amp_samples);
        free(pd->sample_times);
    }

    /* Destroy notebook tab widget if it exists */
    if (pt->notebook_tab) {
        gtk_widget_destroy(pt->notebook_tab);
        pt->notebook_tab = NULL;
    }

    free(pt->watch_name);
    free(pt);
}

void pos_test_start(struct positional_test *pt)
{
    if (!pt)
        return;

    pt->state = POS_STATE_ACTIVE;
    pt->current_position = 0; /* Dial Up */

    /* Record phase start time and compute settling end */
    pt->positions[0].phase_start_time = g_get_monotonic_time();
    pt->positions[0].settling_end_time =
        pt->positions[0].phase_start_time +
        (int64_t)pt->settling_time * 1000000;

    /* Initialize last_event_seen to pick up all new events */
    pt->last_event_seen = 0;

    /* Reset signal loss state */
    memset(&pt->sig_loss, 0, sizeof(pt->sig_loss));
}

void pos_test_continue(struct positional_test *pt)
{
    if (!pt || pt->state != POS_STATE_TRANSITION)
        return;

    /* Advance to the next position */
    pt->current_position++;

    /* Set up the new position's timing */
    struct position_data *pd = &pt->positions[pt->current_position];
    pd->phase_start_time = g_get_monotonic_time();
    pd->settling_end_time = pd->phase_start_time +
        (int64_t)pt->settling_time * 1000000;

    /* Reset signal loss state for the new phase */
    memset(&pt->sig_loss, 0, sizeof(pt->sig_loss));

    /* Reset last_event_seen to current snapshot so we don't pick up
     * stale events from the transition period */
    if (pt->main_win && pt->main_win->active_snapshot) {
        struct snapshot *s = pt->main_win->active_snapshot;
        if (s->events_count > 0 && s->events[s->events_wp])
            pt->last_event_seen = s->events[s->events_wp];
        if (s->amps_count > 0 && s->amps_time[s->amps_wp])
            pt->last_amp_seen = s->amps_time[s->amps_wp];
    }

    /* Transition to active state */
    pt->state = POS_STATE_ACTIVE;
}

void pos_test_skip_position(struct positional_test *pt)
{
    if (!pt)
        return;

    struct position_data *pd = &pt->positions[pt->current_position];
    pd->valid = false;

    if (pt->current_position >= POS_COUNT - 1) {
        pt->state = POS_STATE_COMPLETE;
        /* Display results summary and show save button */
        pos_test_show_results(pt);
    } else {
        pt->state = POS_STATE_TRANSITION;
    }
}

void pos_test_cancel(struct positional_test *pt)
{
    if (!pt)
        return;

    pt->state = POS_STATE_IDLE;

    /* Reset all collected position data counts */
    for (int i = 0; i < POS_COUNT; i++) {
        pt->positions[i].event_count = 0;
        pt->positions[i].amp_count = 0;
    }

    /* Remove and destroy the notebook tab widget */
    if (pt->notebook_tab) {
        gtk_widget_destroy(pt->notebook_tab);
        pt->notebook_tab = NULL;
    }

    /* Detach from main window (if pos_test field exists) */
    /* pt->main_win->pos_test = NULL; — integrated in task 9.1 */

    /* Free everything — the test is done */
    pos_test_destroy(pt);
}

void pos_test_update(struct positional_test *pt,
                     const uint64_t *events, int events_wp, int events_count,
                     const float *amps, const uint64_t *amps_time,
                     int amps_wp, int amps_count,
                     int signal, int sample_rate)
{
    if (!pt)
        return;

    /* Don't update nominal_sr — it was frozen at test start */
    UNUSED(sample_rate);

    /* In TRANSITION or COMPLETE state: just update last_event_seen and
     * last_amp_seen to the newest timestamps so we don't accumulate a
     * backlog, then return without storing anything. */
    if (pt->state != POS_STATE_ACTIVE) {
        if (events_count > 0 && events[events_wp])
            pt->last_event_seen = events[events_wp];
        if (amps_count > 0 && amps_time[amps_wp])
            pt->last_amp_seen = amps_time[amps_wp];
        return;
    }

    struct position_data *pd = &pt->positions[pt->current_position];

    /* --- Extract new tick events from circular buffer --- */
    /* The circular buffer has events_wp pointing at the most recently
     * written entry. Oldest entry is at (events_wp + 1) % events_count.
     * Iterate from oldest to newest. */
    if (events_count > 0) {
        for (int i = 0; i < events_count; i++) {
            /* Oldest entry is at (events_wp+1) % events_count,
             * iterate from oldest to newest */
            int idx = (events_wp + 1 + i) % events_count;

            uint64_t ts = events[idx];
            if (!ts)
                continue;
            if (ts <= pt->last_event_seen)
                continue;

            /* New event — append to current position */
            if (pd->event_count >= pd->event_capacity) {
                int new_cap = pd->event_capacity * 2;
                uint64_t *new_ev = realloc(pd->events,
                    new_cap * sizeof(uint64_t));
                unsigned char *new_tt = realloc(pd->events_tictoc,
                    new_cap * sizeof(unsigned char));
                if (!new_ev || !new_tt)
                    break; /* allocation failure — stop appending */
                pd->events = new_ev;
                pd->events_tictoc = new_tt;
                pd->event_capacity = new_cap;
            }
            pd->events[pd->event_count] = ts;
            /* Copy tic/toc classification from snapshot */
            if (pt->main_win && pt->main_win->active_snapshot)
                pd->events_tictoc[pd->event_count] =
                    pt->main_win->active_snapshot->events_tictoc[idx];
            else
                pd->events_tictoc[pd->event_count] = 0;
            pd->event_count++;
        }
        /* Update last_event_seen to the newest event in the buffer */
        if (events[events_wp])
            pt->last_event_seen = events[events_wp];
    }

    /* --- Extract new amplitude samples from circular buffer --- */
    if (amps_count > 0) {
        for (int i = 0; i < amps_count; i++) {
            int idx = (amps_wp + 1 + i) % amps_count;

            uint64_t ts = amps_time[idx];
            if (!ts)
                continue;
            if (ts <= pt->last_amp_seen)
                continue;

            /* New amplitude sample — append to current position */
            if (pd->amp_count >= pd->amp_capacity) {
                int new_cap = pd->amp_capacity * 2;
                float *new_a = realloc(pd->amps,
                    new_cap * sizeof(float));
                uint64_t *new_at = realloc(pd->amps_time,
                    new_cap * sizeof(uint64_t));
                if (!new_a || !new_at)
                    break;
                pd->amps = new_a;
                pd->amps_time = new_at;
                pd->amp_capacity = new_cap;
            }
            pd->amps[pd->amp_count] = amps[idx];
            pd->amps_time[pd->amp_count] = ts;
            pd->amp_count++;
        }
        /* Update last_amp_seen to the newest amp in the buffer */
        if (amps_time[amps_wp])
            pt->last_amp_seen = amps_time[amps_wp];
    }

    /* --- Store live rate/BE/amplitude sample from the main snapshot --- */
    if (pt->main_win && pt->main_win->active_snapshot) {
        struct snapshot *snap = pt->main_win->active_snapshot;
        if (snap->pb && signal > 0) {
            /* Grow sample arrays if needed */
            if (pd->sample_count >= pd->sample_capacity) {
                int new_cap = pd->sample_capacity * 2;
                double *nr = realloc(pd->rate_samples, new_cap * sizeof(double));
                double *nb = realloc(pd->be_samples, new_cap * sizeof(double));
                double *na = realloc(pd->amp_samples, new_cap * sizeof(double));
                int64_t *nt = realloc(pd->sample_times, new_cap * sizeof(int64_t));
                if (nr && nb && na && nt) {
                    pd->rate_samples = nr;
                    pd->be_samples = nb;
                    pd->amp_samples = na;
                    pd->sample_times = nt;
                    pd->sample_capacity = new_cap;
                }
            }
            if (pd->sample_count < pd->sample_capacity) {
                pd->rate_samples[pd->sample_count] = snap->rate;
                pd->be_samples[pd->sample_count] = snap->be;
                pd->amp_samples[pd->sample_count] = snap->amp;
                pd->sample_times[pd->sample_count] = g_get_monotonic_time();
                pd->sample_count++;
            }
        }
    }

    /* --- Signal loss detection --- */
    if (signal > 0) {
        /* Signal present: reset any loss state */
        if (pt->sig_loss.signal_lost) {
            pt->sig_loss.signal_lost = false;
            pt->sig_loss.loss_start_time = 0;
        }
        pt->sig_loss.zero_signal_count = 0;
    } else {
        /* signal == 0 */
        pt->sig_loss.zero_signal_count++;
        if (!pt->sig_loss.signal_lost && pt->sig_loss.zero_signal_count >= 30) {
            /* ~3 seconds of continuous zero signal — mark as lost */
            pt->sig_loss.signal_lost = true;
            pt->sig_loss.loss_start_time = g_get_monotonic_time();
        }
        if (pt->sig_loss.signal_lost && !pt->sig_loss.dialog_shown) {
            double loss_duration = (g_get_monotonic_time() - pt->sig_loss.loss_start_time) / 1e6;
            if (loss_duration > 60.0) {
                /* 60 seconds of signal loss: show skip/cancel dialog */
                pt->sig_loss.dialog_shown = true;

                GtkWidget *dialog = gtk_message_dialog_new(
                    GTK_WINDOW(gtk_widget_get_toplevel(pt->drawing_area)),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_WARNING,
                    GTK_BUTTONS_NONE,
                    "Signal lost for over 60 seconds.\n"
                    "Position: %s",
                    pos_test_position_name(pt->current_position));

                gtk_dialog_add_button(GTK_DIALOG(dialog), "Skip Position", 1);
                gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel Test", 2);

                int response = gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);

                if (response == 1) {
                    pos_test_skip_position(pt);
                } else {
                    pos_test_cancel(pt);
                    return; /* pt is freed by pos_test_cancel */
                }
            }
        }
    }

    /* --- Countdown timer logic --- */
    double elapsed = (g_get_monotonic_time() - pd->phase_start_time) / 1e6;

    if (elapsed >= pt->position_duration) {
        /* Position phase complete — compute results */
        pos_test_compute_position_results(pd, pt->bph, pt->la, pt->cal,
                                          pt->nominal_sr, pt->settling_time);

        if (pt->current_position >= POS_COUNT - 1) {
            pt->state = POS_STATE_COMPLETE;
            /* Display results summary and show save button */
            pos_test_show_results(pt);
        } else {
            pt->state = POS_STATE_TRANSITION;
        }

        /* Show/hide continue button based on new state */
        if (pt->continue_button) {
            if (pt->state == POS_STATE_TRANSITION)
                gtk_widget_show(pt->continue_button);
            else
                gtk_widget_hide(pt->continue_button);
        }
    } else {
        /* Update status label with position name and countdown */
        int countdown = (int)floor(pt->position_duration - elapsed);
        const char *name = pos_test_position_name(
            (enum position_id)pt->current_position);

        if (pt->status_label) {
            char buf[128];
            if (elapsed < pt->settling_time) {
                snprintf(buf, sizeof(buf), "%s - Settling... %ds", name, countdown);
            } else {
                snprintf(buf, sizeof(buf), "%s - %ds", name, countdown);
            }
            gtk_label_set_text(GTK_LABEL(pt->status_label), buf);
        }

        /* Ensure continue button is hidden during active measurement */
        if (pt->continue_button)
            gtk_widget_hide(pt->continue_button);
    }

    /* Queue a redraw of the swim-lane display */
    if (pt->drawing_area)
        gtk_widget_queue_draw(pt->drawing_area);
}

/* --- Stub implementations (to be completed in later tasks) --- */

void pos_test_compute_position_results(struct position_data *pd,
                                        int bph, double la, int cal,
                                        int nominal_sr, int settling_time)
{
    UNUSED(bph);
    UNUSED(la);
    UNUSED(cal);
    UNUSED(nominal_sr);

    /* Compute final rate/BE/amplitude by averaging live snapshot readings
     * from the last settling_time seconds. This matches what was displayed
     * on the live meter during the test. */
    if (pd->sample_count == 0) {
        pd->valid = false;
        return;
    }

    /* Find samples within the last settling_time seconds */
    int64_t now = pd->sample_times[pd->sample_count - 1];
    int64_t window_start = now - (int64_t)settling_time * 1000000;

    double rate_sum = 0.0;
    double be_sum = 0.0;
    double amp_sum = 0.0;
    int count = 0;
    int amp_count = 0;

    for (int i = pd->sample_count - 1; i >= 0; i--) {
        if (pd->sample_times[i] < window_start)
            break;
        rate_sum += pd->rate_samples[i];
        be_sum += pd->be_samples[i];
        if (pd->amp_samples[i] > 0) {
            amp_sum += pd->amp_samples[i];
            amp_count++;
        }
        count++;
    }

    if (count < 5) {
        /* Not enough samples — use all available */
        rate_sum = 0.0;
        be_sum = 0.0;
        amp_sum = 0.0;
        count = 0;
        amp_count = 0;
        for (int i = 0; i < pd->sample_count; i++) {
            rate_sum += pd->rate_samples[i];
            be_sum += pd->be_samples[i];
            if (pd->amp_samples[i] > 0) {
                amp_sum += pd->amp_samples[i];
                amp_count++;
            }
            count++;
        }
    }

    if (count == 0) {
        pd->valid = false;
        return;
    }

    pd->rate = rate_sum / count;
    pd->beat_error = be_sum / count;
    pd->amplitude = amp_count > 0 ? amp_sum / amp_count : 0.0;
    pd->valid = true;
}

double pos_test_compute_delta(const struct positional_test *pt)
{
    if (!pt)
        return 0.0;

    double min_rate = 0.0;
    double max_rate = 0.0;
    int valid_count = 0;

    for (int i = 0; i < POS_COUNT; i++) {
        if (!pt->positions[i].valid)
            continue;

        double r = pt->positions[i].rate;
        if (valid_count == 0) {
            min_rate = r;
            max_rate = r;
        } else {
            if (r < min_rate) min_rate = r;
            if (r > max_rate) max_rate = r;
        }
        valid_count++;
    }

    if (valid_count <= 1)
        return 0.0;

    return max_rate - min_rate;
}

double pos_test_compute_average_rate(const struct positional_test *pt)
{
    if (!pt)
        return 0.0;

    double sum = 0.0;
    int valid_count = 0;

    for (int i = 0; i < POS_COUNT; i++) {
        if (!pt->positions[i].valid)
            continue;

        sum += pt->positions[i].rate;
        valid_count++;
    }

    if (valid_count == 0)
        return 0.0;

    return sum / valid_count;
}

/* --- Results summary display --- */

static void pos_test_show_results(struct positional_test *pt)
{
    if (!pt)
        return;

    double delta = pos_test_compute_delta(pt);
    double avg_rate = pos_test_compute_average_rate(pt);

    /* Build results text using g_string for easy concatenation */
    GString *text = g_string_new("");

    g_string_append(text, "Position     Rate(s/d)  BE(ms)  Amp(deg)\n");
    g_string_append(text, "──────────── ───────── ─────── ────────\n");

    for (int i = 0; i < POS_COUNT; i++) {
        const struct position_data *pd = &pt->positions[i];
        const char *name = pos_test_position_name((enum position_id)i);

        if (pd->valid) {
            g_string_append_printf(text, "%-12s %+8.1f %7.1f %8.0f\n",
                                   name, pd->rate, pd->beat_error,
                                   pd->amplitude);
        } else {
            g_string_append_printf(text, "%-12s  Insufficient Data\n", name);
        }
    }

    g_string_append(text, "──────────── ───────── ─────── ────────\n");
    g_string_append_printf(text, "Delta:       %8.1f s/d\n", delta);
    g_string_append_printf(text, "Average:     %+8.1f s/d\n", avg_rate);

    /* Set monospace markup on the results label using <tt> tags */
    if (pt->results_label) {
        char *markup = g_markup_printf_escaped("<tt>%s</tt>", text->str);
        gtk_label_set_markup(GTK_LABEL(pt->results_label), markup);
        g_free(markup);
        gtk_widget_show(pt->results_label);
    }

    /* Show the save button */
    if (pt->save_button)
        gtk_widget_show(pt->save_button);

    g_string_free(text, TRUE);
}

char *pos_test_generate_report(const struct positional_test *pt)
{
    if (!pt)
        return NULL;

    GString *report = g_string_new("");

    /* 1. Header section */
    g_string_append(report, "========================================\n");
    g_string_append(report, "  POSITIONAL TEST REPORT\n");
    g_string_append(report, "========================================\n");

    /* Watch name */
    if (pt->watch_name && pt->watch_name[0])
        g_string_append_printf(report, "Watch:      %s\n", pt->watch_name);

    /* Date/time */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char datetime[64];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", tm_info);
    g_string_append_printf(report, "Date:       %s\n", datetime);

    /* Configuration values */
    g_string_append_printf(report, "BPH:        %d\n", pt->bph);
    g_string_append_printf(report, "Lift Angle: %.0f deg\n", pt->la);
    g_string_append_printf(report, "Calibration: %+.1f s/d\n", (double)pt->cal);
    g_string_append_printf(report, "Duration:   %ds (settling: %ds)\n",
                           pt->position_duration, pt->settling_time);

    /* 2. Column headers */
    g_string_append(report, "----------------------------------------\n");
    g_string_append(report, "Position     Rate(s/d)  BE(ms)  Amp(deg)\n");
    g_string_append(report, "----------------------------------------\n");

    /* 3. Position rows */
    for (int i = 0; i < POS_COUNT; i++) {
        const struct position_data *pd = &pt->positions[i];
        const char *name = pos_test_position_name((enum position_id)i);

        if (pd->valid) {
            g_string_append_printf(report, "%-12s %+8.1f %7.1f %8.0f\n",
                                   name, pd->rate, pd->beat_error,
                                   pd->amplitude);
        } else {
            g_string_append_printf(report, "%-12s  Insufficient Data\n", name);
        }
    }

    /* 4. Summary section */
    g_string_append(report, "----------------------------------------\n");

    double delta = pos_test_compute_delta(pt);
    double avg_rate = pos_test_compute_average_rate(pt);

    g_string_append_printf(report, "Delta:      %.1f s/d\n", delta);
    g_string_append_printf(report, "Average:    %+.1f s/d\n", avg_rate);
    g_string_append(report, "========================================\n");

    /* Return the buffer — caller frees with free() */
    return g_string_free(report, FALSE);
}

const char *pos_test_position_name(enum position_id pos)
{
    static const char *names[POS_COUNT] = {
        "Dial Up",
        "Dial Down",
        "Crown Up",
        "Crown Down",
        "Crown Left",
        "Crown Right"
    };

    if (pos < 0 || pos >= POS_COUNT)
        return "Unknown";

    return names[pos];
}

const char *pos_test_position_abbrev(enum position_id pos)
{
    static const char *abbrevs[POS_COUNT] = {
        "DU",
        "DD",
        "CU",
        "CD",
        "CL",
        "CR"
    };

    if (pos < 0 || pos >= POS_COUNT)
        return "??";

    return abbrevs[pos];
}
