#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "linUDP.h"

typedef struct {
    int counter;
    double last_time;
    double fps;
    gboolean initialized;
} AppState;

gboolean on_tick(GtkWidget *widget, GdkFrameClock *clock, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    gint64 frame_time = gdk_frame_clock_get_frame_time(clock);
    double current_time = frame_time / 1000000.0; // Convert to milliseconds

    if (!state->initialized) {
        state->last_time = current_time;
        state->initialized = TRUE;
    }

    double delta = (current_time - state->last_time) / 1000000.0; // Convert to seconds
    if (delta > 0.0) {
        state->fps = 1.0 / delta;
        state->last_time = current_time;
    }

    state->counter++;

    gtk_widget_queue_draw(widget);
    return G_SOURCE_CONTINUE;
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    // Clear background
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_paint(cr);

    // Text settings
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // white

    // Print 4 lines of text
    char line[128];
    double y = 50;

    snprintf(line, sizeof(line), "FPS: %.2f", state->fps);
    cairo_move_to(cr, 50, y);
    cairo_show_text(cr, line);

    y += 40;
    snprintf(line, sizeof(line), "Time (s): %.2f", (float) state->last_time);
    cairo_move_to(cr, 50, y);
    cairo_show_text(cr, line);

    y += 40;
    snprintf(line, sizeof(line), "Counter: %d", state->counter);
    cairo_move_to(cr, 50, y);
    cairo_show_text(cr, line);

    y += 40;
    snprintf(line, sizeof(line), "Random: %d", rand() % 100);
    cairo_move_to(cr, 50, y);
    cairo_show_text(cr, line);

    y += 40;
    snprintf(line, sizeof(line), "Pos: %9.2f mm / %9.2f mm", (float)atomic_load(&globDemPos) / 10000.0, (float)atomic_load(&globActPos) / 10000.0);
    cairo_move_to(cr, 50, y);
    cairo_show_text(cr, line);

    return FALSE;
}

void* gtk_thread_func(void* arg) {
    gtk_init(NULL, NULL);

    AppState *state = g_new0(AppState, 1);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_maximize(GTK_WINDOW(window));
    
    GtkWidget *area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), area);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(area, "draw", G_CALLBACK(on_draw), state);
    gtk_widget_add_tick_callback(area, on_tick, state, NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return NULL;
}

