//
// "cupsaccept", "cupsdisable", "cupsenable", and "cupsreject" commands for
// CUPS.
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

static void	usage(const char *command) _CUPS_NORETURN;


//
// 'main()' - Parse options and accept/reject jobs or disable/enable printers.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  char		*command,		// Command to do
		*opt,			// Option pointer
		uri[1024],		// Printer URI
		*reason;		// Reason for reject/disable
  ipp_t		*request;		// IPP request
  ipp_op_t	op;			// Operation
  int		cancel;			// Cancel jobs?


  localize_init(argv);

  // See what operation we're supposed to do...
  if ((command = strrchr(argv[0], '/')) != NULL)
    command ++;
  else
    command = argv[0];

  cancel = 0;

  if (!strcmp(command, "cupsaccept"))
  {
    op = IPP_OP_CUPS_ACCEPT_JOBS;
  }
  else if (!strcmp(command, "cupsreject"))
  {
    op = IPP_OP_CUPS_REJECT_JOBS;
  }
  else if (!strcmp(command, "cupsdisable"))
  {
    op = IPP_OP_PAUSE_PRINTER;
  }
  else if (!strcmp(command, "cupsenable"))
  {
    op = IPP_OP_RESUME_PRINTER;
  }
  else
  {
    cupsLangPrintf(stderr, _("%s: Don't know what to do."), command);
    return (1);
  }

  reason = NULL;

  // Process command-line arguments...
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      usage(command);
    }
    else if (!strcmp(argv[i], "--hold"))
    {
      op = IPP_OP_HOLD_NEW_JOBS;
    }
    else if (!strcmp(argv[i], "--release"))
    {
      op = IPP_OP_RELEASE_HELD_NEW_JOBS;
    }
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'E' : // Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);
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
		  cupsLangPrintf(stderr, _("%s: Error - expected username after \"-U\" option."), command);
		  usage(command);
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'c' : // Cancel jobs
	      cancel = 1;
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
		  cupsLangPrintf(stderr, _("%s: Error - expected hostname after \"-h\" option."), command);
		  usage(command);
		}

		cupsSetServer(argv[i]);
	      }
	      break;

	  case 'r' : // Reason for cancellation
	      if (opt[1] != '\0')
	      {
		reason = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected reason text after \"-r\" option."), command);
		  usage(command);
		}

		reason = argv[i];
	      }
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%c\"."), command, *opt);
	      usage(command);
	}
      }
    }
    else
    {
     /*
      * Accept/disable/enable/reject a destination...
      */

      request = ippNewRequest(op);

      httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 0, "/printers/%s", argv[i]);
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,  "printer-uri", NULL, uri);
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
      if (reason != NULL)
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_TEXT, "printer-state-message", NULL, reason);

      // Do the request and get back a response...
      ippDelete(cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/admin/"));

      if (cupsLastError() > IPP_STATUS_OK_CONFLICTING_ATTRIBUTES)
      {
	cupsLangPrintf(stderr, _("%s: Operation failed: %s"), command, ippErrorString(cupsLastError()));
	return (1);
      }

      // Cancel all jobs if requested...
      if (cancel)
      {
        // Build an Cancel-Jobs request, which requires the following
	// attributes:
	//
	//   attributes-charset
	//   attributes-natural-language
	//   printer-uri
	request = ippNewRequest(IPP_OP_CANCEL_JOBS);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	ippDelete(cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/admin/"));

        if (cupsLastError() > IPP_STATUS_OK_CONFLICTING_ATTRIBUTES)
	{
	  cupsLangPrintf(stderr, "%s: %s", command, cupsLastErrorString());
	  return (1);
	}
      }
    }
  }

  return (0);
}


//
// 'usage()' - Show program usage and exit.
//

static void
usage(const char *command)		// I - Command name
{
  cupsLangPrintf(stdout, _("Usage: %s [options] destination(s)"), command);
  cupsLangPuts(stdout, _("Options:"));
  cupsLangPuts(stdout, _("-E                      Encrypt the connection to the server"));
  cupsLangPuts(stdout, _("-h server[:port]        Connect to the named server and port"));
  cupsLangPuts(stdout, _("-r reason               Specify a reason message that others can see"));
  cupsLangPuts(stdout, _("-U username             Specify the username to use for authentication"));
  if (!strcmp(command, "cupsdisable"))
    cupsLangPuts(stdout, _("--hold                  Hold new jobs"));
  if (!strcmp(command, "cupsenable"))
    cupsLangPuts(stdout, _("--release               Release previously held jobs"));

  exit(1);
}
