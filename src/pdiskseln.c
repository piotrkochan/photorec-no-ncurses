/*

    File: pdiskseln.c

    Copyright (C) 1998-2008 Christophe GRENIER <grenier@cgsecurity.org>

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

#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(DJGPP) || !defined(HAVE_GETEUID)
#undef SUDO_BIN
#endif

#if defined(DISABLED_FOR_FRAMAC)
#undef HAVE_NCURSES
#endif

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>	/* geteuid */
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "types.h"
#include "common.h"
#include "intrf.h"
#if defined(HAVE_NCURSES)
#include "intrfn.h"
#endif
#include "dir.h"
#include "list.h"
#include "filegen.h"
#include "photorec.h"
#include "sessionp.h"
#include "json_log.h"
#include "partauto.h"
#include "log.h"
#include "pdisksel.h"
#include "pdiskseln.h"
#include "ppartseln.h"
#include "hidden.h"
#include "hiddenn.h"
#include "nodisk.h"
#include "chgarch.h"
#include "chgarchn.h"
#include "autoset.h"

#if defined(HAVE_NCURSES)
#define NBR_DISK_MAX 		(LINES-6-8)
#define INTER_DISK_X		0
#define INTER_DISK_Y		(8+NBR_DISK_MAX)
#define INTER_NOTE_Y		(LINES-4)
#endif

extern const arch_fnct_t arch_none;


