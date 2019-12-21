/*
 *  cef_file.h
 *
 *  Created by Chung-Kai Chen on 2011/4/29.
 *  Copyright 2011 Realtek, Inc.. All rights reserved.
 */

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "tm.h"
#include "hash-set.h"
#include "machmode.h"
#include "vec.h"
#include "double-int.h"
#include "input.h"
#include "alias.h"
#include "symtab.h"
#include "wide-int.h"
#include "inchash.h"
#include "tree.h"

#include "opts.h"

#include "stringpool.h" /* for get_identifier () */

#define CEF_VERSION "CEF-ITV-4.0"
#define CEF_ITV_LINE_LENGTH 4096

extern FILE *cef_itv_file;
extern FILE *cef_rpt_file;
extern const char *full_progname;
extern const char *cef_input_path;
extern const char *cef_output_path;

extern bool cef_do_flag_intervention ();
extern bool cef_do_flag_reporting ();
extern bool cef_do_attribute_intervention ();
extern bool cef_do_attribute_reporting ();

extern bool cef_do_flag_processing; /* extra control for flag processing */
extern bool cef_do_attribute_processing (); /* Some behavoirs on attributes will be different when CEF is on. For example, the appearance order of attributes in the source is honored. */

extern void cef_intervene_flags (struct cl_decoded_option **decoded_options, unsigned int *decoded_options_count, unsigned int lang_mask);
extern void cef_report_flags (struct cl_decoded_option **decoded_options, unsigned int *decoded_options_count);
extern int cef_report_ending (int retval);

extern char *cef_bf_tag; /* cef build-flow tag -- used to identify a build flow (compilation) */
extern char *cef_info_pass_on;
extern bool cef_binary_read (FILE*, char *);
extern bool cef_binary_write (FILE* file, const char *format, ...) __attribute__((format(printf,2,3)));

extern char *find_delimiter (char *s, char c);
extern char *encode (const char *s, const char *chars);
extern char *decode (char *s);
