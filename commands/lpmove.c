//
// "lpmove" command for CUPS.
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

static int	move_job(http_t *http, const char *src, int jobid, const char *dest);
static void	usage(void) _CUPS_NORETURN;


//
// 'main()' - Parse options and show status information.
//

int
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  http_t	*http;			// Connection to server
  const char	*opt,			// Option pointer
		*job;			// Job name
  int		jobid;			// Job ID
  size_t	num_dests;		// Number of destinations
  cups_dest_t	*dests;			// Destinations
  const char	*src,			// Original queue
		*dest;			// New destination


  localize_init(argv);

  dest      = NULL;
  dests     = NULL;
  job       = NULL;
  jobid     = 0;
  num_dests = 0;
  src       = NULL;

  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      usage();
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
		  cupsLangPuts(stderr, _("Error: need hostname after \"-h\" option."));
		  usage();
		}

		cupsSetServer(argv[i]);
	      }
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Unknown option \"%c\"."), argv[0], *opt);
	      usage();
	}
      }
    }
    else if (!jobid && !src)
    {
      if (num_dests == 0)
        num_dests = cupsGetDests(CUPS_HTTP_DEFAULT, &dests);

      if ((job = strrchr(argv[i], '-')) != NULL && cupsGetDest(argv[i], NULL, num_dests, dests) == NULL)
        jobid = atoi(job + 1);
      else if (isdigit(argv[i][0] & 255) && !cupsGetDest(argv[i], NULL, num_dests, dests))
        jobid = atoi(argv[i]);
      else
        src = argv[i];
    }
    else if (dest == NULL)
    {
      dest = argv[i];
    }
    else
    {
      cupsLangPrintf(stderr, _("lpmove: Unknown argument \"%s\"."), argv[i]);
      usage();
    }
  }

  if ((!jobid && !src) || !dest)
    usage();

  http = httpConnect(cupsGetServer(), ippGetPort(), /*addrlist*/NULL, AF_UNSPEC, cupsGetEncryption(), /*blocking*/true, /*msec*/30000, /*cancel*/NULL);

  if (http == NULL)
  {
    cupsLangPrintf(stderr, _("lpmove: Unable to connect to server: %s"), strerror(errno));
    return (1);
  }

  return (move_job(http, src, jobid, dest));
}


//
// 'move_job()' - Move a job.
//

static int				// O - 0 on success, 1 on error
move_job(http_t     *http,		// I - HTTP connection to server
         const char *src,		// I - Source queue
         int        jobid,		// I - Job ID
	 const char *dest)		// I - Destination queue
{
  ipp_t	*request;			// IPP Request
  char	job_uri[HTTP_MAX_URI],		// job-uri
	printer_uri[HTTP_MAX_URI];	// job-printer-uri


  if (!http)
    return (1);

  // Build a CUPS-Move-Job request, which requires the following
  // attributes:
  //
  //   attributes-charset
  //   attributes-natural-language
  //   job-uri/printer-uri
  //   job-printer-uri
  //   requesting-user-name
  request = ippNewRequest(IPP_OP_CUPS_MOVE_JOB);

  if (jobid)
  {
    snprintf(job_uri, sizeof(job_uri), "ipp://localhost/jobs/%d", jobid);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, job_uri);
  }
  else
  {
    httpAssembleURIf(HTTP_URI_CODING_ALL, job_uri, sizeof(job_uri), "ipp", NULL, "localhost", 0, "/printers/%s", src);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, job_uri);
  }

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",  NULL, cupsGetUser());

  httpAssembleURIf(HTTP_URI_CODING_ALL, printer_uri, sizeof(printer_uri), "ipp", NULL, "localhost", 0, "/printers/%s", dest);
  ippAddString(request, IPP_TAG_JOB, IPP_TAG_URI, "job-printer-uri", NULL, printer_uri);

  // Do the request and get back a response...
  ippDelete(cupsDoRequest(http, request, "/jobs"));

  if (cupsLastError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, "lpmove: %s", cupsLastErrorString());
    return (1);
  }
  else
  {
    return (0);
  }
}


//
// 'usage()' - Show program usage and exit.
//

static void
usage(void)
{
  cupsLangPuts(stdout, _("Usage: lpmove [options] job destination\n"
                         "       lpmove [options] source-destination destination"));
  cupsLangPuts(stdout, _("Options:"));
  cupsLangPuts(stdout, _("-E                      Encrypt the connection to the server"));
  cupsLangPuts(stdout, _("-h server[:port]        Connect to the named server and port"));
  cupsLangPuts(stdout, _("-U username             Specify the username to use for authentication"));

  exit(1);
}