#if defined(HAVE_NCURSES)
static int photorec_disk_selection_ncurses(struct ph_param *params, struct ph_options *options, const list_disk_t *list_disk, alloc_data_t *list_search_space)
{
  int command;
  int real_key;
  unsigned int menu=0;
  int offset=0;
  int pos_num=0;
#ifdef SUDO_BIN
  int use_sudo=0;
#endif
  const list_disk_t *element_disk;
  const list_disk_t *current_disk=list_disk;
  static const struct MenuItem menuMain[]=
  {
    { 'P', "Previous",""},
    { 'N', "Next","" },
    { 'O',"Proceed",""},
    { 'S', "Sudo", "Use the sudo command to restart as root"},
    { 'Q',"Quit","Quit program"},
    { 0,NULL,NULL}
  };
  if(list_disk==NULL)
  {
    log_critical("No disk found\n");
    return intrf_no_disk_ncurses("PhotoRec");
  }
  /* ncurses interface */
  while(1)
  {
    const char *menu_options;
    int i;
    aff_copy(stdscr);
    wmove(stdscr,4,0);
    wprintw(stdscr,"  PhotoRec is free software, and");
    wmove(stdscr,5,0);
    wprintw(stdscr,"comes with ABSOLUTELY NO WARRANTY.");
    wmove(stdscr,7,0);
    wprintw(stdscr,"Select a media and choose 'Proceed' using arrow keys:");
    for(i=0,element_disk=list_disk;
	element_disk!=NULL && i<offset+NBR_DISK_MAX;
	i++, element_disk=element_disk->next)
    {
      if(i<offset)
	continue;
      wmove(stdscr,8+i-offset,0);
      if(element_disk!=current_disk)
	wprintw(stdscr," %s\n",element_disk->disk->description_short(element_disk->disk));
      else
      {
	wattrset(stdscr, A_REVERSE);
	wprintw(stdscr,">%s\n",element_disk->disk->description_short(element_disk->disk));
	wattroff(stdscr, A_REVERSE);
      }
    }
    {
      mvwaddstr(stdscr, INTER_NOTE_Y,0,"Note: ");
#if defined(HAVE_GETEUID) && !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(DJGPP)
      if(geteuid()!=0)
      {
	if(has_colors())
	  wbkgdset(stdscr,' ' | A_BOLD | COLOR_PAIR(1));
	wprintw(stdscr,"Some disks won't appear unless you're root user.");
        if(has_colors())
          wbkgdset(stdscr,' ' | COLOR_PAIR(0));
#ifdef SUDO_BIN
	use_sudo=1;
#endif
      }
      else
#endif
      if(current_disk != NULL && current_disk->disk->serial_no != NULL)
      {
        if(has_colors())
          wbkgdset(stdscr,' ' | A_BOLD | COLOR_PAIR(2));
	wprintw(stdscr, "Serial number %s", current_disk->disk->serial_no);
        if(has_colors())
          wbkgdset(stdscr,' ' | COLOR_PAIR(0));
      }
      wmove(stdscr, INTER_NOTE_Y+1, 0);
      wprintw(stdscr,"Disk capacity must be correctly detected for a successful recovery.");
      wmove(stdscr, INTER_NOTE_Y+2, 0);
      wprintw(stdscr,"If a disk listed above has an incorrect size, check HD jumper settings and BIOS");
      wmove(stdscr, INTER_NOTE_Y+3, 0);
      wprintw(stdscr,"detection, and install the latest OS patches and disk drivers."); 
    }
#ifdef SUDO_BIN
    if(use_sudo > 0)
    {
      if(i<=NBR_DISK_MAX && element_disk==NULL)
	menu_options="OSQ";
      else
	menu_options="PNOSQ";
    }
    else
#endif
    {
      if(i<=NBR_DISK_MAX && element_disk==NULL)
	menu_options="OQ";
      else
	menu_options="PNOQ";
    }
    command = wmenuSelect_ext(stdscr, INTER_NOTE_Y-1, INTER_DISK_Y, INTER_DISK_X, menuMain, 8,
	menu_options, MENU_HORIZ | MENU_BUTTON | MENU_ACCEPT_OTHERS, &menu,&real_key);
    switch(command)
    {
      case KEY_UP:
      case 'P':
	if(current_disk->prev!=NULL)
	{
	  current_disk=current_disk->prev;
	  pos_num--;
	}
	break;
      case KEY_DOWN:
      case 'N':
	if(current_disk->next!=NULL)
	{
	  current_disk=current_disk->next;
	  pos_num++;
	}
	break;
      case KEY_PPAGE:
	for(i=0;i<NBR_DISK_MAX-1 && current_disk->prev!=NULL;i++)
	{
	  current_disk=current_disk->prev;
	  pos_num--;
	}
	break;
      case KEY_NPAGE:
	for(i=0;i<NBR_DISK_MAX-1 && current_disk->next!=NULL;i++)
	{
	  current_disk=current_disk->next;
	  pos_num++;
	}
	break;
      case 'o':
      case 'O':
	{
	  disk_t *disk=current_disk->disk;
	  const int hpa_dco=is_hpa_or_dco(disk);
	  autodetect_arch(disk, &arch_none);
	  autoset_unit(disk);
	  params->disk=disk;
	  /* Log detailed disk and partition info after disk is set */
	  json_log_partition_info(params->disk);
	  json_log_disk_info(params->disk);
	  if((hpa_dco==0 || interface_check_hidden_ncurses(disk, hpa_dco)==0) &&
	      (options->expert == 0 ||
	       change_arch_type_ncurses(disk, options->verbose)==0))
	    menu_photorec(params, options, list_search_space);
	}
	break;
      case 's':
      case 'S':
	return 1;
      case 'q':
      case 'Q':
	return 0;
    }
    if(pos_num<offset)
      offset=pos_num;
    if(pos_num>=offset+NBR_DISK_MAX)
      offset=pos_num-NBR_DISK_MAX+1;
  }
}
#endif

