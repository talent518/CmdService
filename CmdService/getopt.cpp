#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "getopt.h"

#define OPTERRCOLON 1
#define OPTERRNF 2
#define OPTERRARG 3

static char *memnstr(char *haystack, char *needle, int needle_len, char *end)
{
	char *p = haystack;
	char ne = needle[needle_len-1];

	if (needle_len == 1) {
		return (char *)memchr(p, *needle, (end-p));
	}

	if (needle_len > end-haystack) {
		return NULL;
	}

	end -= needle_len;

	while (p <= end) {
		if ((p = (char *)memchr(p, *needle, (end-p+1))) && ne == p[needle_len-1]) {
			if (!memcmp(needle, p, needle_len-1)) {
				return p;
			}
		}

		if (p == NULL) {
			return NULL;
		}

		p++;
	}

	return NULL;
}

static int opt_error(int argc, char * const *argv, int oint, int optchr, int err, int show_err) /* {{{ */
{
	if (show_err)
	{
		fprintf(stderr, "Error in argument %d, char %d: ", oint, optchr+1);
		switch(err)
		{
		case OPTERRCOLON:
			fprintf(stderr, ": in flags\n");
			break;
		case OPTERRNF:
			fprintf(stderr, "option not found %c\n", argv[oint][optchr]);
			break;
		case OPTERRARG:
			fprintf(stderr, "no argument for option %c\n", argv[oint][optchr]);
			break;
		default:
			fprintf(stderr, "unknown\n");
			break;
		}
	}
	return('?');
}
/* }}} */

int optidx = -1;

int getopt(int argc, char* const *argv, const opt_struct opts[], char **optarg, int *optind, int show_err, int arg_start) /* {{{ */
{
	static int optchr = 0;
	static int dash = 0; /* have already seen the - */

	optidx = -1;

	if (*optind >= argc) {
		return(EOF);
	}
	if (!dash) {
		if ((argv[*optind][0] !=  '-')) {
			return(EOF);
		} else {
			if (!argv[*optind][1])
			{
				/*
				* use to specify stdin. Need to let pgm process this and
				* the following args
				*/
				return(EOF);
			}
		}
	}
	if ((argv[*optind][0] == '-') && (argv[*optind][1] == '-')) {
		char *pos;
		int arg_end = strlen(argv[*optind])-1;

		/* '--' indicates end of args if not followed by a known long option name */
		if (argv[*optind][2] == '\0') {
			(*optind)++;
			return(EOF);
		}

		arg_start = 2;

		/* Check for <arg>=<val> */
		if ((pos = memnstr(&argv[*optind][arg_start], "=", 1, argv[*optind]+arg_end)) != NULL) {
			arg_end = pos-&argv[*optind][arg_start];
			arg_start++;
		} else {
			arg_end--;
		}

		while (1) {
			optidx++;
			if (opts[optidx].opt_char == '-') {
				(*optind)++;
				return(opt_error(argc, argv, *optind-1, optchr, OPTERRARG, show_err));
			} else if (opts[optidx].opt_name && !strncmp(&argv[*optind][2], opts[optidx].opt_name, arg_end) && arg_end == strlen(opts[optidx].opt_name)) {
				break;
			}
		}

		optchr = 0;
		dash = 0;
		arg_start += strlen(opts[optidx].opt_name);
	} else {
		if (!dash) {
			dash = 1;
			optchr = 1;
		}
		/* Check if the guy tries to do a -: kind of flag */
		if (argv[*optind][optchr] == ':') {
			dash = 0;
			(*optind)++;
			return (opt_error(argc, argv, *optind-1, optchr, OPTERRCOLON, show_err));
		}
		arg_start = 1 + optchr;
	}
	if (optidx < 0) {
		while (1) {
			optidx++;
			if (opts[optidx].opt_char == '-') {
				int errind = *optind;
				int errchr = optchr;

				if (!argv[*optind][optchr+1]) {
					dash = 0;
					(*optind)++;
				} else {
					optchr++;
					arg_start++;
				}
				return(opt_error(argc, argv, errind, errchr, OPTERRNF, show_err));
			} else if (argv[*optind][optchr] == opts[optidx].opt_char) {
				break;
			}
		}
	}
	if (opts[optidx].need_param) {
		/* Check for cases where the value of the argument
		is in the form -<arg> <val>, -<arg>=<varl> or -<arg><val> */
		dash = 0;
		if (!argv[*optind][arg_start]) {
			(*optind)++;
			if (*optind == argc) {
				/* Was the value required or is it optional? */
				if (opts[optidx].need_param == 1) {
					return(opt_error(argc, argv, *optind-1, optchr, OPTERRARG, show_err));
				}
			/* Optional value is not supported with -<arg> <val> style */
			} else if (opts[optidx].need_param == 1) {
				*optarg = argv[(*optind)++];
 			}
		} else if (argv[*optind][arg_start] == '=') {
			arg_start++;
			*optarg = &argv[*optind][arg_start];
			(*optind)++;
		} else {
			*optarg = &argv[*optind][arg_start];
			(*optind)++;
		}
		return opts[optidx].opt_char;
	} else {
		/* multiple options specified as one (exclude long opts) */
		if (arg_start >= 2 && !((argv[*optind][0] == '-') && (argv[*optind][1] == '-'))) {
			if (!argv[*optind][optchr+1])
			{
				dash = 0;
				(*optind)++;
			} else {
				optchr++;
			}
		} else {
			(*optind)++;
		}
		return opts[optidx].opt_char;
	}
	assert(0);
	return(0);	/* never reached */
}
