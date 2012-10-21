/*
 * xsys.c - main functions for X-Sys 2
 * by mikeshoup
 * Copyright (C) 2003, 2004, 2005 Michael Shoup
 * Copyright (C) 2005, 2006, 2007 Tony Vroon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xchat-plugin.h"
#include "parse.h"
#include "match.h"
#include "xsys.h"

#define DEFAULT_FORMAT "%B%1:%B %2 **"
#define DEFAULT_PERCENTAGES 1
#define DEFAULT_PCIIDS "/usr/share/hwdata/pci.ids"

static xchat_plugin *ph;

static char name[] = "SysInfo";
static char desc[] = "Display info about your hardware and OS";
static char version[] = "2.2";

void
sysinfo_get_pciids (char* dest)
{
	xchat_pluginpref_get_str (ph, "pciids", dest);
}

int
sysinfo_get_percentages ()
{
	return xchat_pluginpref_get_int (ph, "percentages");
}

static int
print_summary (int announce, char* format)
{
	char sysinfo[bsize];
	char buffer[bsize];
	char cpu_model[bsize];
	char cpu_cache[bsize];
	char cpu_vendor[bsize];
	char os_host[bsize];
	char os_user[bsize];
	char os_kernel[bsize];
	unsigned long long mem_total;
	unsigned long long mem_free;
	unsigned int count;
	double cpu_freq;
	int giga = 0;
	int weeks;
	int days;
	int hours;
	int minutes;
	int seconds;
	sysinfo[0] = '\0';

	snprintf (buffer, bsize, "%s", xchat_get_info (ph, "version"));
	format_output ("HexChat", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN OS PARSING */
	if (xs_parse_os (os_user, os_host, os_kernel) != 0)
	{
		xchat_printf (ph, "ERROR in parse_os()");
		return XCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s", os_kernel);
	format_output ("OS", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN DISTRO PARSING */
        if (xs_parse_distro (buffer) != 0)
        {
		strncpy (buffer, "Unknown", bsize);
	}

	format_output ("Distro", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));	

	/* BEGIN CPU PARSING */
	if (xs_parse_cpu (cpu_model, cpu_vendor, &cpu_freq, cpu_cache, &count) != 0)
	{
		xchat_printf (ph, "ERROR in parse_cpu()");
		return XCHAT_EAT_ALL;
	}

	if (cpu_freq > 1000)
	{
		cpu_freq /= 1000;
		giga = 1;
	}

	if (giga)
	{
		snprintf (buffer, bsize, "%d x %s (%s) @ %.2fGHz", count, cpu_model, cpu_vendor, cpu_freq);
	}
	else
	{
		snprintf (buffer, bsize, "%d x %s (%s) @ %.0fMHz", count, cpu_model, cpu_vendor, cpu_freq);
	}

	format_output ("CPU", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN MEMORY PARSING */
	if (xs_parse_meminfo (&mem_total, &mem_free, 0) == 1)
	{
		xchat_printf (ph, "ERROR in parse_meminfo!");
		return XCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s", pretty_freespace ("Physical", &mem_free, &mem_total));
	format_output ("RAM", buffer, format);	
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (sysinfo));

	/* BEGIN DISK PARSING */
	if (xs_parse_df (NULL, buffer))
	{
		xchat_printf (ph, "ERROR in parse_df");
		return XCHAT_EAT_ALL;
	}

	format_output ("Disk", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN VIDEO PARSING */
	if (xs_parse_video (buffer))
	{
		xchat_printf (ph, "ERROR in parse_video");
		return XCHAT_EAT_ALL;
	}

	format_output ("VGA", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN SOUND PARSING */
	if (xs_parse_sound (buffer))
	{
		strncpy (buffer, "Not present", bsize);
	}

	format_output ("Sound", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN ETHERNET PARSING */
	if (xs_parse_ether (buffer))
	{
		strncpy (buffer, "None found", bsize);
	}
	format_output ("Ethernet", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	/* BEGIN UPTIME PARSING */
	if (xs_parse_uptime (&weeks, &days, &hours, &minutes, &seconds))
	{
		xchat_printf (ph, "ERROR in parse_uptime()");
		return XCHAT_EAT_ALL;
	}

	if (minutes != 0 || hours != 0 || days != 0 || weeks != 0)
	{
		if (hours != 0 || days != 0 || weeks != 0)
		{
			if (days  !=0 || weeks != 0)
			{
				if (weeks != 0)
				{
					snprintf (buffer, bsize, "%dw %dd %dh %dm %ds", weeks, days, hours, minutes, seconds);
				}
				else
				{
					snprintf (buffer, bsize, "%dd %dh %dm %ds", days, hours, minutes, seconds);
				}
			}
			else
			{
				snprintf (buffer, bsize, "%dh %dm %ds", hours, minutes, seconds);
			}
		}
		else
		{
			snprintf (buffer, bsize, "%dm %ds", minutes, seconds);
		}
	}

        format_output ("Uptime", buffer, format);
	strcat (sysinfo, "\017 ");
	strncat (sysinfo, buffer, bsize - strlen (buffer));

	if (announce)
	{
		xchat_commandf (ph, "ME %s", sysinfo);
	}
	else
	{
		xchat_printf (ph, "%s", sysinfo);
	}

	return XCHAT_EAT_ALL;
}

static int
print_os (int announce, char* format)
{
	char buffer[bsize];
	char user[bsize];
	char host[bsize];
	char kernel[bsize];

	if (xs_parse_os (user, host, kernel) != 0)
	{
		xchat_printf (ph, "ERROR in parse_os()");
		return XCHAT_EAT_ALL;
	}

	snprintf (buffer, bsize, "%s@%s, %s", user, host, kernel);
	format_output ("OS", buffer, format);
	
	if (announce)
	{
		xchat_commandf (ph, "ME %s", buffer);
	}
	else
	{
		xchat_printf (ph, "%s", buffer);
	}

	return XCHAT_EAT_ALL;
}

static int
print_distro (int announce, char* format)
{
	char name[bsize];

	if (xs_parse_distro (name) != 0)
	{
		xchat_printf (ph, "ERROR in parse_distro()!");
		return XCHAT_EAT_ALL;
	}

	format_output("Distro", name, format);

	if (announce)
	{
		xchat_commandf (ph, "ME %s", name);
	}
	else
	{
		xchat_printf (ph, "%s", name);
	}
	return XCHAT_EAT_ALL;
}

static int
print_cpu (int announce, char* format)
{
	char model[bsize];
	char vendor[bsize];
	char cache[bsize];
	char buffer[bsize];
	unsigned int count;
	double freq;
	int giga = 0;

	if (xs_parse_cpu (model, vendor, &freq, cache, &count) != 0)
	{
		xchat_printf (ph, "ERROR in parse_cpu()");
		return XCHAT_EAT_ALL;
	}

	if (freq > 1000)
	{
		freq /= 1000;
		giga = 1;
	}

	if (giga)
	{
		snprintf (buffer, bsize, "%d x %s (%s) @ %.2fGHz w/ %s L2 Cache", count, model, vendor, freq, cache);
	}
	else
	{
		snprintf (buffer, bsize, "%d x %s (%s) @ %.0fMHz w/ %s L2 Cache", count, model, vendor, freq, cache);
	}

	format_output ("CPU", buffer, format);

	if (announce)
	{
		xchat_commandf (ph, "ME %s", buffer);
	}
	else
	{
		xchat_printf (ph, "%s", buffer);
	}

	return XCHAT_EAT_ALL;
}

static int
print_ram (int announce, char* format)
{
	unsigned long long mem_total;
	unsigned long long mem_free;
	unsigned long long swap_total;
	unsigned long long swap_free;
	char string[bsize];

	if (xs_parse_meminfo (&mem_total, &mem_free, 0) == 1)
	{
		xchat_printf (ph, "ERROR in parse_meminfo!");
		return XCHAT_EAT_ALL;
	}
	if (xs_parse_meminfo (&swap_total, &swap_free, 1) == 1)
	{
		xchat_printf (ph, "ERROR in parse_meminfo!");
		return XCHAT_EAT_ALL;
	}

	snprintf (string, bsize, "%s - %s", pretty_freespace ("Physical", &mem_free, &mem_total), pretty_freespace ("Swap", &swap_free, &swap_total));
	format_output ("RAM", string, format);
	
	if (announce)
	{
		xchat_commandf (ph, "ME %s", string);
	}
	else
	{
		xchat_printf (ph, "%s", string);
	}
	
	return XCHAT_EAT_ALL;
}

static int
print_disk (int announce, char* format)
{
	char string[bsize] = {0,};

#if 0
	if (*word == '\0')
	{
		if (xs_parse_df (NULL, string))
		{
			xchat_printf (ph, "ERROR in parse_df");
			return XCHAT_EAT_ALL;
		}
	}
	else
	{
		if (xs_parse_df (*word, string))
		{
			xchat_printf (ph, "ERROR in parse_df");
			return XCHAT_EAT_ALL;
		}
	}
#endif

	if (xs_parse_df (NULL, string))
	{
		xchat_printf (ph, "ERROR in parse_df");
		return XCHAT_EAT_ALL;
	}

	format_output ("Disk", string, format);

	if (announce)
	{
		xchat_commandf (ph, "ME %s", string);
	}
	else
	{
		xchat_printf (ph, "%s", string);
	}

	return XCHAT_EAT_ALL;
}

static int
print_vga (int announce, char* format)
{
	char vid_card[bsize];
	char agp_bridge[bsize];
	char buffer[bsize];
	int ret;

	if ((ret = xs_parse_video (vid_card)) != 0)
	{
		xchat_printf (ph, "ERROR in parse_video! %d", ret);
		return XCHAT_EAT_ALL;
	}

	if (xs_parse_agpbridge (agp_bridge) != 0)
	{
		snprintf (buffer, bsize, "%s", vid_card);
	}
	else
	{
		snprintf (buffer, bsize, "%s @ %s", vid_card, agp_bridge);
	}

	format_output ("VGA", buffer, format);

	if (announce)
	{
		xchat_commandf (ph, "ME %s", buffer);
	}
	else
	{
		xchat_printf (ph, "%s", buffer);
	}

	return XCHAT_EAT_ALL;
}

static int
print_sound (int announce, char* format)
{
	char sound[bsize];

	if (xs_parse_sound (sound) != 0)
	{
		xchat_printf (ph, "ERROR in parse_asound()!");
		return XCHAT_EAT_ALL;
	}

	format_output ("Sound", sound, format);

	if (announce)
	{
		xchat_commandf (ph, "ME %s", sound);
	}
	else
	{
		xchat_printf (ph, "%s", sound);
	}

	return XCHAT_EAT_ALL;
}


static int
print_ethernet (int announce, char* format)
{
	char ethernet_card[bsize];

	if (xs_parse_ether (ethernet_card))
	{
		strncpy (ethernet_card, "None found", bsize);
	}

	format_output ("Ethernet", ethernet_card, format);

	if (announce)
	{
		xchat_commandf(ph, "ME %s", ethernet_card);
	}
	else
	{
		xchat_printf(ph, "%s", ethernet_card);
	}

	return XCHAT_EAT_ALL;
}

static int
print_uptime (int announce, char* format)
{
	char buffer[bsize];
	int weeks;
	int days;
	int hours;
	int minutes;
	int seconds;

	if (xs_parse_uptime (&weeks, &days, &hours, &minutes, &seconds))
	{
		xchat_printf (ph, "ERROR in parse_uptime()");
		return XCHAT_EAT_ALL;
	}

	if (minutes != 0 || hours != 0 || days != 0 || weeks != 0)
	{
		if (hours != 0 || days != 0 || weeks != 0)
		{
			if (days  !=0 || weeks != 0)
			{
				if (weeks != 0)
				{
					snprintf (buffer, bsize, "%dw %dd %dh %dm %ds", weeks, days, hours, minutes, seconds);
				}
				else
				{
					snprintf (buffer, bsize, "%dd %dh %dm %ds", days, hours, minutes, seconds);
				}
			}
			else
			{
				snprintf (buffer, bsize, "%dh %dm %ds", hours, minutes, seconds);
			}
		}
		else
		{
			snprintf (buffer, bsize, "%dm %ds", minutes, seconds);
		}
	}

	format_output ("Uptime", buffer, format);

	if (announce)
	{
		xchat_commandf (ph, "ME %s", buffer);
	}
	else
	{
		xchat_printf (ph, "%s", buffer);
	}

	return XCHAT_EAT_ALL;
}

static int
netdata_cb (char *word[], char *word_eol[], void *userdata)
{
	char netdata[bsize];
	char format[bsize];
	unsigned long long bytes_recv;
	unsigned long long bytes_sent;
	
	if (*word[2] == '\0')
	{
		xchat_printf (ph, "You must specify a network device! (eg.: /netdata eth0)");
		return XCHAT_EAT_ALL;
	}

	if (xs_parse_netdev (word[2], &bytes_recv, &bytes_sent) != 0)
	{
		xchat_printf (ph, "ERROR in parse_netdev");
		return XCHAT_EAT_ALL;
	}

	bytes_recv /= 1024;
	bytes_sent /= 1024;
	
	snprintf (netdata, bsize, "%s: %.1f MB Recieved, %.1f MB Sent", word[2], (double)bytes_recv/1024.0, (double)bytes_sent/1024.0);
	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("Netdata", netdata, format);

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", netdata);
	}
	else
	{
		xchat_commandf (ph, "say %s", netdata);
	}
	
	return XCHAT_EAT_ALL;
}

static int
netstream_cb (char *word[], char *word_eol[], void *userdata)
{
	char netstream[bsize];
	char mag_r[3];
	char mag_s[5];
	char format[bsize];
	unsigned long long bytes_recv;
	unsigned long long bytes_sent;
	unsigned long long bytes_recv_p;
	unsigned long long bytes_sent_p;

	struct timespec ts = {1, 0};

	if (*word[2] == '\0')
	{
		xchat_printf (ph, "You must specify a network device! (eg.: /netstream eth0)");
		return XCHAT_EAT_ALL;
	}

	if (xs_parse_netdev(word[2], &bytes_recv, &bytes_sent) != 0)
	{
		xchat_printf (ph, "ERROR in parse_netdev");
		return XCHAT_EAT_ALL;
	}

	while (nanosleep (&ts, &ts) < 0);

	if (xs_parse_netdev(word[2], &bytes_recv_p, &bytes_sent_p) != 0)
	{
		xchat_printf (ph, "ERROR in parse_netdev");
		return XCHAT_EAT_ALL;
	}

	bytes_recv = (bytes_recv_p - bytes_recv);
	bytes_sent = (bytes_sent_p - bytes_sent);

	if (bytes_recv > 1024)
	{
		bytes_recv /= 1024;
		snprintf (mag_r, 5, "KB/s");
	}
	else
	{
		snprintf (mag_r, 5, "B/s");
	}

	if (bytes_sent > 1024)
	{
		bytes_sent /= 1024;
		snprintf (mag_s, 5, "KB/s");
	}
	else
	{
		snprintf (mag_s, 5, "B/s");
	}

	snprintf (netstream, bsize, "%s: Receiving %llu %s, Sending %llu %s", word[2], bytes_recv, mag_r, bytes_sent, mag_s);
	xchat_pluginpref_get_str (ph, "format", format);
	format_output ("Netstream", netstream, format);

	if ((long)userdata)
	{
		xchat_printf (ph, "%s", netstream);
	}
	else
	{
		xchat_commandf (ph, "say %s", netstream);
	}

	return XCHAT_EAT_ALL;
}

static int
sysinfo_cb (char *word[], char *word_eol[], void *userdata)
{
	int announce = 0;
	char format[bsize];

	if (!xchat_pluginpref_get_str (ph, "format", format))
	{
		xchat_printf (ph, "Error reading config file!");
		return XCHAT_EAT_ALL;
	}

	if (xchat_list_int (ph, NULL, "type") >= 2)
	{
		announce = 1;
	}

	if (!g_ascii_strcasecmp ("HELP", word[2]))
	{
		xchat_printf (ph, "Usage: /SYSINFO [OS|DISTRO|CPU|RAM|DISK|VGA|SOUND|ETHERNET|UPTIME]\n");
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("OS", word[2]))
	{
		print_os (announce, format);
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("DISTRO", word[2]))
	{
		print_distro (announce, format);
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("CPU", word[2]))
	{
		print_cpu (announce, format);
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("RAM", word[2]))
	{
		print_ram (announce, format);
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("DISK", word[2]))
	{
		print_disk (announce, format);
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("VGA", word[2]))
	{
		print_vga (announce, format);
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("SOUND", word[2]))
	{
		print_sound (announce, format);
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("ETHERNET", word[2]))
	{
		print_ethernet (announce, format);
		return XCHAT_EAT_ALL;
	}
	else if (!g_ascii_strcasecmp ("UPTIME", word[2]))
	{
		print_uptime (announce, format);
		return XCHAT_EAT_ALL;
	}
	else
	{
		print_summary (announce, format);
		return XCHAT_EAT_ALL;
	}
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;
	*plugin_name    = name;
	*plugin_desc    = desc;
	*plugin_version = version;
	char buffer[bsize];

	xchat_hook_command (ph, "SYSINFO",    XCHAT_PRI_NORM, sysinfo_cb,   "Usage: /SYSINFO [OS|DISTRO|CPU|RAM|DISK|VGA|SOUND|ETHERNET|UPTIME]", 0);
	xchat_hook_command (ph, "NETDATA",    XCHAT_PRI_NORM, netdata_cb,   NULL, (void *) 0);
	xchat_hook_command (ph, "ENETDATA",   XCHAT_PRI_NORM, netdata_cb,   NULL, (void *) 1);
	xchat_hook_command (ph, "NETSTREAM",  XCHAT_PRI_NORM, netstream_cb, NULL, (void *) 0);
	xchat_hook_command (ph, "ENETSTREAM", XCHAT_PRI_NORM, netstream_cb, NULL, (void *) 1);

	/* this is required for the very first run */
	if (xchat_pluginpref_get_str (ph, "pciids", buffer) == 0)
	{
		xchat_pluginpref_set_str (ph, "pciids", DEFAULT_PCIIDS);
	}

	if (xchat_pluginpref_get_str (ph, "format", buffer) == 0)
	{
		xchat_pluginpref_set_str (ph, "format", DEFAULT_FORMAT);
	}

	if (xchat_pluginpref_get_int (ph, "percentages") == -1)
	{
		xchat_pluginpref_set_int (ph, "percentages", DEFAULT_PERCENTAGES);
	}

	xchat_command (ph, "MENU ADD \"Window/Display System Info\" \"SYSINFO\"");
	xchat_printf (ph, "%s plugin loaded\n", name);
	return 1;
}

int
xchat_plugin_deinit (void)
{
	xchat_command (ph, "MENU DEL \"Window/Display System Info\"");
	xchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
