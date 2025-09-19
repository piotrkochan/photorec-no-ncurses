/*

    File: json_log.c

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "types.h"
#include "common.h"
#include "list.h"
#include "filegen.h"
#include "photorec.h"
#include "json_log.h"

/* Global JSON log file handle */
static FILE* json_log_file = NULL;

char* get_iso_timestamp(void)
{
  static char timestamp[32];
  time_t now;
  struct tm* tm_info;

  time(&now);
  tm_info = localtime(&now);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S%z", tm_info);

  return timestamp;
}

FILE* json_log_open(const char* filename)
{
  if(filename == NULL) return NULL;

  json_log_file = fopen(filename, "w");
  if(json_log_file == NULL) {
    fprintf(stderr, "Error: Cannot create JSON log file: %s\n", filename);
    return NULL;
  }

  return json_log_file;
}

void json_log_close(FILE* log_file)
{
  (void)log_file; /* suppress unused parameter warning */
  if(json_log_file != NULL) {
    fclose(json_log_file);
    json_log_file = NULL;
  }
}

void json_log_session_start(const struct ph_param* params,
                           const struct ph_options* options)
{
  if(json_log_file == NULL) return;

  fprintf(json_log_file, "{\"type\":\"session_start\",\"timestamp\":\"%s\",\"photorec_version\":\"7.3-WIP\",\"user_is_root\":%s",
          get_iso_timestamp(), (geteuid() == 0) ? "true" : "false");
  if(params->recup_dir)
    fprintf(json_log_file, ",\"recovery_directory\":\"%s\"", params->recup_dir);
  if(params->max_file_size > 0)
    fprintf(json_log_file, ",\"max_file_size\":%llu", (unsigned long long)params->max_file_size);
  fprintf(json_log_file, ",\"paranoid_mode\":%s,\"keep_corrupted_files\":%s}\n",
          options->paranoid ? "true" : "false",
          options->keep_corrupted_file ? "true" : "false");
  fflush(json_log_file);
}

void json_log_partition_info(const disk_t* disk)
{
  if(json_log_file == NULL) return;

  if(disk == NULL) {
    fprintf(json_log_file, "{\"type\":\"partition_info\",\"timestamp\":\"%s\",\"disk\":\"NULL\",\"size_gb\":0}\n",
            get_iso_timestamp());
  } else {
    fprintf(json_log_file, "{\"type\":\"partition_info\",\"timestamp\":\"%s\",\"disk\":\"%s\",\"size_gb\":%.1f}\n",
            get_iso_timestamp(),
            disk->device ? disk->device : "Unknown",
            (double)disk->disk_size / (1024.0 * 1024.0 * 1024.0));
  }
  fflush(json_log_file);
}

void json_log_recovery_progress(const struct ph_param* params,
                               uint64_t current_sector, uint64_t total_sectors,
                               const char* elapsed_time, const char* estimated_time,
                               const file_stat_t* file_stats)
{
  if(json_log_file == NULL) return;

  fprintf(json_log_file, "{\"type\":\"recovery_progress\",\"timestamp\":\"%s\",\"pass\":%u,\"operation\":\"Reading sector\",\"current_sector\":%llu,\"total_sectors\":%llu",
          get_iso_timestamp(), params->pass, (unsigned long long)current_sector, (unsigned long long)total_sectors);
  if(elapsed_time)
    fprintf(json_log_file, ",\"elapsed_time\":\"%s\"", elapsed_time);
  if(estimated_time)
    fprintf(json_log_file, ",\"estimated_completion\":\"%s\"", estimated_time);

  fprintf(json_log_file, ",\"files_found\":{");
  if(file_stats)
  {
    int first = 1;
    int total = 0;
    int i;
    /* Iterate through file_stats array until file_hint is NULL */
    for(i = 0; file_stats[i].file_hint != NULL; i++)
    {
      if(file_stats[i].recovered > 0)
      {
        if(!first) fprintf(json_log_file, ",");
        fprintf(json_log_file, "\"%s\":%u",
                file_stats[i].file_hint->extension ? file_stats[i].file_hint->extension : "unknown",
                file_stats[i].recovered);
        total += file_stats[i].recovered;
        first = 0;
      }
    }
    fprintf(json_log_file, "},\"files_total\":%d}\n", total);
  }
  else {
    fprintf(json_log_file, "},\"files_total\":0}\n");
  }
  fflush(json_log_file);
}

void json_log_file_recovered(const char* filename, const char* filetype,
                             uint64_t filesize, uint64_t offset)
{
  if(json_log_file == NULL) return;

  fprintf(json_log_file, "{\"type\":\"file_recovered\",\"timestamp\":\"%s\"", get_iso_timestamp());
  if(filename)
    fprintf(json_log_file, ",\"filename\":\"%s\"", filename);
  if(filetype)
    fprintf(json_log_file, ",\"filetype\":\"%s\"", filetype);
  fprintf(json_log_file, ",\"filesize\":%llu,\"disk_offset\":%llu}\n",
          (unsigned long long)filesize, (unsigned long long)offset);
  fflush(json_log_file);
}

void json_log_disk_info(const disk_t* disk)
{
  if(json_log_file == NULL || disk == NULL) return;

  fprintf(json_log_file, "{\"type\":\"disk_info\",\"timestamp\":\"%s\",\"device\":\"%s\",\"size_gb\":%.1f,\"sector_size\":%u",
          get_iso_timestamp(),
          disk->device ? disk->device : "Unknown",
          (double)disk->disk_size / (1024.0 * 1024.0 * 1024.0),
          disk->sector_size);

  if(disk->geom.cylinders > 0) {
    fprintf(json_log_file, ",\"chs\":\"%lu %u %u\"",
            disk->geom.cylinders, disk->geom.heads_per_cylinder, disk->geom.sectors_per_head);
  }

  fprintf(json_log_file, "}\n");
  fflush(json_log_file);
}

void json_log_options_info(const struct ph_options* options)
{
  if(json_log_file == NULL || options == NULL) return;

  fprintf(json_log_file, "{\"type\":\"options_info\",\"timestamp\":\"%s\",\"paranoid\":%s,\"brute_force\":%s,\"keep_corrupted\":%s,\"ext2_mode\":%s,\"expert_mode\":%s,\"low_memory\":%s}\n",
          get_iso_timestamp(),
          options->paranoid ? "true" : "false",
          options->mode_ext2 ? "true" : "false",
          options->keep_corrupted_file ? "true" : "false",
          options->mode_ext2 ? "true" : "false",
          options->expert ? "true" : "false",
          options->lowmem ? "true" : "false");
  fflush(json_log_file);
}

void json_log_signatures_info(int signatures_count)
{
  if(json_log_file == NULL) return;

  fprintf(json_log_file, "{\"type\":\"signatures_info\",\"timestamp\":\"%s\",\"enabled_signatures\":%d}\n",
          get_iso_timestamp(), signatures_count);
  fflush(json_log_file);
}

void json_log_pass_info(int pass, int blocksize, const char* status)
{
  if(json_log_file == NULL) return;

  fprintf(json_log_file, "{\"type\":\"pass_info\",\"timestamp\":\"%s\",\"pass\":%d,\"blocksize\":%d",
          get_iso_timestamp(), pass, blocksize);
  if(status)
    fprintf(json_log_file, ",\"status\":\"%s\"", status);
  fprintf(json_log_file, "}\n");
  fflush(json_log_file);
}

void json_log_session_end(const file_stat_t* file_stats)
{
  if(json_log_file == NULL) return;

  fprintf(json_log_file, "{\"type\":\"session_end\",\"timestamp\":\"%s\",\"final_statistics\":{",
          get_iso_timestamp());

  if(file_stats)
  {
    int first = 1;
    int total = 0;
    int i;
    /* Iterate through file_stats array until file_hint is NULL */
    for(i = 0; file_stats[i].file_hint != NULL; i++)
    {
      if(file_stats[i].recovered > 0)
      {
        if(!first) fprintf(json_log_file, ",");
        fprintf(json_log_file, "\"%s\":%u",
                file_stats[i].file_hint->extension ? file_stats[i].file_hint->extension : "unknown",
                file_stats[i].recovered);
        total += file_stats[i].recovered;
        first = 0;
      }
    }
    fprintf(json_log_file, "},\"files_total\":%d}\n", total);
  }
  else {
    fprintf(json_log_file, "},\"files_total\":0}\n");
  }
  fflush(json_log_file);
}