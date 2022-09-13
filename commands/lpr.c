//
// "lpr" command for CUPS.
//
// Copyright © 2021-2022 by OpenPrinting.
// Copyright © 2007-2019 by Apple Inc.
// Copyright © 1997-2007 by Easy Software Products.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "localize.h"
#include <unistd.h>
#include <fcntl.h>
#ifndef O_BINARY
#  define O_BINARY 0
#endif // !O_BINARY


//
// Local functions...
//

static void	usage(void) _CUPS_NORETURN;


//
// 'main()' - Parse options and send files for printing.
//

int
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i, j;			// Looping var
  int		job_id;			// Job ID
  char		ch;			// Option character
  char		*printer,		// Destination printer or class
		*instance,		// Instance
		*opt;			// Option pointer
  const char	*title;			// Job title
  int		num_copies;		// Number of copies per file
  int		num_files;		// Number of files to print
  const char	*files[1000];		// Files to print
  cups_dest_t	*dest = NULL;		// Selected destination
  cups_dinfo_t	*dinfo;			// Destination information
  int		num_options;		// Number of options
  cups_option_t	*options;		// Options
  bool		deletefile;		// Delete file after print?
  char		buffer[8192];		// Copy buffer
  http_status_t	status;			// Write status
  const char	*format;		// Document format
  ssize_t	bytes;			// Bytes read


  localize_init(argv);

  deletefile  = false;
  printer     = NULL;
  dest        = NULL;
  num_options = 0;
  options     = NULL;
  num_files   = 0;
  title       = NULL;

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
	switch (ch = *opt)
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
		  cupsLangPrintf(stderr, _("%s: Error - expected username after \"-U\" option."), argv[0]);
		  usage();
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'H' : // Connect to host
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
		  cupsLangPrintf(stderr, _("%s: Error - expected hostname after \"-H\" option."), argv[0]);
		  usage();
		}
		else
		{
		  cupsSetServer(argv[i]);
		}
	      }
	      break;

	  case '1' : // TROFF font set 1
	  case '2' : // TROFF font set 2
	  case '3' : // TROFF font set 3
	  case '4' : // TROFF font set 4
	  case 'i' : // indent
	  case 'w' : // width
	      if (opt[1] != '\0')
	      {
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected value after \"-%c\" option."), argv[0], ch);
		  usage();
		}
	      }
	      // Fall through to the other legacy format options...

	  case 'c' : // CIFPLOT
	  case 'd' : // DVI
	  case 'f' : // FORTRAN
	  case 'g' : // plot
	  case 'n' : // Ditroff
	  case 't' : // Troff
	  case 'v' : // Raster image
	      cupsLangPrintf(stderr, _("%s: Warning - \"%c\" format modifier not supported - output may not be correct."), argv[0], ch);
	      break;

	  case 'o' : // Option
	      if (opt[1] != '\0')
	      {
		num_options = cupsParseOptions(opt + 1, num_options, &options);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected option=value after \"-o\" option."), argv[0]);
		  usage();
		}

		num_options = cupsParseOptions(argv[i], num_options, &options);
	      }
	      break;

	  case 'l' : // Literal/raw
	      num_options = cupsAddOption("raw", "true", num_options, &options);
	      break;

	  case 'p' : // Prettyprint
	      num_options = cupsAddOption("prettyprint", "true", num_options, &options);
	      break;

	  case 'h' : // Suppress burst page
	      num_options = cupsAddOption("job-sheets", "none", num_options, &options);
	      break;

	  case 's' : // Don't use symlinks
	      break;

	  case 'm' : // Mail on completion
	      {
		char	email[1024];	// EMail address

		snprintf(email, sizeof(email), "mailto:%s@%s", cupsGetUser(), httpGetHostname(NULL, buffer, sizeof(buffer)));
		num_options = cupsAddOption("notify-recipient-uri", email, num_options, &options);
	      }
	      break;

	  case 'q' : // Queue file but don't print
	      num_options = cupsAddOption("job-hold-until", "indefinite", num_options, &options);
	      break;

	  case 'r' : // Remove file after printing
	      deletefile = true;
	      break;

	  case 'P' : // Destination printer or class
	      if (opt[1] != '\0')
	      {
		printer = opt + 1;
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

		printer = argv[i];
	      }

	      if ((instance = strrchr(printer, '/')) != NULL)
		*instance++ = '\0';

	      if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer, instance)) != NULL)
	      {
		for (j = 0; j < dest->num_options; j ++)
		{
		  if (cupsGetOption(dest->options[j].name, num_options, options) == NULL)
		    num_options = cupsAddOption(dest->options[j].name, dest->options[j].value, num_options, &options);
		}
	      }
	      else if (cupsLastError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsLastError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
	      {
		cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
		return (1);
	      }
	      else if (cupsLastError() == IPP_STATUS_ERROR_NOT_FOUND)
	      {
		cupsLangPrintf(stderr, _("%s: Error - The printer or class does not exist."), argv[0]);
		return (1);
	      }
	      break;

	  case '#' : // Number of copies
	      if (opt[1] != '\0')
	      {
		num_copies = atoi(opt + 1);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected copies after \"-#\" option."), argv[0]);
		  usage();
		}

		num_copies = atoi(argv[i]);
	      }

	      if (num_copies < 1)
	      {
		cupsLangPrintf(stderr, _("%s: Error - copies must be 1 or more."), argv[0]);
		return (1);
	      }

	      num_options = cupsAddIntegerOption("copies", num_copies, num_options, &options);
	      break;

	  case 'C' : // Class
	  case 'J' : // Job name
	  case 'T' : // Title
	      if (opt[1] != '\0')
	      {
		title = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected name after \"-%c\" option."), argv[0], ch);
		  usage();
		}

		title = argv[i];
	      }
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%c\"."), argv[0], *opt);
	      return (1);
	}
      }
    }
    else if (num_files < 1000)
    {
      // Print a file...
      if (access(argv[i], R_OK) != 0)
      {
        cupsLangPrintf(stderr, _("%s: Error - unable to access \"%s\" - %s"), argv[0], argv[i], strerror(errno));
        return (1);
      }

      files[num_files] = argv[i];
      num_files ++;

      if (title == NULL)
      {
        if ((title = strrchr(argv[i], '/')) != NULL)
	  title ++;
	else
          title = argv[i];
      }
    }
    else
    {
      cupsLangPrintf(stderr, _("%s: Error - too many files - \"%s\"."), argv[0], argv[i]);
    }
  }

  // Get the destination...
  if (!dest)
  {
    if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, /*printer*/NULL, /*instance*/NULL)) != NULL)
    {
      for (j = 0; j < dest->num_options; j ++)
      {
	if (cupsGetOption(dest->options[j].name, num_options, options) == NULL)
	  num_options = cupsAddOption(dest->options[j].name, dest->options[j].value, num_options, &options);
      }
    }
    else if (cupsLastError() == IPP_STATUS_ERROR_NOT_FOUND)
    {
      cupsLangPrintf(stderr, _("%s: Error - %s"), argv[0], cupsLastErrorString());
      return (1);
    }
    else if (cupsLastError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsLastError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
    {
      cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
      return (1);
    }
    else
    {
      cupsLangPrintf(stderr, _("%s: Error - scheduler not responding."), argv[0]);
      return (1);
    }
  }

  dinfo = cupsCopyDestInfo(CUPS_HTTP_DEFAULT, dest);

  // Title...
  if (!title)
  {
    if (num_files == 0)
    {
      title = "(stdin)";
    }
    else if (num_files == 1)
    {
      if ((title = strrchr(files[0], '/')) != NULL)
        title ++;
    }
  }

  // Create the job...
  if (cupsCreateDestJob(CUPS_HTTP_DEFAULT, dest, dinfo, &job_id, title, num_options, options) > IPP_STATUS_OK_EVENTS_COMPLETE)
  {
    cupsLangPrintf(stderr, "%s: %s", argv[0], cupsLastErrorString());
    return (1);
  }

  if (cupsGetOption("raw", num_options, options))
    format = CUPS_FORMAT_RAW;
  else if ((format = cupsGetOption("document-format", num_options, options)) == NULL)
    format = CUPS_FORMAT_AUTO;

  // See if we have any files to print; if not, print from stdin...
  if (num_files > 0)
  {
    // Print file(s)...
    for (i = 0; i < num_files; i ++)
    {
      int	fd;			// File descriptor
      const char *docname;		// Document name

      if ((fd = open(files[i], O_RDONLY | O_BINARY)) < 0)
      {
        cupsLangPrintf(stderr, _("%s: Unable to open \"%s\" - %s"), argv[0], files[i], strerror(errno));
	cupsCancelDestJob(CUPS_HTTP_DEFAULT, dest, job_id);
	return (1);
      }

      if ((docname = strrchr(files[i], '/')) != NULL)
        docname ++;
      else
        docname = files[i];

      status = cupsStartDestDocument(CUPS_HTTP_DEFAULT, dest, dinfo, job_id, docname, format, /*num_options*/0, /*options*/NULL, (i + 1) == num_files);

      while (status == HTTP_STATUS_CONTINUE && (bytes = read(fd, buffer, sizeof(buffer))) > 0)
	status = cupsWriteRequestData(CUPS_HTTP_DEFAULT, buffer, (size_t)bytes);

      close(fd);

      if (status != HTTP_STATUS_CONTINUE)
      {
	cupsLangPrintf(stderr, _("%s: Error - unable to queue %s - %s."), argv[0], files[i], httpStatusString(status));
	cupsFinishDestDocument(CUPS_HTTP_DEFAULT, dest, dinfo);
	cupsCancelDestJob(CUPS_HTTP_DEFAULT, dest, job_id);
	return (1);
      }

      if (cupsFinishDestDocument(CUPS_HTTP_DEFAULT, dest, dinfo) != IPP_STATUS_OK)
      {
	cupsLangPrintf(stderr, "%s: %s", argv[0], cupsLastErrorString());
	cupsCancelDestJob(CUPS_HTTP_DEFAULT, dest, job_id);
	return (1);
      }
    }

    if (deletefile && job_id > 0)
    {
      // Delete print files after printing...
      for (i = 0; i < num_files; i ++)
        unlink(files[i]);
    }
  }
  else
  {
    // Print stdin...
    status = cupsStartDestDocument(CUPS_HTTP_DEFAULT, dest, dinfo, job_id, "(stdin)", format, /*num_options*/0, /*options*/NULL, true);

    while (status == HTTP_STATUS_CONTINUE && (bytes = read(0, buffer, sizeof(buffer))) > 0)
      status = cupsWriteRequestData(CUPS_HTTP_DEFAULT, buffer, (size_t)bytes);

    if (status != HTTP_STATUS_CONTINUE)
    {
      cupsLangPrintf(stderr, _("%s: Error - unable to queue %s - %s."), argv[0], "(stdin)", httpStatusString(status));
      cupsFinishDestDocument(CUPS_HTTP_DEFAULT, dest, dinfo);
      cupsCancelDestJob(CUPS_HTTP_DEFAULT, dest, job_id);
      return (1);
    }

    if (cupsFinishDestDocument(CUPS_HTTP_DEFAULT, dest, dinfo) != IPP_STATUS_OK)
    {
      cupsLangPrintf(stderr, "%s: %s", argv[0], cupsLastErrorString());
      cupsCancelDestJob(CUPS_HTTP_DEFAULT, dest, job_id);
      return (1);
    }
  }

  if (job_id < 1)
  {
    cupsLangPrintf(stderr, "%s: %s", argv[0], cupsLastErrorString());
    return (1);
  }

  return (0);
}


