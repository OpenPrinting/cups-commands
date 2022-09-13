//
// "lprm" command for CUPS.
//
// Copyright © 2021-2022 by OpenPrinting.
// Copyright © 2007-2018 by Apple Inc.
// Copyright © 1997-2006 by Easy Software Products.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "localize.h"


//
// Local functions...
//

static void	usage(void) _CUPS_NORETURN;


//
// 'main()' - Parse options and cancel jobs.
//

int				// O - Exit status
main(int  argc,			// I - Number of command-line arguments
     char *argv[])		// I - Command-line arguments
{
  int		i;		// Looping var
  int		job_id;		// Job ID
  char		*destname,	// Destination name
		*instance,	// Pointer to instance name
		*opt;		// Option pointer
  cups_dest_t	*dest = NULL,	// Destination
		*defdest;	// Default destination
  bool		did_cancel;	// Did we cancel something?


  localize_init(argv);

 /*
  * Setup to cancel individual print jobs...
  */

  did_cancel = false;
  defdest    = cupsGetNamedDest(CUPS_HTTP_DEFAULT, NULL, NULL);

 /*
  * Process command-line arguments...
  */

  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      usage();
    }
    else if (argv[i][0] == '-' && argv[i][1] != '\0')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'E' : // Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);
	      break;

	  case 'P' : // Cancel jobs on a printer
	      if (opt[1] != '\0')
	      {
		destname = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected destination after \"-P\" option."), argv[0]);
		  usage();
		}

		destname = argv[i];
	      }

	      if ((instance = strchr(destname, '/')) != NULL)
		*instance = '\0';

	      if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, destname, NULL)) == NULL)
	      {
		cupsLangPrintf(stderr, _("%s: Error - unknown destination \"%s\"."), argv[0], destname);
		goto error;
	      }
	      break;

	  case 'U' : // Username
	      if (opt[1] != '\0')
	      {
		cupsSetUser(opt + 1);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected username after \"-U\" option."), argv[0]);
		  usage();
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'h' : // Connect to host
	      if (opt[1] != '\0')
	      {
		cupsSetServer(opt + 1);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected hostname after \"-h\" option."), argv[0]);
		  usage();
		}
		else
		{
		  cupsSetServer(argv[i]);
		}
	      }

	      if (defdest)
		cupsFreeDests(1, defdest);

	      defdest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, NULL, NULL);
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%c\"."), argv[0], *opt);
	      usage();
	}
      }
    }
    else
    {
      // Cancel a job or printer...
      dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, argv[i], NULL);

      if (dest)
      {
        job_id = 0;
      }
      else if (isdigit(argv[i][0] & 255))
      {
        job_id = atoi(argv[i]);
      }
      else if (!strcmp(argv[i], "-"))
      {
        // Cancel all jobs
        job_id = -1;
      }
      else
      {
	cupsLangPrintf(stderr, _("%s: Error - unknown destination \"%s\"."), argv[0], argv[i]);
	goto error;
      }

      if (cupsCancelDestJob(CUPS_HTTP_DEFAULT, dest, job_id) != IPP_STATUS_OK)
      {
        cupsLangPrintf(stderr, "%s: %s", argv[0], cupsLastErrorString());
	goto error;
      }

      did_cancel = true;
    }
  }

  // If nothing has been canceled yet, cancel the current job on the specified
  // (or default) printer...
  if (!did_cancel && cupsCancelDestJob(CUPS_HTTP_DEFAULT, defdest, 0) != IPP_STATUS_OK)
  {
    cupsLangPrintf(stderr, "%s: %s", argv[0], cupsLastErrorString());
    goto error;
  }

  return (0);

  // If we get here there was an error, so clean up...
  error:

  return (1);
}


//
// 'usage()' - Show program usage and exit.
//

static void
usage(void)
{
  cupsLangPuts(stdout, _("Usage: lprm [options] [id]\n"
                          "       lprm [options] -"));
  cupsLangPuts(stdout, _("Options:"));
  cupsLangPuts(stdout, _("-                       Cancel all jobs"));
  cupsLangPuts(stdout, _("-E                      Encrypt the connection to the server"));
  cupsLangPuts(stdout, _("-h server[:port]        Connect to the named server and port"));
  cupsLangPuts(stdout, _("-P destination          Specify the destination"));
  cupsLangPuts(stdout, _("-U username             Specify the username to use for authentication"));

  exit(1);
}
