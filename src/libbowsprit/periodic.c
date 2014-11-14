/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <unistd.h>

#include <clogger.h>
#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/os.h>
#include <libcork/threads.h>
#include <libcork/helpers/errors.h>

#include "bowsprit.h"

#define CLOG_CHANNEL  "bowsprit"


/*-----------------------------------------------------------------------
 * Periodic tasks
 */

#define DEFAULT_INTERVAL_SEC  30
#define SLEEP_INTERVAL_MSEC   10

struct bws_periodic {
    struct bws_ctx  *ctx;
    struct bws_snapshot  *snapshot;
    cork_timestamp  interval;
    cork_timestamp  next_fire;

    void  *user_data;
    cork_free_f  free_user_data;
    bws_periodic_run_f  run;

    struct cork_thread  *thread;
    bool  should_continue;
};

struct bws_periodic *
bws_periodic_new(struct bws_ctx *ctx,
                 void *user_data, cork_free_f free_user_data,
                 bws_periodic_run_f run)
{
    struct bws_periodic  *periodic = cork_new(struct bws_periodic);
    periodic->ctx = ctx;
    periodic->snapshot = bws_snapshot_new();
    cork_timestamp_init_sec(&periodic->interval, DEFAULT_INTERVAL_SEC);
    periodic->next_fire = 0;
    periodic->user_data = user_data;
    periodic->free_user_data = free_user_data;
    periodic->run = run;
    periodic->thread = NULL;
    return periodic;
}

void
bws_periodic_free(struct bws_periodic *periodic)
{
    assert(periodic->thread == NULL);
    bws_snapshot_free(periodic->snapshot);
    cork_free_user_data(periodic);
    cork_delete(struct bws_periodic, periodic);
}

void
bws_periodic_set_interval(struct bws_periodic *periodic, unsigned int seconds)
{
    assert(periodic->next_fire == 0 && periodic->thread == NULL);
    cork_timestamp_init_sec(&periodic->interval, seconds);
}

static int
bws_periodic_poll_(struct bws_periodic *periodic, cork_timestamp now)
{
    if (CORK_UNLIKELY(periodic->next_fire == 0)) {
        clog_debug("Process first snapshot");
        periodic->next_fire = now + periodic->interval;
        bws_ctx_snapshot(periodic->ctx, periodic->snapshot);
        return periodic->run(periodic->user_data, periodic->snapshot, now);
    } else if (CORK_UNLIKELY(now >= periodic->next_fire)) {
        clog_debug("Process snapshot");
        cork_timestamp  fire_ts = periodic->next_fire;
        periodic->next_fire += periodic->interval;
        bws_ctx_snapshot(periodic->ctx, periodic->snapshot);
        return periodic->run(periodic->user_data, periodic->snapshot, fire_ts);
    } else {
        return 0;
    }
}

int
bws_periodic_poll(struct bws_periodic *periodic)
{
    cork_timestamp  now;
    cork_timestamp_init_now(&now);
    return bws_periodic_poll_(periodic, now);
}

int
bws_periodic_mocked_poll(struct bws_periodic *periodic, cork_timestamp now)
{
    return bws_periodic_poll_(periodic, now);
}


static int
bws_periodic__background_run(void *user_data)
{
    struct bws_periodic  *periodic = user_data;
    while (CORK_LIKELY(periodic->should_continue)) {
        usleep(SLEEP_INTERVAL_MSEC * 1000);
        rii_check(bws_periodic_poll(periodic));
    }
    return 0;
}

int
bws_periodic_run_in_background(struct bws_periodic *periodic)
{
    assert(periodic->thread == NULL);
    periodic->should_continue = true;
    periodic->thread = cork_thread_new
        ("bowsprit", periodic, NULL, bws_periodic__background_run);
    return cork_thread_start(periodic->thread);
}

int
bws_periodic_join(struct bws_periodic *periodic)
{
    struct cork_thread  *thread;
    assert(periodic->thread != NULL);
    thread = periodic->thread;
    periodic->thread = NULL;
    periodic->should_continue = false;
    return cork_thread_join(thread);
}
