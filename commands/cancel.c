//
// "cancel" command for CUPS.
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

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  http_t	*http;			// HTTP connection to server
  int		i;			// Looping var
  int		job_id;			// Job ID
  size_t	num_dests;		// Number of destinations
  cups_dest_t	*dests;			// Destinations
  char		*opt,			// Option pointer
		*dest,			// Destination printer
		*job,			// Job ID pointer
		*user;			// Cancel jobs for a user
  bool		purge;			// Purge or cancel jobs?
  char		uri[1024];		// Printer or job URI
  ipp_t		*request;		// IPP request
  ipp_t		*response;		// IPP response
  ipp_op_t	op;			// Operation


  localize_init(argv);

 /*
  * Setup to cancel individual print jobs...
  */

  op        = IPP_OP_CANCEL_JOB;
  purge     = false;
  dest      = NULL;
  user      = NULL;
  http      = NULL;
  num_dests = 0;
  dests     = NULL;


 /*
  * Process command-line arguments...
  */

  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      usage();
    }
    else if (argv[i][0] == '-' && argv[i][1])
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'E' : // Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);

	      if (http)
		httpSetEncryption(http, HTTP_ENCRYPTION_REQUIRED);
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

	  case 'a' : // Cancel all jobs
	      op = purge ? IPP_OP_PURGE_JOBS : IPP_OP_CANCEL_JOBS;
	      break;

	  case 'h' : // Connect to host
	      if (http != NULL)
	      {
		httpClose(http);
		http = NULL;
	      }

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
	      break;

	  case 'u' : // Username
	      op = IPP_OP_CANCEL_MY_JOBS;

	      if (opt[1] != '\0')
	      {
		user = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected username after \"-u\" option."), argv[0]);
		  usage();
		}
		else
		{
		  user = argv[i];
		}
	      }
	      break;

	  case 'x' : // Purge job(s)
	      purge = true;

	      if (op == IPP_OP_CANCEL_JOBS)
		op = IPP_OP_PURGE_JOBS;
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%c\"."), argv[0], *opt);
	      return (1);
	}
      }
    }
    else
    {
      // Cancel a job or printer...
      if (num_dests == 0)
        num_dests = cupsGetDests(http, &dests);

      if (!strcmp(argv[i], "-"))
      {
        // Delete the current job...
        dest   = "";
	job_id = 0;
      }
      else if (cupsGetDest(argv[i], NULL, num_dests, dests) != NULL)
      {
        // Delete the current job on the named destination...
        dest   = argv[i];
	job_id = 0;
      }
      else if ((job = strrchr(argv[i], '-')) != NULL && isdigit(job[1] & 255))
      {
        // Delete the specified job ID.
        dest   = NULL;
	op     = IPP_OP_CANCEL_JOB;
        job_id = atoi(job + 1);
      }
      else if (isdigit(argv[i][0] & 255))
      {
        // Delete the specified job ID.
        dest   = NULL;
	op     = IPP_OP_CANCEL_JOB;
        job_id = atoi(argv[i]);
      }
      else
      {
        // Bad printer name!
        cupsLangPrintf(stderr, _("%s: Error - unknown destination \"%s\"."), argv[0], argv[i]);
	return (1);
      }

      // For Solaris LP compatibility, ignore a destination name after
      // cancelling a specific job ID...
      if (job_id && (i + 1) < argc && cupsGetDest(argv[i + 1], NULL, num_dests, dests) != NULL)
        i ++;

      // Open a connection to the server...
      if (http == NULL)
      {
	if ((http = httpConnect(cupsGetServer(), ippGetPort(), /*addrlist*/NULL, AF_UNSPEC, cupsGetEncryption(), /*blocking*/true, /*msec*/30000, /*cancel*/NULL)) == NULL)
	{
	  cupsLangPrintf(stderr, _("%s: Unable to connect to server."), argv[0]);
	  return (1);
	}
      }

      // Build an IPP request, which requires the following
      // attributes:
      //
      //   attributes-charset
      //   attributes-natural-language
      //   printer-uri + job-id *or* job-uri
      //   [requesting-user-name]
      request = ippNewRequest(op);

      if (dest)
      {
	httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 0, "/printers/%s", dest);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
	ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", job_id);
      }
      else
      {
        snprintf(uri, sizeof(uri), "ipp://localhost/jobs/%d", job_id);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);
      }

      if (user)
      {
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, user);
	ippAddBoolean(request, IPP_TAG_OPERATION, "my-jobs", true);

        if (op == IPP_OP_CANCEL_JOBS)
          op = IPP_OP_CANCEL_MY_JOBS;
      }
      else
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

      if (purge)
	ippAddBoolean(request, IPP_TAG_OPERATION, "purge-jobs", purge);

      // Do the request and get back a response...
      if (op == IPP_OP_CANCEL_JOBS && (!user || strcasecmp(user, cupsGetUser())))
        response = cupsDoRequest(http, request, "/admin/");
      else
        response = cupsDoRequest(http, request, "/jobs/");

      if (response == NULL || ippGetStatusCode(response) > IPP_STATUS_OK_CONFLICTING_ATTRIBUTES)
      {
        if (op == IPP_OP_PURGE_JOBS)
	  cupsLangPrintf(stderr, _("%s: purge-jobs failed: %s"), argv[0], cupsLastErrorString());
        else if (op == IPP_OP_CANCEL_JOB)
	  cupsLangPrintf(stderr, _("%s: cancel-job failed: %s"), argv[0], cupsLastErrorString());
	else
	  cupsLangPrintf(stderr, _("%s: cancel-jobs failed: %s"), argv[0], cupsLastErrorString());

	ippDelete(response);

	return (1);
      }

      ippDelete(response);
    }
  }

  if (num_dests == 0 && op != IPP_OP_CANCEL_JOB)
  {
    // Open a connection to the server...
    if (http == NULL)
    {
      if ((http = httpConnect(cupsGetServer(), ippGetPort(), /*addrlist*/NULL, AF_UNSPEC, cupsGetEncryption(), /*blocking*/true, /*msec*/30000, /*cancel*/NULL)) == NULL)
      {
	cupsLangPrintf(stderr, _("%s: Unable to contact server."), argv[0]);
	return (1);
      }
    }

    // Build an IPP request, which requires the following
    // attributes:
    //
    //   attributes-charset
    //   attributes-natural-language
    //   printer-uri + job-id *or* job-uri
    //   [requesting-user-name]
    request = ippNewRequest(op);

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, "ipp://localhost/printers/");

    if (user)
    {
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, user);
      ippAddBoolean(request, IPP_TAG_OPERATION, "my-jobs", true);
    }
    else
    {
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
    }

    ippAddBoolean(request, IPP_TAG_OPERATION, "purge-jobs", purge);

    // Do the request and get back a response...
    response = cupsDoRequest(http, request, "/admin/");

    if (response == NULL || ippGetStatusCode(response) > IPP_STATUS_OK_CONFLICTING_ATTRIBUTES)
    {
      if (op == IPP_OP_PURGE_JOBS)
	cupsLangPrintf(stderr, _("%s: purge-jobs failed: %s"), argv[0], cupsLastErrorString());
      else if (op == IPP_OP_CANCEL_JOB)
	cupsLangPrintf(stderr, _("%s: cancel-job failed: %s"), argv[0], cupsLastErrorString());
      else
	cupsLangPrintf(stderr, _("%s: cancel-jobs failed: %s"), argv[0], cupsLastErrorString());

      ippDelete(response);

      return (1);
    }

    ippDelete(response);
  }

  return (0);
}


//
// 'usage()' - Show program usage and exit.
//

static void
usage(void)
{
  cupsLangPuts(stdout, _("Usage: cancel [options] [id]\n"
                         "       cancel [options] [destination]\n"
                         "       cancel [options] [destination-id]"));
  cupsLangPuts(stdout, _("Options:"));
  cupsLangPuts(stdout, _("-a                      Cancel all jobs"));
  cupsLangPuts(stdout, _("-E                      Encrypt the connection to the server"));
  cupsLangPuts(stdout, _("-h server[:port]        Connect to the named server and port"));
  cupsLangPuts(stdout, _("-u owner                Specify the owner to use for jobs"));
  cupsLangPuts(stdout, _("-U username             Specify the username to use for authentication"));
  cupsLangPuts(stdout, _("-x                      Purge jobs rather than just canceling"));

  exit(1);
}