//
// 'usage()' - Show program usage and exit.
//

static void
usage(void)
{
  cupsLangPuts(stdout, _("Usage: lpr [options] [file(s)]"));
  cupsLangPuts(stdout, _("Options:"));
  cupsLangPuts(stdout, _("-# num-copies           Specify the number of copies to print"));
  cupsLangPuts(stdout, _("-E                      Encrypt the connection to the server"));
  cupsLangPuts(stdout, _("-H server[:port]        Connect to the named server and port"));
  cupsLangPuts(stdout, _("-m                      Send an email notification when the job completes"));
  cupsLangPuts(stdout, _("-o option[=value]       Specify a printer-specific option"));
  cupsLangPuts(stdout, _("-o job-sheets=standard  Print a banner page with the job"));
  cupsLangPuts(stdout, _("-o media=size           Specify the media size to use"));
  cupsLangPuts(stdout, _("-o number-up=N          Specify that input pages should be printed N-up (1, 2, 4, 6, 9, and 16 are supported)"));
  cupsLangPuts(stdout, _("-o orientation-requested=N\n"
                          "                        Specify portrait (3) or landscape (4) orientation"));
  cupsLangPuts(stdout, _("-o print-quality=N      Specify the print quality - draft (3), normal (4), or best (5)"));
  cupsLangPuts(stdout, _("-o sides=one-sided      Specify 1-sided printing"));
  cupsLangPuts(stdout, _("-o sides=two-sided-long-edge\n"
                          "                        Specify 2-sided portrait printing"));
  cupsLangPuts(stdout, _("-o sides=two-sided-short-edge\n"
                          "                        Specify 2-sided landscape printing"));
  cupsLangPuts(stdout, _("-P destination          Specify the destination"));
  cupsLangPuts(stdout, _("-q                      Specify the job should be held for printing"));
  cupsLangPuts(stdout, _("-r                      Remove the file(s) after submission"));
  cupsLangPuts(stdout, _("-T title                Specify the job title"));
  cupsLangPuts(stdout, _("-U username             Specify the username to use for authentication"));

  exit(1);
}
