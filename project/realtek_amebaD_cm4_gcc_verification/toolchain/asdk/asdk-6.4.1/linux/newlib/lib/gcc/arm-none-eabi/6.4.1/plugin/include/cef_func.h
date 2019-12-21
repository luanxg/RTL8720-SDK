/*
 *  cef_func.h
 *
 *  Created by Chung-Kai Chen on 2011/4/29.
 *  Copyright 2011 Realtek, Inc.. All rights reserved.
 */

#include "cef_file.h"

#include "output.h"
#include "hash-map.h"
#include "is-a.h"
#include "plugin-api.h"
#include "hashtab.h"
#include "hard-reg-set.h"
#include "function.h"
#include "ipa-ref.h"
#include "cgraph.h"
#include "attribs.h"

extern void cef_report_attributes (tree decl);
extern void cef_report_attributes_lto (tree decl);
extern void cef_report_inline (tree caller, tree callee);
extern void cef_report_inline_rejected (cgraph_edge *e);
extern void cef_report_function_versioning (tree old_decl, tree new_decl);
extern tree cef_intervene_attributes (tree decl, tree *attributes);

struct fn_rename_entry {
  const char *fn_orig_name;
  const char *fn_new_name;
};
extern htab_t fn_rename_htab;

extern const char *get_orig_name (const char *name);
extern void set_rename (const char *orig_name, const char *new_name);