int do_curses_photorec(struct ph_param *params, struct ph_options *options, const list_disk_t *list_disk)
{
  static alloc_data_t list_search_space={
    .list = TD_LIST_HEAD_INIT(list_search_space.list)
  };
  const int resume_session=(params->cmd_device!=NULL && strcmp(params->cmd_device,"resume")==0);

  log_info("do_curses_photorec: cmd_device='%s', resume_session=%d\n",
    params->cmd_device ? params->cmd_device : "NULL", resume_session);

  /* Log JSON session start if enabled */
  json_log_session_start(params, options);
#ifndef DISABLED_FOR_FRAMAC
  if(params->cmd_device==NULL || resume_session!=0)
  {
    char *saved_device=NULL;
    char *saved_cmd=NULL;
    session_load(&saved_device, &saved_cmd,&list_search_space, params);
    log_info("session_load: saved_device='%s', saved_cmd='%s', list_empty=%d\n",
      saved_device ? saved_device : "NULL", saved_cmd ? saved_cmd : "NULL", td_list_empty(&list_search_space.list));
    if(saved_device!=NULL && saved_cmd!=NULL && !td_list_empty(&list_search_space.list) && resume_session!=0)
    {
      /* For resume, extract file options from saved session command */
      char *cli_cmd = NULL;
      char *fileopt_start = strstr(saved_cmd, "fileopt,");
      char *options_start = strstr(saved_cmd, ",options,");

      /* Try direct resume with search command */
      params->cmd_device=NULL;  /* Don't use CLI mode for device selection */
      params->cmd_run=strdup("search");  /* Simple search command - session contains the ranges */

      /* Log available disks */
      log_info("Resume: Looking for disk '%s' in available disks:\n", saved_device);
      {
        const list_disk_t *disk_walker;
        int disk_count = 0;
        for(disk_walker = list_disk; disk_walker != NULL; disk_walker = disk_walker->next) {
          log_info("Resume: Available disk %d: '%s'\n", disk_count++, disk_walker->disk->device);
        }
      }

      params->disk=photorec_disk_selection_cli(saved_device, list_disk, &list_search_space);
      if(params->disk != NULL) {
        log_info("Resume: Found disk %s\n", params->disk->device);
        log_info("Resume: Search space has %s entries\n", td_list_empty(&list_search_space.list) ? "no" : "some");
        /* Log first few search space ranges */
        {
          struct td_list_head *search_walker = NULL;
          int range_count = 0;
          td_list_for_each(search_walker, &list_search_space.list) {
            alloc_data_t *range = td_list_entry(search_walker, alloc_data_t, list);
            log_info("Resume: Range %d: %llu-%llu (sectors: %llu-%llu)\n", range_count++,
              (unsigned long long)range->start, (unsigned long long)range->end,
              (unsigned long long)(range->start / params->disk->sector_size),
              (unsigned long long)(range->end / params->disk->sector_size));
            if(range_count >= 5) break;  /* Just show first 5 ranges */
          }
        }
        /* Set up basic parameters for recovery */
        autoset_unit(params->disk);
        /* TODO: Apply saved file options from session */
        /* Start recovery directly */
        log_info("Resume: Starting menu_photorec with %d search ranges\n",
          td_list_empty(&list_search_space.list) ? 0 : 1);
        menu_photorec(params, options, &list_search_space);
        log_info("Resume: Recovery completed\n");
        free(params->cmd_run);
        params->cmd_run=NULL;
      } else {
        log_info("Resume: No disk found\n");
        log_critical("Resume failed: Disk '%s' not found in available disks.\n", saved_device);
        log_critical("To resume, run: photorec %s /cmd resume\n", saved_device);
      }
      free(saved_device);
      free(saved_cmd);
    }
    else
    {
      free(saved_device);
      free(saved_cmd);
      free_list_search_space(&list_search_space);
      rename("photorec.ses", "photorec.se2");
    }
  }
#endif
  if(params->cmd_device!=NULL && params->cmd_run!=NULL)
  {
    /*@ assert valid_read_string(params->cmd_run); */
    params->disk=photorec_disk_selection_cli(params->cmd_device, list_disk, &list_search_space);
    /*@ assert params->disk == \null || valid_disk(params->disk); */
    /* Log detailed disk and partition info after disk is set in CLI mode */
    if(params->disk != NULL) {
      json_log_partition_info(params->disk);
      json_log_disk_info(params->disk);
    }
    if(params->disk==NULL)
    {
      log_critical("No disk found\n");
      /*@ assert params->cmd_run == \null || valid_read_string(params->cmd_run); */
      return 0;
    }
    /*@ assert valid_disk(params->disk); */
    change_arch_type_cli(params->disk, options->verbose, &params->cmd_run);
    autoset_unit(params->disk);
    menu_photorec(params, options, &list_search_space);
    /*@ assert params->cmd_run == \null || valid_read_string(params->cmd_run); */
    return 0;
  }
  /*@ assert params->cmd_run == \null || valid_read_string(params->cmd_run); */
  /* Log JSON session end if enabled */
  json_log_session_end(params->file_stats);
  return 0;
}
