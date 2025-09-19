/*

    File: json_log.h

    Copyright (C) 2025 JSON Logging for PhotoRec

    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

#ifndef _JSON_LOG_H
#define _JSON_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>
#include "types.h"
#include "photorec.h"

/* JSON logging functions */
FILE* json_log_open(const char* filename);
void json_log_close(FILE* log_file);
void json_log_partition_info(const disk_t* disk);
void json_log_recovery_progress(const struct ph_param* params,
                               uint64_t current_sector, uint64_t total_sectors,
                               const char* elapsed_time, const char* estimated_time,
                               const file_stat_t* file_stats);
void json_log_file_recovered(const char* filename, const char* filetype,
                             uint64_t filesize, uint64_t offset);
void json_log_session_start(const struct ph_param* params,
                           const struct ph_options* options);
void json_log_disk_info(const disk_t* disk);
void json_log_options_info(const struct ph_options* options);
void json_log_signatures_info(int signatures_count);
void json_log_pass_info(int pass, int blocksize, const char* status);
void json_log_session_end(const file_stat_t* file_stats);
char* get_iso_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif