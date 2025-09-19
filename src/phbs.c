/*

    File: phbs.c

    Copyright (C) 1998-2009 Christophe GRENIER <grenier@cgsecurity.org>

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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include "types.h"
#include "common.h"
#include "intrf.h"
#ifdef HAVE_NCURSES
#include "intrfn.h"
#endif
#include "list.h"
#include "filegen.h"
#include "photorec.h"
#include "log.h"
#include "file_tar.h"
#include "pnext.h"
#include "phnc.h"
#include "phbs.h"
#include "file_found.h"
#include "json_log.h"

#define READ_SIZE 1024*512

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_tar)
extern const file_hint_t file_hint_tar;
#endif
extern file_check_list_t file_check_list;
extern int need_to_stop;

/*@
  @ requires \valid(dst);
  @ requires \valid_read(src);
  @ requires \separated(src, dst);
  @*/
static inline void file_recovery_cpy(file_recovery_t *dst, const file_recovery_t *src)
{
  memcpy(dst, src, sizeof(*dst));
  dst->location.list.prev=&dst->location.list;
  dst->location.list.next=&dst->location.list;
}

pstatus_t photorec_find_blocksize(struct ph_param *params, const struct ph_options *options, alloc_data_t *list_search_space)
{
  uint64_t offset=0;
  unsigned char *buffer_start;
  unsigned char *buffer_olddata;
  unsigned char *buffer;
  time_t start_time;
  time_t previous_time;
  unsigned int buffer_size;
  const unsigned int blocksize=params->blocksize;
  const unsigned int read_size=(blocksize>65536?blocksize:65536);
  /*@ assert read_size >= 65536; */
  alloc_data_t *current_search_space;
  file_recovery_t file_recovery;
#ifndef DISABLED_FOR_FRAMAC
  params->file_nbr=0;
  reset_file_recovery(&file_recovery);
  file_recovery.blocksize=blocksize;
  buffer_size=blocksize + READ_SIZE;
  buffer_start=(unsigned char *)MALLOC(buffer_size);
  buffer_olddata=buffer_start;
  buffer=buffer_olddata + blocksize;
  start_time=time(NULL);
  previous_time=start_time;
  memset(buffer_olddata, 0, blocksize);

  /* Variables for JSON progress logging */
  time_t last_json_log_time = time(NULL);
  current_search_space=td_list_first_entry(&list_search_space->list, alloc_data_t, list);
  if(current_search_space!=list_search_space)
    offset=current_search_space->start;
  if(options->verbose>0)
    info_list_search_space(list_search_space, current_search_space, params->disk->sector_size, 0, options->verbose);
  params->disk->pread(params->disk, buffer, READ_SIZE, offset);
  while(current_search_space!=list_search_space)
  {
    const uint64_t old_offset=offset;
    {
      file_recovery_t file_recovery_new;
      file_recovery_new.blocksize=blocksize;
      file_recovery_new.location.start=offset;
#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_tar)
      if(file_recovery.file_stat!=NULL && file_recovery.file_stat->file_hint==&file_hint_tar &&
          is_valid_tar_header((const struct tar_posix_header *)(buffer-0x200)))
      { /* Currently saving a tar, do not check the data for know header */
      }
      else
#endif
      {
	const struct td_list_head *tmpl;
        file_recovery_new.file_stat=NULL;
	td_list_for_each(tmpl, &file_check_list.list)
	{
	  const struct td_list_head *tmp;
	  const file_check_list_t *pos=td_list_entry_const(tmpl, const file_check_list_t, list);
	  td_list_for_each(tmp, &pos->file_checks[buffer[pos->offset]].list)
	  {
	    const file_check_t *file_check=td_list_entry_const(tmp, const file_check_t, list);
	    /*@ assert valid_file_check_node(file_check); */
	    if((file_check->length==0 || memcmp(buffer + file_check->offset, file_check->value, file_check->length)==0) &&
		file_check->header_check(buffer, read_size, 1, &file_recovery, &file_recovery_new)!=0)
	    {
	      file_recovery_new.file_stat=file_check->file_stat;
	      break;
	    }
	  }
	  if(file_recovery_new.file_stat!=NULL)
	    break;
	}
        if(file_recovery_new.file_stat!=NULL && file_recovery_new.file_stat->file_hint!=NULL)
	{
	  /* A new file begins, backup file offset */
	  current_search_space=file_found(current_search_space, offset, file_recovery_new.file_stat);
	  params->file_nbr++;
	  file_recovery_cpy(&file_recovery, &file_recovery_new);
	}
      }
    }
    /* Check for data EOF */
    if(file_recovery.file_stat!=NULL)
    {
      data_check_t res=DC_CONTINUE;
      if(file_recovery.data_check!=NULL)
	res=file_recovery.data_check(buffer_olddata, 2*blocksize, &file_recovery);
      file_recovery.file_size+=blocksize;
      /* Check global file size limit from /maxsize parameter for early termination */
      if(res!=DC_STOP && res!=DC_ERROR && params->max_file_size > 0 && file_recovery.file_size > params->max_file_size)
      {
	res=DC_STOP;
#ifndef DISABLED_FOR_FRAMAC
	log_verbose("File size limit reached (%llu), stopping recovery early\n",
	    (long long unsigned)params->max_file_size);
#endif
      }
      if(res==DC_STOP || res==DC_ERROR)
      {
	/* EOF found */
	reset_file_recovery(&file_recovery);
      }
    }
    /* Check for maximum filesize */
    if(file_recovery.file_stat!=NULL && file_recovery.file_stat->file_hint->max_filesize>0 && file_recovery.file_size>=file_recovery.file_stat->file_hint->max_filesize)
    {
      reset_file_recovery(&file_recovery);
    }

    if(params->file_nbr >= 10)
    {
      current_search_space=list_search_space;
    }
    else
      get_next_sector(list_search_space, &current_search_space, &offset, blocksize);
    if(current_search_space==list_search_space)
    {
      /* End of disk found => EOF */
      reset_file_recovery(&file_recovery);
    }
    buffer_olddata+=blocksize;
    buffer+=blocksize;
    if( old_offset+blocksize!=offset ||
        buffer+read_size>buffer_start+buffer_size)
    {
      memcpy(buffer_start, buffer_olddata, blocksize);
      buffer_olddata=buffer_start;
      buffer=buffer_olddata+blocksize;
      if(options->verbose>1)
      {
        log_verbose("Reading sector %10llu/%llu\n",
	    (unsigned long long)((offset - params->partition->part_offset) / params->disk->sector_size),
	    (unsigned long long)((params->partition->part_size-1) / params->disk->sector_size));
      }
      if(params->disk->pread(params->disk, buffer, READ_SIZE, offset) != READ_SIZE)
      {
#ifdef HAVE_NCURSES
	wmove(stdscr,11,0);
	wclrtoeol(stdscr);
	wprintw(stdscr,"Error reading sector %10lu\n",
	    (unsigned long)((offset - params->partition->part_offset) / params->disk->sector_size));
#endif
      }
      {
        time_t current_time;
        current_time=time(NULL);
        if(current_time>previous_time)
        {
          /* Log JSON progress every second during blocksize finding */
          if(current_time > last_json_log_time)
          {
            char elapsed_str[64];
            const time_t elapsed = current_time - start_time;
            snprintf(elapsed_str, sizeof(elapsed_str), "%uh%02um%02us",
                    (unsigned)(elapsed/3600),
                    (unsigned)((elapsed/60)%60),
                    (unsigned)(elapsed%60));

            uint64_t current_sector = (offset-params->partition->part_offset)/params->disk->sector_size;
            uint64_t total_sectors = (params->partition->part_size-1)/params->disk->sector_size;

            json_log_recovery_progress(params, current_sector, total_sectors, elapsed_str, NULL, params->file_stats);
            last_json_log_time = current_time;
          }

          previous_time=current_time;
#ifdef HAVE_NCURSES
          if(photorec_progressbar(stdscr, 0, params, offset, current_time))
	  {
	    log_info("PhotoRec has been stopped\n");
	    current_search_space=list_search_space;
	  }
#endif
	}
      }
      if(need_to_stop!=0)
      {
	log_info("PhotoRec has been stopped\n");
	current_search_space=list_search_space;
      }
    }
  } /* end while(current_search_space!=list_search_space) */
  free(buffer_start);
#endif
  return PSTATUS_OK;
}
