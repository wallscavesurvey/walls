/*****************************************************************************/
// File: client_server.cpp [scope = APPS/CLIENT-SERVER]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/*****************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/*****************************************************************************/
// Licensee: Mr David McKenzie
// License number: 00590
// The licensee has been granted a NON-COMMERCIAL license to the contents of
// this source file.  A brief summary of this license appears below.  This
// summary is not to be relied upon in preference to the full text of the
// license agreement, accepted at purchase of the license.
// 1. The Licensee has the right to install and use the Kakadu software and
//    to develop Applications for the Licensee's own use.
// 2. The Licensee has the right to Deploy Applications built using the
//    Kakadu software to Third Parties, so long as such Deployment does not
//    result in any direct or indirect financial return to the Licensee or
//    any other Third Party, which further supplies or otherwise uses such
//    Applications.
// 3. The Licensee has the right to distribute Reusable Code (including
//    source code and dynamically or statically linked libraries) to a Third
//    Party, provided the Third Party possesses a license to use the Kakadu
//    software, and provided such distribution does not result in any direct
//    or indirect financial return to the Licensee.
/******************************************************************************
Description:
  Implements the objects declared in "kdu_client_window.h".  Used in JPIP
client and server components.
******************************************************************************/
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "kdu_client_window.h"

/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                    combine_sampled_ranges                          */
/*****************************************************************************/

static void
  combine_sampled_ranges(kdu_sampled_range &dst, kdu_sampled_range &src)
  /* Donates as many elements of the `src' range as possible to the `dst'
     range, leaving `dst' as large as possible.  In the process the `src'
     range could become empty, in which case `src.from' will be greater than
     `src.to'.  Each range is guaranteed to have the property that its
     `to' members is included in the range. */
{
  if (src.from > src.to)
    return;
  if ((src.context_type != 0) || (dst.context_type != 0))
    {
      if ((src.context_type != dst.context_type) ||
          (src.remapping_ids[0] != dst.remapping_ids[0]) ||
          (src.remapping_ids[1] != dst.remapping_ids[1]))
        return;
    }

  int step=dst.step;
  bool common_sampling =
    (src.step == step) && (((src.from-dst.from) % step) == 0);

  // Remove as much from the start of the `src' range as possible
  if (common_sampling)
    {
      if (src.from < dst.from)
        {
          if (src.to >= (dst.from-step))
            {
              dst.from = src.from;
              src.from = dst.to + step;
            }
        }
      else if (src.from <= dst.to)
        src.from = dst.to + step;
    }
  else
    { // Consider removing at most the first element of the `src' range
      if (src.from == (dst.from-step))
        dst.from -= step;
      else if (src.from == (dst.to+step))
        dst.to += step;
      if ((src.from >= dst.from) && (src.from <= dst.to) &&
          (((src.from - dst.from) % step) == 0))
        src.from += src.step;
    }
  if (src.from > src.to)
    return;

  // Remove as much from the end of the `src' range as possible
  if (common_sampling)
    {
      if (src.to > dst.to)
        {
          if (src.from <= (dst.to+step))
            {
              dst.to = src.to;
              src.to = dst.from - step;
            }
        }
      else if (src.to >= dst.from)
        src.to = dst.from - step;
    }
  else // Consider removing at most the last element of the `src' range
    {
      if (src.to == (dst.from-step))
        dst.from -= step;
      else if (src.to == (dst.to+step))
        dst.to += step;
      if ((src.to >= dst.from) && (src.to <= dst.to) &&
          (((src.to - dst.from) % step) == 0))
        src.to -= src.step;
    }
}

/*****************************************************************************/
/* STATIC                        kd_has_prefix                               */
/*****************************************************************************/

static bool kd_has_prefix(const char *string, const char *prefix)
{
  for (; (*string != '\0') && (*prefix != '\0'); string++, prefix++)
    if (*string != *prefix)
      return false;
  return (*prefix == '\0');
}

/*****************************************************************************/
/* STATIC                       kd_parse_ufloat                              */
/*****************************************************************************/

static bool kd_parse_ufloat(const char * &string, float &value)
  /* If `string' represents an unsigned float (following the JPIP definition),
     this function advances `string' and returns true, setting `value' equal to
     the parsed quantity.  Otherwise, the function returns false, leaving
     `string' and `value' untouched. */
{
  const char *sp;
  float val = 0.0F;
  for (sp=string; (*sp >= '0') && (*sp <= '9'); sp++)
    val = 10.0F * val + (float)((*sp) - '0');
  if (sp == string)
    return false; // Need at least one leading digit
  if (*sp == '.')
    {
      sp++;
      for (float scale=0.1F; (*sp >= '0') & (*sp <= '9'); sp++, scale*=0.1F)
        val += scale * (float)((*sp) - '0');
      if (sp[-1] == '.')
        return false;
    }
  value = val;
  string = sp;
  return true;
}

/*****************************************************************************/
/* STATIC                       kd_write_ufloat                              */
/*****************************************************************************/

static int kd_write_ufloat(char * &out, float val)
  /* Writes an unsigned floating point value to the `out' buffer, using the
     same format as that parsed by `kd_parse_ufloat' (JPIP UFLOAT data type).
     Returns the number of characters written to `out', or which would be
     written if `out' were non-NULL.  If `out' is non-NULL, the value is
     written to `out' and the `out' pointer is advanced by `num_chars'. */
{
  kdu_uint32 int_part = (kdu_uint32) val;
  int exponent = 0;
  while ((int_part>>exponent) > 1)
    exponent++;
  int d, frac_digits = (20-exponent) / 3; // Around 6 digits precision in total
  val -= (float) int_part;
  for (d=0; d < frac_digits; d++)
    val *= 10.0F;
  kdu_uint32 frac_part = (kdu_uint32)(val + 0.5F);
  char tmp_buf[40];
  if (frac_digits > 0)
    sprintf(tmp_buf,"%u.%u",int_part,frac_part);
  else
    sprintf(tmp_buf,"%u",int_part);
  int num_bytes = (int) strlen(tmp_buf);
  if (out != NULL)
    { strcpy(out,tmp_buf); out += num_bytes; }
  return num_bytes;
}

/*****************************************************************************/
/* STATIC                        parse_csf_pref                              */
/*****************************************************************************/

static const char *
  parse_csf_pref(const char *string, int &num_rows,
                 int &max_sensitivities_per_row, float *data)
  /* Parses `string' as the body of a "csf=" contrast-sensitivity preference
     statement, returning NULL if an error occurred, or else a pointer to
     the next character to be parsed.  If `data' is NULL, the
     function only sets the `num_rows' and `max_sensitivities_per_row'
     values, so that the caller can allocate a `data' array
     large enough to hold the data.  The data consists of a sequence of
     `num_rows' rows, separated by commas -- note that distinct
     related-pref-set entries in a "pref" request field are also separated
     by commas, so this function must distinguish two uses of the same
     separator.  Each row is of the form "density:<UFLOAT>[;angle:<UFLOAT>];"
     followed by a ";"-separated list of sensitivities. */
{
  const char *start = string;
  const char *match;
  float density, angle, sensitivity;
  num_rows = 0;
  while (kd_has_prefix(string,match="density:"))
    {
      string += strlen(match);
      if (!kd_parse_ufloat(string,density))
        return NULL;
      if (data != NULL)
        data[0] = density;
      
      angle = 0.0F;
      if (kd_has_prefix(string,match=";angle:"))
        {
          string += strlen(match);
          if (!kd_parse_ufloat(string,angle))
            return NULL;
        }
      if (data != NULL)
        data[1] = angle;
      
      int num_sensitivities = 0;
      for (; *string == ';'; num_sensitivities++)
        {
          string++;
          if (!kd_parse_ufloat(string,sensitivity))
            return NULL;
          if (data != NULL)
            data[2+num_sensitivities] = sensitivity;
        }
        
      num_rows++;
      if (num_sensitivities > max_sensitivities_per_row)
        {
          max_sensitivities_per_row = num_sensitivities;
          assert(data == NULL);
        }
      if (data != NULL)
        data += max_sensitivities_per_row;
      
      if (*string == ',')
        string++;
      else if (*string == '/')
        return string;
      else if (*string != '\0')
        return NULL;
    }
  if (string != start)
    {
      if (string[-1] == ',')
        string--;
    }
  if (num_rows == 0)
    return NULL;
  return string;
}


/* ========================================================================= */
/*                                kdu_range_set                              */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdu_range_set::~kdu_range_set                       */
/*****************************************************************************/

kdu_range_set::~kdu_range_set()
{
  if (ranges != NULL)
    delete[] ranges;
}

/*****************************************************************************/
/*                          kdu_range_set::copy_from                         */
/*****************************************************************************/

void
  kdu_range_set::copy_from(const kdu_range_set &src)
{
  init();
  bool new_allocation = false;
  if (src.num_ranges > max_ranges)
    {
      max_ranges = src.num_ranges;
      if (ranges != NULL)
        { delete[] ranges; ranges = NULL; }
      ranges = new kdu_sampled_range[max_ranges];
      new_allocation = true;
    }

  int n;
  for (n=0; n < src.num_ranges; n++)
    {
      ranges[n] = src.ranges[n];
      ranges[n].expansion = NULL;
    }
  this->num_ranges = n;
}

/*****************************************************************************/
/*                           kdu_range_set::contains                         */
/*****************************************************************************/

bool
  kdu_range_set::contains(const kdu_range_set &rhs,
                          bool empty_set_defaults_to_zero) const
{
  kdu_sampled_range src(0), dst;
  int k, n, rhs_ranges = rhs.num_ranges;
  if (empty_set_defaults_to_zero && (rhs_ranges == 0))
    rhs_ranges = 1;
  for (n=0; n < rhs_ranges; n++)
    {
      if (n < rhs.num_ranges)
        src = rhs.ranges[n];
      if (src.context_type == KDU_JPIP_CONTEXT_TRANSLATED)
        continue; // No need to include translated ranges
      if ((num_ranges == 0) && empty_set_defaults_to_zero)
        { // Special case in which the empty set defaults to the singleton 0
          if ((src.from != 0) || (src.to != 0))
            return false;
          continue;
        }
      for (k=0; k < num_ranges; k++)
        {
          dst = ranges[k];
          if ((dst.context_type != 0) || (src.context_type != 0))
            { // Check compatibility of contexts
              if ((dst.context_type != src.context_type) ||
                  (dst.remapping_ids[0] != src.remapping_ids[0]) ||
                  (dst.remapping_ids[1] != src.remapping_ids[1]))
                continue;
            }
          int step = dst.step;
          bool common_sampling =
            (src.step == step) && (((src.from-dst.from) % step) == 0);

          // Remove as much from the start of the `src' range as possible
          bool wrap_around=false;
          if ((src.from >= dst.from) && (src.from <= dst.to))
            {
              int new_from=src.from;
              if (common_sampling)
                new_from = dst.to + step;
              else if (((src.from - dst.from) % step) == 0)
                new_from += src.step;
              if (new_from < src.from)
                wrap_around = true;
              src.from = new_from;
            }

          // Remove as much from the end of the `src' range as possible
          if ((src.to >= dst.from) && (src.to <= dst.to))
            {
              int new_to=src.to;
              if (common_sampling)
                new_to = dst.from - step;
              else if (((src.to - dst.from) % step) == 0)
                new_to -= src.step;
              if (new_to > src.to)
                wrap_around = true;
              src.to = new_to;
            }

          if (wrap_around || !src)
            break;
        }
      if (k == num_ranges)
        return false;
    }
  return true;
}

/*****************************************************************************/
/*                             kdu_range_set::add                            */
/*****************************************************************************/

void
  kdu_range_set::add(kdu_sampled_range range, bool allow_merging)
{
  int n;
  if ((range.step <= 0) || (range.from > range.to))
    return;
  range.expansion = NULL;

  // Adjust `to' to make sure it is included in the range -- simplifies merging
  int kval = (range.to - range.from) / range.step;
  range.to = range.from + kval * range.step;

  if (max_ranges < (num_ranges+1))
    {
      int new_max_ranges = max_ranges + 10;
      kdu_sampled_range *new_ranges = new kdu_sampled_range[new_max_ranges];
      for (n=0; n < num_ranges; n++)
        new_ranges[n] = ranges[n];
      if (ranges != NULL)
        delete[] ranges;
      ranges = new_ranges;
      max_ranges = new_max_ranges;
    }

  if (allow_merging)
    { // Merge as much as possible of the new range into the existing list
      for (n=0; (n < num_ranges) && (range.from <= range.to); n++)
        combine_sampled_ranges(ranges[n],range);
    }

  // Add what remains of the new range into the list
  if (range.from <= range.to)
    ranges[num_ranges++] = range;

  if (!allow_merging)
    return;

  // Bubble sort the ranges
  bool done=false;
  while (!done)
    {
      done = true;
      for (n=1; n < num_ranges; n++)
        if (ranges[n-1].from > ranges[n].from)
          {
            range = ranges[n];
            ranges[n] = ranges[n-1];
            ranges[n-1] = range;
            done = false;
          }
    }

  // See if some more merging can be done (don't try to completely solve the
  // range merging problem, though).
  for (n=1; n < num_ranges; n++)
    if (ranges[n-1].to >= (ranges[n].from - ranges[n].step))
      {
        combine_sampled_ranges(ranges[n-1],ranges[n]);
        if (ranges[n].is_empty())
          {
            num_ranges--;
            for (int k=n; k < num_ranges; k++)
              ranges[k] = ranges[k+1];
            n--;
          }
      }
}

/*****************************************************************************/
/*                            kdu_range_set::expand                          */
/*****************************************************************************/

int
  kdu_range_set::expand(int *buf, int min_idx, int max_idx)
{
  int count = 0;
  for (int n=0; n < num_ranges; n++)
    {
      kdu_sampled_range *rg = ranges + n;
      int from = rg->from;
      if (from < min_idx)
        {
          from += (min_idx-from)/rg->step;
          if (from < min_idx)
            from += rg->step;
          assert(from >= min_idx);
        }
      for (int c=from; (c <= rg->to) && (c <= max_idx); c+=rg->step)
        {
          int i;
          for (i=0; i < count; i++)
            if (buf[i] == c)
              break;
          if (i == count)
            buf[count++] = c;
        }
    }
  return count;
}


/* ========================================================================= */
/*                                 kdu_window                                */
/* ========================================================================= */

/*****************************************************************************/
/*                           kdu_window::kdu_window                          */
/*****************************************************************************/

kdu_window::kdu_window()
{
  expansions = last_used_expansion = NULL;
  metareq = free_metareqs = NULL;
  have_metareq_all = have_metareq_global =
    have_metareq_stream = have_metareq_window = false;
  init();
}

/*****************************************************************************/
/*                          kdu_window::~kdu_window                          */
/*****************************************************************************/

kdu_window::~kdu_window()
{
  init_metareq();
  assert(metareq == NULL);
  while ((metareq=free_metareqs) != NULL)
    {
      free_metareqs = metareq->next;
      delete metareq;
    }
  while ((last_used_expansion=expansions) != NULL)
    {
      expansions = last_used_expansion->next;
      delete last_used_expansion;
    }
}

/*****************************************************************************/
/*                              kdu_window::init                             */
/*****************************************************************************/

void
  kdu_window::init()
{
  resolution = region.pos = region.size = kdu_coords(0,0);
  max_layers = 0; round_direction = -1;
  components.init(); codestreams.init(); contexts.init();
  last_used_expansion = NULL;
  init_metareq();
}

/*****************************************************************************/
/*                            kdu_window::is_empty                           */
/*****************************************************************************/

bool
  kdu_window::is_empty() const
{
  return ((resolution.x == 0) && (resolution.y == 0) &&
          region.is_empty() && codestreams.is_empty() &&
          components.is_empty() && contexts.is_empty() &&
          (max_layers == 0) && (metareq == NULL));
}

/*****************************************************************************/
/*                           kdu_window::copy_from                           */
/*****************************************************************************/

void
  kdu_window::copy_from(const kdu_window &src, bool copy_expansions)
{
  region = src.region;
  resolution = src.resolution;
  round_direction = src.round_direction;
  max_layers = src.max_layers;
  components.copy_from(src.components);
  codestreams.copy_from(src.codestreams);
  contexts.copy_from(src.contexts);

  if (copy_expansions)
    {
      int n, num_ranges = src.contexts.num_ranges;
      assert(num_ranges == this->contexts.num_ranges);
      for (n=0; n < num_ranges; n++)
        {
          kdu_range_set *src_expansion = src.contexts.ranges[n].expansion;
          if (src_expansion != NULL)
            {
              kdu_range_set *dst_expansion = create_context_expansion(n);
              dst_expansion->copy_from(*src_expansion);
            }
        }
    }

  copy_metareq_from(src);
}

/*****************************************************************************/
/*                      kdu_window::copy_metareq_from                        */
/*****************************************************************************/

void kdu_window::copy_metareq_from(const kdu_window &src)
{
  init_metareq();
  metadata_only = src.metadata_only;
  for (kdu_metareq *req=src.metareq; req != NULL; req=req->next)
    add_metareq(req->box_type,req->qualifier,req->priority,
                req->byte_limit,req->recurse,req->root_bin_id,req->max_depth);
}

/*****************************************************************************/
/*                        kdu_window::imagery_equals                         */
/*****************************************************************************/

bool
  kdu_window::imagery_equals(const kdu_window &rhs) const
{
  if ((rhs.max_layers != max_layers) ||
      (rhs.resolution != resolution) ||
      (rhs.round_direction != round_direction) ||
      (rhs.region != region) ||
      (rhs.components != components) ||
      (rhs.contexts != contexts) ||
      (rhs.metadata_only != metadata_only))
    return false;
  
  bool default_to_codestream_zero = contexts.is_empty();
  return rhs.codestreams.equals(codestreams,default_to_codestream_zero);
}

/*****************************************************************************/
/*                       kdu_window::imagery_contains                        */
/*****************************************************************************/

bool
  kdu_window::imagery_contains(const kdu_window &rhs) const
{
  if (max_layers != 0)
    {
      if ((rhs.max_layers == 0) || (max_layers < rhs.max_layers))
        return false;
    }
  
  if (metadata_only && !rhs.metadata_only)
    return false;
  
  if (!components.is_empty())
    { // Otherwise, all components are included in current window
      if (rhs.components.is_empty() || !components.contains(rhs.components))
        return false;
    }

  bool missing_codestream_defaults_to_zero = false;
  if (rhs.codestreams.is_empty() && (!codestreams.is_empty()) &&
      rhs.contexts.is_empty())
    missing_codestream_defaults_to_zero = true;
  if (!codestreams.contains(rhs.codestreams,
                            missing_codestream_defaults_to_zero))
    return false;

  if (!contexts.contains(rhs.contexts))
    return false;
  
  if ((rhs.resolution.x > resolution.x) || (rhs.resolution.y > resolution.y) ||
      (rhs.round_direction > round_direction))
    return false;

  kdu_coords min = region.pos;
  kdu_coords max = min + region.size - kdu_coords(1,1);
  kdu_coords rhs_min = rhs.region.pos;
  kdu_coords rhs_max = rhs_min + rhs.region.size - kdu_coords(1,1);
  if (((double) min.x)*((double) rhs.resolution.x) >
      ((double) rhs_min.x)*((double) resolution.x))
    return false;
  if (((double) min.y)*((double) rhs.resolution.y) >
      ((double) rhs_min.y)*((double) resolution.y))
    return false;
  if (((double) max.x)*((double) rhs.resolution.x) <
      ((double) rhs_max.x)*((double) resolution.x))
    return false;
  if (((double) max.y)*((double) rhs.resolution.y) <
      ((double) rhs_max.y)*((double) resolution.y))
    return false;
  
  return true;
}

/*****************************************************************************/
/*                  kdu_window::create_context_expansion                     */
/*****************************************************************************/

kdu_range_set *
  kdu_window::create_context_expansion(int which)
{
  kdu_sampled_range *range = contexts.access_range(which);
  if (range == NULL)
    return NULL;
  if (range->expansion != NULL)
    return range->expansion;

  if (last_used_expansion != NULL)
    { // Check to see if `contexts' has been reset since we last updated the
      // `last_used_expansion' member
      int n=0;
      const kdu_sampled_range *rscan;
      while ((rscan = contexts.access_range(n++)) != NULL)
        if (rscan->expansion != NULL)
          break;
      if (rscan == NULL)
        last_used_expansion = NULL; // Not using any expansions after all
    }

  if (expansions == NULL)
    expansions = new kdu_range_set;
  else if ((last_used_expansion != NULL) &&
           (last_used_expansion->next == NULL))
    last_used_expansion->next = new kdu_range_set;
  if (last_used_expansion == NULL)
    last_used_expansion = expansions;
  else
    last_used_expansion = last_used_expansion->next;
  range->expansion = last_used_expansion;
  range->expansion->init();
  return range->expansion;
}

/*****************************************************************************/
/*                        kdu_window::parse_context                          */
/*****************************************************************************/

const char *
  kdu_window::parse_context(const char *scan)
{
  kdu_sampled_range range;
  if ((scan[0] == 'j') && (scan[1] == 'p') &&
      (scan[2] == 'x') && (scan[3] == 'l'))
    range.context_type = KDU_JPIP_CONTEXT_JPXL;
  else if ((scan[0] == 'm') && (scan[1] == 'j') &&
           (scan[2] == '2') && (scan[3] == 't'))
    range.context_type = KDU_JPIP_CONTEXT_MJ2T;
  else
    { // Skip unrecognized expression
      while (isalnum(*scan) || (*scan == '<') || (*scan == '>') ||
             (*scan == '[') || (*scan == ']') || (*scan == '_') ||
             (*scan == '-') || (*scan == '+') || (*scan == ':') ||
             (*scan == '/') || (*scan == '.'))
        scan++;
      return scan;
    }
  if (scan[4] != '<')
    return scan;
  const char *open_range=scan;
  scan += 5;

  char *end_cp;
  range.from = range.to = strtol(scan,&end_cp,10);
  if (end_cp == scan)
    return scan-1;
  scan = end_cp;
  if (*scan == '-')
    {
      scan++;
      range.to = strtol(scan,&end_cp,10);
      if (end_cp == scan)
        range.to = INT_MAX;
      scan = end_cp;
    }
  if (*scan == ':')
    {
      scan++;
      range.step = strtol(scan,&end_cp,10);
      if (end_cp == scan)
        return scan-1;
      scan = end_cp;
    }

  assert((range.remapping_ids[0] < 0) && (range.remapping_ids[1] < 0));

  if ((range.context_type == KDU_JPIP_CONTEXT_MJ2T) &&
      (scan[0]=='+') && (scan[1]=='n') && (scan[2]=='o') && (scan[3]=='w'))
    {
      range.remapping_ids[1] = 0;
      scan += 4;
    }
  if (*scan != '>')
    return open_range;
  scan++;

  if (*scan == '[')
    {
      open_range = scan;
      scan++;
      if (range.context_type == KDU_JPIP_CONTEXT_JPXL)
        {
          if (*scan != 's')
            return open_range;
          scan++;
          range.remapping_ids[0] = strtol(scan,&end_cp,10);
          if (end_cp == scan)
            return open_range;
          scan = end_cp;
          if (*scan != 'i')
            return scan;
          scan++;
          range.remapping_ids[1] = strtol(scan,&end_cp,10);
          if (end_cp == scan)
            return open_range;
          scan = end_cp;
        }
      else if (range.context_type == KDU_JPIP_CONTEXT_MJ2T)
        {
          if ((scan[0]=='t') && (scan[1]=='r') && (scan[2]=='a') &&
              (scan[3]=='c') && (scan[4]=='k'))
            {
              range.remapping_ids[0] = 0;
              scan += 5;
            }
          else if ((scan[0]=='m') && (scan[1]=='o') && (scan[2]=='v') &&
                   (scan[3]=='i') && (scan[4]=='e'))
            {
              range.remapping_ids[0] = 1;
              scan += 5;
            }
          else
            return open_range;
        }
      if (*scan != ']')
        return open_range;
      scan++;
    }
  
  contexts.add(range,false);

  if (*scan != '=')
    return scan;
  scan++;

  // From this point on, we are parsing a context expansion
  int which=contexts.get_num_ranges() - 1;
  assert(which >= 0);
  kdu_range_set *set = create_context_expansion(which);

  range = kdu_sampled_range(); // Reinitialize `range'
  while (((range.from=strtol(scan,&end_cp,10)) >= 0) && (end_cp > scan))
    {
      scan = end_cp;
      if (*scan == '-')
        {
          scan++;
          range.to = strtol(scan,&end_cp,10);
          if (end_cp == scan)
            range.to = INT_MAX;
          scan = end_cp;
        }
      else
        range.to = range.from;
      if (*scan == ':')
        {
          scan++;
          range.step = strtol(scan,&end_cp,10);
          if (end_cp == scan)
            return scan-1;
          scan = end_cp;
        }
      else
        range.step = 1;
      set->add(range);
      if (*scan == ',')
        scan++;
    }

  return scan;
}

/*****************************************************************************/
/*                        kdu_window::parse_metareq                          */
/*****************************************************************************/

const char *
  kdu_window::parse_metareq(const char *string)
{
  while (*string != '\0')
    {
      while ((*string == ',') || (*string == ';'))
        string++;
      if (*string != '[')
        return string;

      const char *end_prop, *prop=string+1;
      for (string=prop+1; *string != ']'; string++)
        if (*string == '\0')
          return prop;
      end_prop = string;
      string++;

      // Parse global properties for the descriptor
      kdu_metareq req;
      req.root_bin_id = 0;
      req.max_depth = INT_MAX;
      if (*string == 'R')
        {
          for (string++; (*string >= '0') && (*string <= '9'); string++)
            req.root_bin_id = (req.root_bin_id * 10) + (int)(*string - '0');
          if (string[-1] == 'R')
            return string;
        }
      if (*string == 'D')
        {
          req.max_depth = 0;
          for (string++; (*string >= '0') && (*string <= '9'); string++)
            req.max_depth = (req.max_depth * 10) + (int)(*string - '0');
          if (string[-1] == 'D')
            return string;
        }

      if ((string[0] == '!') && (string[1] == '!') && (string[2] == '\0'))
        {
          string += 2;
          metadata_only = true;
        }
      else if ((*string != ',') && (*string != ';') && (*string != '\0'))
        return string;
      
      // Now cycle through all the box properties in the descriptor
      while (prop < end_prop)
        {
          while ((*prop == ',') || (*prop == ';'))
            prop++;
          req.box_type = 0;
          if (*prop == '*')
            prop++;
          else
            {
              int code_len = 0;
              req.box_type = kdu_parse_type_code(prop,code_len);
              prop += code_len;
            }
          req.recurse = false;
          req.byte_limit = INT_MAX;
          if (*prop == ':')
            {
              req.byte_limit = 0;
              for (prop++; (*prop >= '0') && (*prop <= '9'); prop++)
                req.byte_limit = (req.byte_limit * 10) + (int)(*prop - '0');
              if (prop[-1] == ':')
                {
                  if (*prop == 'r')
                    { prop++; req.recurse = true; }
                  else
                    return prop;
                }
            }
          req.qualifier = KDU_MRQ_DEFAULT;
          if (*prop == '/')
            {
              prop++;
              req.qualifier = 0;
              while ((*prop == 'w') || (*prop == 's') ||
                     (*prop == 'g') || (*prop == 'a'))
                {
                  if (*prop == 'w')
                    { prop++; req.qualifier |= KDU_MRQ_WINDOW; }
                  else if (*prop == 's')
                    { prop++; req.qualifier |= KDU_MRQ_STREAM; }
                  else if (*prop == 'g')
                    { prop++; req.qualifier |= KDU_MRQ_GLOBAL; }
                  else if (*prop == 'a')
                    { prop++; req.qualifier |= KDU_MRQ_ALL; }
                }
              if (req.qualifier == 0)
                return prop-1;
            }
          req.priority = false;
          if (*prop == '!')
            { prop++; req.priority = true; }
          if ((prop < end_prop) && (*prop != ',') && (*prop != ';'))
            return prop;
          add_metareq(req.box_type,req.qualifier,req.priority,
                      req.byte_limit,req.recurse,req.root_bin_id,
                      req.max_depth);
        }
    }
  return NULL;
}

/* ========================================================================= */
/*                             kdu_window_prefs                              */
/* ========================================================================= */

/*****************************************************************************/
/*                        kdu_window_prefs::set_pref                         */
/*****************************************************************************/

bool kdu_window_prefs::set_pref(int pref_flag, bool make_required)
{
  int *set_flags = (make_required)?&required:&preferred;
  int set_mask = 0;
  if (pref_flag & KDU_WINDOW_PREF_MASK)
    set_mask = KDU_WINDOW_PREF_MASK;
  else if (pref_flag & KDU_CONCISENESS_PREF_MASK)
    set_mask = KDU_CONCISENESS_PREF_MASK;
  else if (pref_flag & KDU_PLACEHOLDER_PREF_MASK)
    set_mask = KDU_PLACEHOLDER_PREF_MASK;
  else if (pref_flag & KDU_CODESEQ_PREF_MASK)
    set_mask = KDU_CODESEQ_PREF_MASK;
  else
    return false;
  preferred &= ~set_mask;
  required &= ~set_mask;
  (*set_flags) |= pref_flag;
  return true;
}

/*****************************************************************************/
/*                         kdu_window_prefs::update                          */
/*****************************************************************************/

int kdu_window_prefs::update(const kdu_window_prefs &src)
{
  int updated_pref_sets = 0;
  int delta_flags = (src.preferred ^ preferred) | (src.required ^ required);
  if ((src.preferred | src.required) & KDU_WINDOW_PREF_MASK)
    {
      preferred &= ~KDU_WINDOW_PREF_MASK;
      required &= ~KDU_WINDOW_PREF_MASK;
      preferred |= (src.preferred & KDU_WINDOW_PREF_MASK);
      required |= (src.required & KDU_WINDOW_PREF_MASK);
      if (delta_flags & KDU_WINDOW_PREF_MASK)
        updated_pref_sets |= KDU_WINDOW_PREF_MASK;
    }
  if ((src.preferred | src.required) & KDU_CONCISENESS_PREF_MASK)
    { 
      preferred &= ~KDU_CONCISENESS_PREF_MASK;
      required &= ~KDU_CONCISENESS_PREF_MASK;
      preferred |= (src.preferred & KDU_CONCISENESS_PREF_MASK);
      required |= (src.required & KDU_CONCISENESS_PREF_MASK);
      if (delta_flags & KDU_CONCISENESS_PREF_MASK)
        updated_pref_sets |= KDU_CONCISENESS_PREF_MASK;
    }
  if ((src.preferred | src.required) & KDU_PLACEHOLDER_PREF_MASK)
    { 
      preferred &= ~KDU_PLACEHOLDER_PREF_MASK;
      required &= ~KDU_PLACEHOLDER_PREF_MASK;
      preferred |= (src.preferred & KDU_PLACEHOLDER_PREF_MASK);
      required |= (src.required & KDU_PLACEHOLDER_PREF_MASK);
    }
  if ((src.preferred | src.required) & KDU_CODESEQ_PREF_MASK)
    { 
      preferred &= ~KDU_CODESEQ_PREF_MASK;
      required &= ~KDU_CODESEQ_PREF_MASK;
      preferred |= (src.preferred & KDU_CODESEQ_PREF_MASK);
      required |= (src.required & KDU_CODESEQ_PREF_MASK);
      if (delta_flags & KDU_CODESEQ_PREF_MASK)
        updated_pref_sets |= KDU_CODESEQ_PREF_MASK;
    }
  if ((src.preferred | src.required) & KDU_MAX_BANDWIDTH_PREF)
    { 
      preferred &= ~KDU_MAX_BANDWIDTH_PREF;
      required &= ~KDU_MAX_BANDWIDTH_PREF;
      preferred |= (src.preferred & KDU_MAX_BANDWIDTH_PREF);
      required |= (src.required & KDU_MAX_BANDWIDTH_PREF);
      if ((delta_flags & KDU_MAX_BANDWIDTH_PREF) ||
          (max_bandwidth != src.max_bandwidth))
        updated_pref_sets |= KDU_MAX_BANDWIDTH_PREF;
      max_bandwidth = src.max_bandwidth;
    }
  if (src.preferred & KDU_BANDWIDTH_SLICE_PREF)
    { 
      preferred &= ~KDU_BANDWIDTH_SLICE_PREF;
      required &= ~KDU_BANDWIDTH_SLICE_PREF;
      preferred |= (src.preferred & KDU_BANDWIDTH_SLICE_PREF);
      required |= (src.required & KDU_BANDWIDTH_SLICE_PREF);
      if ((delta_flags & KDU_BANDWIDTH_SLICE_PREF) ||
          (bandwidth_slice != src.bandwidth_slice))
        updated_pref_sets |= KDU_BANDWIDTH_SLICE_PREF;
      bandwidth_slice = src.bandwidth_slice;
    }
  if (src.preferred & KDU_COLOUR_METH_PREF)
    { 
      preferred &= ~KDU_COLOUR_METH_PREF;
      required &= ~KDU_COLOUR_METH_PREF;
      preferred |= (src.preferred & KDU_COLOUR_METH_PREF);
      required |= (src.required & KDU_COLOUR_METH_PREF);
      bool delta_meth = false;
      for (int i=0; i < 4; i++)
        { 
          if (colour_meth_pref_limits[i] != src.colour_meth_pref_limits[i])
            delta_meth = true;
          colour_meth_pref_limits[i] = src.colour_meth_pref_limits[i];
        }
      if ((delta_flags & KDU_COLOUR_METH_PREF) || delta_meth)
        updated_pref_sets |= KDU_COLOUR_METH_PREF;
    }
  if (src.preferred & KDU_CONTRAST_SENSITIVITY_PREF)
    { 
      preferred &= ~KDU_CONTRAST_SENSITIVITY_PREF;
      required &= ~KDU_CONTRAST_SENSITIVITY_PREF;
      preferred |= (src.preferred & KDU_CONTRAST_SENSITIVITY_PREF);
      required |= (src.required & KDU_CONTRAST_SENSITIVITY_PREF);
      bool delta_csf = false;
      if ((num_csf_angles != src.num_csf_angles) ||
          (max_sensitivities_per_csf_angle !=
           src.max_sensitivities_per_csf_angle))
        delta_csf = true;
      num_csf_angles = src.num_csf_angles;
      max_sensitivities_per_csf_angle = src.max_sensitivities_per_csf_angle;
      int i, num_sensitivities =
        (2+max_sensitivities_per_csf_angle)*num_csf_angles;
      if (!delta_csf)
        { // So far, both sets of sensitivites have identical dimensions
          for (i=0; i < num_sensitivities; i++)
            {
              if (csf_sensitivities[i] != src.csf_sensitivities[i])
                delta_csf = true;
              csf_sensitivities[i] = src.csf_sensitivities[i];
            }
        }
      else
        { // Allocate and copy a new set of sensitivities
          if (csf_sensitivities != NULL)
            delete[] csf_sensitivities;
          csf_sensitivities = NULL;
          csf_sensitivities = new float[num_sensitivities];
          for (i=0; i < num_sensitivities; i++)
            csf_sensitivities[i] = src.csf_sensitivities[i];
        }
      if ((delta_flags & KDU_CONTRAST_SENSITIVITY_PREF) || delta_csf)
        updated_pref_sets |= KDU_CONTRAST_SENSITIVITY_PREF;
    }
  denied |= src.denied; // Keep track of things we have ever denied
  return updated_pref_sets;
}

/*****************************************************************************/
/*                          kdu_window_prefs::init                           */
/*****************************************************************************/

void kdu_window_prefs::init()
{
  preferred = required = denied = 0;
  max_bandwidth = 0;
  bandwidth_slice = 0;
  for (int i=0; i < 4; i++)
    colour_meth_pref_limits[i] = 0;
  num_csf_angles = max_sensitivities_per_csf_angle = 0;
  if (csf_sensitivities != NULL)
    { delete[] csf_sensitivities; csf_sensitivities = NULL; }
}

/*****************************************************************************/
/*                       kdu_window_prefs::parse_prefs                       */
/*****************************************************************************/

const char *
  kdu_window_prefs::parse_prefs(const char *string)
{
  init(); // Make sure it's empty to start with
  const char *match;
  while (*string != '\0')
    {
      int new_flag = 0;
      int new_mask = 0;
      const char *pref_start = string;
      if (kd_has_prefix(string,match="fullwindow"))
        {
          string += strlen(match);
          new_flag = KDU_WINDOW_PREF_FULL;
          new_mask = KDU_WINDOW_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="progressive"))
        {
          string += strlen(match);
          new_flag = KDU_WINDOW_PREF_PROGRESSIVE;
          new_mask = KDU_WINDOW_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="concise"))
        {
          string += strlen(match);
          new_flag = KDU_CONCISENESS_PREF_CONCISE;
          new_mask = KDU_CONCISENESS_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="loose"))
        {
          string += strlen(match);
          new_flag = KDU_CONCISENESS_PREF_LOOSE;
          new_mask = KDU_CONCISENESS_PREF_MASK;
        }      
      else if (kd_has_prefix(string,match="meta:incr"))
        {
          string += strlen(match);
          new_flag = KDU_PLACEHOLDER_PREF_INCR;
          new_mask = KDU_PLACEHOLDER_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="meta:equiv"))
        {
          string += strlen(match);
          new_flag = KDU_PLACEHOLDER_PREF_EQUIV;
          new_mask = KDU_PLACEHOLDER_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="meta:orig"))
        {
          string += strlen(match);
          new_flag = KDU_PLACEHOLDER_PREF_ORIG;
          new_mask = KDU_PLACEHOLDER_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="codeseq:sequential"))
        {
          string += strlen(match);
          new_flag = KDU_CODESEQ_PREF_FWD;
          new_mask = KDU_CODESEQ_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="codeseq:reverse-sequential"))
        {
          string += strlen(match);
          new_flag = KDU_CODESEQ_PREF_BWD;
          new_mask = KDU_CODESEQ_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="codeseq:interleaved"))
        {
          string += strlen(match);
          new_flag = KDU_CODESEQ_PREF_ILVD;
          new_mask = KDU_CODESEQ_PREF_MASK;
        }
      else if (kd_has_prefix(string,match="mbw:"))
        {
          string += strlen(match);
          new_flag = new_mask = KDU_MAX_BANDWIDTH_PREF;
          const char *param_start = string;
          for (; (*string >= '0') && (*string <= '9'); string++)
            max_bandwidth = (max_bandwidth*10) + (kdu_long)(*string-'0');
          if (max_bandwidth == 0)
            return param_start;
          if (*string == 'K')
            { max_bandwidth *= 1000; string++; }
          else if (*string == 'M')
            { max_bandwidth *= 1000000; string++; }
          else if (*string == 'G')
            { max_bandwidth *= 1000000; max_bandwidth *= 1000; string++; }
          else if (*string == 'T')
            { max_bandwidth *= 1000000; max_bandwidth *= 1000000; string++; }
        }
      else if (kd_has_prefix(string,match="slice:"))
        {
          string += strlen(match);
          new_flag = new_mask = KDU_BANDWIDTH_SLICE_PREF;
          const char *param_start = string;
          for (; (*string >= '0') && (*string <= '9'); string++)
            bandwidth_slice = (bandwidth_slice*10) + (kdu_uint32)(*string-'0');
          if (bandwidth_slice == 0)
            return param_start;
        }
      else if (kd_has_prefix(string,match="color-"))
        {
          new_flag = new_mask = KDU_COLOUR_METH_PREF;
          do {
            if (*string == ';') string++;
            kdu_byte *limit_ref = colour_meth_pref_limits;
            if (kd_has_prefix(string,match="color-enum"))
              { limit_ref += 0; string += strlen(match); }
            else if (kd_has_prefix(string,match="color-ricc"))
              { limit_ref += 1; string += strlen(match); }
            else if (kd_has_prefix(string,match="color-icc"))
              { limit_ref += 2; string += strlen(match); }
            else if (kd_has_prefix(string,match="color-vend"))
              { limit_ref += 3; string += strlen(match); }
            else
              return string;
            kdu_uint32 limit = 255;
            if (*string == ':')
              {
                const char *param_start = string;
                for (limit=0, string++; (*string >= '0') && (*string <= '9'); )
                  limit = (limit*10) + (kdu_uint32)(*(string++)-'0');
                if (limit == 0)
                  return param_start;
                if (limit > 4)
                  limit = 4;
              }
            if (limit < 1)
              *limit_ref = 1;
            else
              *limit_ref = (kdu_byte) limit;
          } while (*string == ';');
        }
      else if (kd_has_prefix(string,match="csf="))
        {
          new_flag = new_mask = KDU_CONTRAST_SENSITIVITY_PREF;
          string += strlen(match);
          if (parse_csf_pref(string,num_csf_angles,
                             max_sensitivities_per_csf_angle,NULL)==NULL)
            return pref_start;
          csf_sensitivities =
            new float[num_csf_angles*(2+max_sensitivities_per_csf_angle)];
          string = parse_csf_pref(string,num_csf_angles,
                                  max_sensitivities_per_csf_angle,
                                  csf_sensitivities);
        }
      
      if ((preferred | required) & new_mask)
        return pref_start; // More than one occurrence within related-pref-set
      if ((string[0] == '/') && (string[1] == 'r'))
        { // Preference is required
          required |= new_flag;
          string += 2;
        }
      else
        preferred |= new_flag;
      if (*string == '\0')
        break; // Finished parsing all preferences
      if (*string == ',')
        string++; // Encountered preference separator
      else
        return string; // Error encountered here 
    }
  
  return NULL; // Parsing successful
}

/*****************************************************************************/
/*                       kdu_window_prefs::write_prefs                       */
/*****************************************************************************/

int kdu_window_prefs::write_prefs(char tgt_buf[], int write_mask)
{
  int num_chars = 0;
  int prefs = preferred | required;
  const char *label;
  char *out = tgt_buf;
  
  if ((prefs & KDU_WINDOW_PREF_MASK) &&
      (write_mask & KDU_WINDOW_PREF_MASK))
    {
      if (prefs & KDU_WINDOW_PREF_PROGRESSIVE)
        label = "progressive";
      else
        label = "fullwindow";
      num_chars += (int) strlen(label);
      if (out != NULL)
        { strcpy(out,label); out += strlen(label); }
      if (required & KDU_WINDOW_PREF_MASK)
        {
          num_chars += 2;
          if (out != NULL)
            { *(out++) = '/'; *(out++) = 'r'; }
        }
    }

  if ((prefs & KDU_CONCISENESS_PREF_MASK) &&
      (write_mask & KDU_CONCISENESS_PREF_MASK))
    {
      if (prefs & KDU_CONCISENESS_PREF_CONCISE)
        label = "concise";
      else
        label = "loose";
      num_chars += (int) strlen(label);
      if (out != NULL)
        { strcpy(out,label); out += strlen(label); }
      if (required & KDU_CONCISENESS_PREF_MASK)
        {
          num_chars += 2;
          if (out != NULL)
            { *(out++) = '/'; *(out++) = 'r'; }
        }
    }
  
  if ((prefs & KDU_PLACEHOLDER_PREF_MASK) &&
      (write_mask & KDU_PLACEHOLDER_PREF_MASK))
    {
      if (num_chars > 0)
        {
          num_chars++;
          if (out != NULL)
            *(out++) = ',';
        }
      if (prefs & KDU_PLACEHOLDER_PREF_INCR)
        label = "meta:incr";
      else if (prefs & KDU_PLACEHOLDER_PREF_EQUIV)
        label = "meta:equiv";
      else
        label = "meta:orig";
      num_chars += (int) strlen(label);
      if (out != NULL)
        { strcpy(out,label); out += strlen(label); }
      if (required & KDU_PLACEHOLDER_PREF_MASK)
        {
          num_chars += 2;
          if (out != NULL)
            { *(out++) = '/'; *(out++) = 'r'; }
        }
    }
  
  if ((prefs & KDU_CODESEQ_PREF_MASK) &&
      (write_mask & KDU_CODESEQ_PREF_MASK))
    {
      if (num_chars > 0)
        {
          num_chars++;
          if (out != NULL)
            *(out++) = ',';
        }
      if (prefs & KDU_CODESEQ_PREF_FWD)
        label = "codeseq:sequential";
      else if (prefs & KDU_CODESEQ_PREF_BWD)
        label = "codeseq:reverse-sequential";
      else
        label = "codeseq:interleaved";
      num_chars += (int) strlen(label);
      if (out != NULL)
        { strcpy(out,label); out += strlen(label); }
      if (required & KDU_CODESEQ_PREF_MASK)
        {
          num_chars += 2;
          if (out != NULL)
            { *(out++) = '/'; *(out++) = 'r'; }
        }
    }
  
  if ((prefs & KDU_MAX_BANDWIDTH_PREF) &&
      (write_mask & KDU_MAX_BANDWIDTH_PREF))
    {
      if (num_chars > 0)
        {
          num_chars++;
          if (out != NULL)
            *(out++) = ',';
        }
      kdu_long val = max_bandwidth;
      const char *suffix = "";
      if (val >= 1000)
        { suffix="K"; val = val / 1000; }
      if (val >= 1000)
        { suffix="M"; val = val / 1000; }
      if (val >= 1000)
        { suffix="G"; val = val / 1000; }
      if (val >= 1000)
        { suffix="T"; val = val / 1000; }
      char tmp_buf[20]; sprintf(tmp_buf,"mbw:%u%s",(kdu_uint32)val,suffix);
      num_chars += (int) strlen(tmp_buf);
      if (out != NULL)
        { strcpy(out,tmp_buf); out += strlen(tmp_buf); }
      if (required & KDU_MAX_BANDWIDTH_PREF)
        {
          num_chars += 2;
          if (out != NULL)
            { *(out++) = '/'; *(out++) = 'r'; }
        }
    }
  
  if ((prefs & KDU_BANDWIDTH_SLICE_PREF) &&
      (write_mask & KDU_BANDWIDTH_SLICE_PREF))
    {
      if (num_chars > 0)
        {
          num_chars++;
          if (out != NULL)
            *(out++) = ',';
        }
      char tmp_buf[20]; sprintf(tmp_buf,"slice:%u",bandwidth_slice);
      num_chars += (int) strlen(tmp_buf);
      if (out != NULL)
        { strcpy(out,tmp_buf); out += strlen(tmp_buf); }
      if (required & KDU_BANDWIDTH_SLICE_PREF)
        {
          num_chars += 2;
          if (out != NULL)
            { *(out++) = '/'; *(out++) = 'r'; }
        }
    }
  
  if ((prefs & KDU_COLOUR_METH_PREF) &&
      (write_mask & KDU_COLOUR_METH_PREF) &&
      (colour_meth_pref_limits[0] | colour_meth_pref_limits[1] |
       colour_meth_pref_limits[2] | colour_meth_pref_limits[3]))
    {
      if (num_chars > 0)
        {
          num_chars++;
          if (out != NULL)
            *(out++) = ',';
        }
      bool meth_pref_found=false;
      for (int i=0; i < 4; i++)
        if (colour_meth_pref_limits[i] != 0)
          {
            if (meth_pref_found)
              {
                num_chars++;
                if (out != NULL)
                  *(out++) = ';';
              }
            meth_pref_found = true;
            switch (i) {
              case 0: label = "color-enum"; break;
              case 1: label = "color-ricc"; break;
              case 2: label = "color-icc"; break;
              case 3: label = "color-vend"; break;
            }
           num_chars += (int) strlen(label); 
           if (out != NULL)
             { strcpy(out,label); out += strlen(label); }
           if (colour_meth_pref_limits[i] <= 4)
             {
               num_chars += 2;
               if (out != NULL)
                 { *(out++) = ':';
                   *(out++) = (char)('0'+colour_meth_pref_limits[i]); }
             }
          }
      assert(meth_pref_found);
      
      if (required & KDU_COLOUR_METH_PREF)
        {
          num_chars += 2;
          if (out != NULL)
            { *(out++) = '/'; *(out++) = 'r'; }
        }      
    }

  if ((prefs & KDU_CONTRAST_SENSITIVITY_PREF) &&
      (write_mask & KDU_CONTRAST_SENSITIVITY_PREF) &&
      (num_csf_angles > 0) && (max_sensitivities_per_csf_angle > 0))
    {
      if (num_chars > 0)
        {
          num_chars++;
          if (out != NULL)
            *(out++) = ',';
        }
      label = "csf=";
      num_chars += (int) strlen(label);
      if (out != NULL)
        { strcpy(out,label); out += strlen(label); }
      for (int r=0; r < num_csf_angles; r++)
        {
          if (r > 0)
            {
              num_chars++;
              if (out != NULL)
                *(out++) = ',';
            }
          float *csf_data = csf_sensitivities +
            r*(2+max_sensitivities_per_csf_angle);
          label = "density:";
          num_chars += (int) strlen(label);
          if (out != NULL)
            { strcpy(out,label); out += strlen(label); }
          num_chars += kd_write_ufloat(out,csf_data[0]);
          if (csf_data[1] != 0.0F)
            {
              label = ";angle:";
              num_chars += (int) strlen(label);
              if (out != NULL)
                { strcpy(out,label); out += strlen(label); }
              num_chars += kd_write_ufloat(out,csf_data[1]);
            }
          csf_data += 2;
          for (int c=0; c < max_sensitivities_per_csf_angle; c++)
            if (csf_data[c] < 0.0F)
              break;
            else
              {
                num_chars++;
                if (out != NULL) *(out++) = ';';
                num_chars += kd_write_ufloat(out,csf_data[c]);
              }
        }
      if (required & KDU_CONTRAST_SENSITIVITY_PREF)
        {
          num_chars += 2;
          if (out != NULL)
            { *(out++) = '/'; *(out++) = 'r'; }
        }      
    }  
  
  if (out != NULL)
    {
      assert(out == (tgt_buf+num_chars));
      *out = '\0';
    }
  return num_chars;
}


/* ========================================================================= */
/*                             kdu_window_model                              */
/* ========================================================================= */

/*****************************************************************************/
/*                         kdu_window_model::clear                           */
/*****************************************************************************/

void kdu_window_model::clear()
{
  add_context = NULL;
  kdwm_stream_context *sc;
  while ((sc=contexts) != NULL)
    { 
      contexts = sc->next;
      sc->next = free_contexts;
      free_contexts = sc;
      while ((sc->tail=sc->head) != NULL)
        { 
          sc->head = sc->tail->next;
          sc->tail->next = free_instructions;
          free_instructions = sc->tail;
        }
    }
  while ((meta_tail = meta_head) != NULL)
    { 
      meta_head = meta_tail->next;
      meta_tail->next = free_instructions;
      free_instructions = meta_tail;
    }
}

/*****************************************************************************/
/*                        kdu_window_model::append                           */
/*****************************************************************************/

void kdu_window_model::append(const kdu_window_model &src)
{
  kdwm_stream_context *sc, *tc, *prev_tc;
  kdwm_instruction *si, *ti;
  for (sc=src.contexts; sc != NULL; sc=sc->next)
    { 
      if (sc->head == NULL)
        continue; // No instructions here
      for (prev_tc=NULL, tc=contexts; tc != NULL; prev_tc=tc, tc=tc->next)
        if ((tc->smin > sc->smin) ||
            ((tc->smin == sc->smin) && (tc->smax >= sc->smax)))
          break; // Contexts inserted in order of `smin' then `smax'
      if ((tc == NULL) || (tc->smin != sc->smin) || (tc->smax != sc->smax))
        { // Insert new codestream context
          kdwm_stream_context *new_tc = free_contexts;
          if (new_tc == NULL)
            new_tc = new kdwm_stream_context;
          else
            free_contexts = new_tc->next;
          new_tc->next = tc;
          if (prev_tc == NULL)
            contexts = new_tc;
          else
            prev_tc->next = new_tc;
          tc = new_tc;
          tc->smin = sc->smin;
          tc->smax = sc->smax;
          tc->head = tc->tail = NULL;
        }
      for (si=sc->head; si != NULL; si=si->next)
        { // Append copy of intruction
          if ((ti = free_instructions) == NULL)
            ti = new kdwm_instruction;
          else
            free_instructions = ti->next;
          *ti = *si;
          ti->next = NULL;
          if (tc->tail == NULL)
            tc->head = tc->tail = ti;
          else
            tc->tail = tc->tail->next = ti;
        }
    }
  
  for (si=src.meta_head; si != NULL; si=si->next)
    { // Append copy of metadata instruction
      if ((ti = free_instructions) == NULL)
        ti = new kdwm_instruction;
      else
        free_instructions = ti->next;
      *ti = *si;
      ti->next = NULL;
      if (meta_tail == NULL)
        meta_head = meta_tail = ti;
      else
        meta_tail = meta_tail->next = ti;
    }
}

/*****************************************************************************/
/*                kdu_window_model::set_codestream_context                   */
/*****************************************************************************/

void kdu_window_model::set_codestream_context(int smin, int smax)
{
  if (smax < smin)
    { 
      assert(0);
      smax = smin;
    }
  kdwm_stream_context *sc, *prev_sc;
  for (prev_sc=NULL, sc=contexts; sc != NULL; prev_sc=sc, sc=sc->next)
    if ((sc->smin > smin) || ((sc->smin == smin) && (sc->smax >= smax)))
      break; // Contexts inserted in order of `smin' then `smax'
  if ((sc == NULL) || (sc->smin != smin) || (sc->smax != smax))
    { // Insert new codestream context
      kdwm_stream_context *new_sc = free_contexts;
      if (new_sc == NULL)
        new_sc = new kdwm_stream_context;
      else
        free_contexts = new_sc->next;
      new_sc->next = sc;
      if (prev_sc == NULL)
        contexts = new_sc;
      else
        prev_sc->next = new_sc;
      sc = new_sc;
      sc->smin = smin;
      sc->smax = smax;
      sc->head = sc->tail = NULL;
    }
  add_context = sc;
}

/*****************************************************************************/
/*             kdu_window_model::add_instruction (absolute bin-ID)           */
/*****************************************************************************/

void kdu_window_model::add_instruction(int databin_class, kdu_long bin_id,
                                       int flags, int limit)
{
  if ((add_context == NULL) && (databin_class != KDU_META_DATABIN))
    { // Must call `init' first! 
      assert(0);
      return;
    }
  if ((limit == 0) && (flags & KDU_WINDOW_MODEL_SUBTRACTIVE))
    return; // Additive instruction with limit of 0 is a no-op
  kdwm_instruction *inst = free_instructions;
  if (inst == NULL)
    inst = new kdwm_instruction;
  else
    free_instructions = inst->next;
  inst->next = NULL;
  inst->subtractive =
    ((flags & KDU_WINDOW_MODEL_SUBTRACTIVE) || background_full)?1:0;
  inst->absolute_bin_id = 1;
  inst->atomic = (bin_id >= 0);
  if (inst->subtractive)
    { 
      if (limit < 0)
        limit = 0;
      else if (limit == INT_MAX)
        limit = INT_MAX-1;
    }
  if (databin_class == KDU_PRECINCT_DATABIN)
    { 
      if (limit >= (INT_MAX>>1))
        limit = (INT_MAX>>1)-1;
      if (limit > 0)
        limit += limit + ((flags & KDU_WINDOW_MODEL_LAYERS)?1:0);
    }
  inst->limit = limit; 
  inst->databin_class = databin_class;
  inst->bin_id = bin_id;
  if (databin_class == KDU_META_DATABIN)
    { 
      if (meta_tail == NULL)
        meta_tail = meta_head = inst;
      else
        meta_tail = meta_tail->next = inst;
    }
  else
    { 
      if (add_context->tail == NULL)
        add_context->tail = add_context->head = inst;
      else
        add_context->tail = add_context->tail->next = inst;
    }
}

/*****************************************************************************/
/*               kdu_window_model::add_instruction (implicit)                */
/*****************************************************************************/

void kdu_window_model::add_instruction(int tmin, int tmax, int cmin, int cmax,
                                       int rmin, int rmax, kdu_long pmin,
                                       kdu_long pmax, int flags, int limit)
{
  if (add_context == NULL)
    { // Must call `init' first! 
      assert(0);
      return;
    }
  if ((limit == 0) && (flags & KDU_WINDOW_MODEL_SUBTRACTIVE))
    return; // Additive instruction with limit of 0 is a no-op
  if (tmax < tmin)
    tmax = tmin;
  if (rmax < rmin)
    rmax = rmin;
  if (cmax < cmin)
    cmax = cmin;
  if (pmax < pmin)
    pmax = pmin;
  kdwm_instruction *inst = free_instructions;
  if (inst == NULL)
    inst = new kdwm_instruction;
  else
    free_instructions = inst->next;
  inst->next = NULL;
  if (add_context->tail == NULL)
    add_context->tail = add_context->head = inst;
  else
    add_context->tail = add_context->tail->next = inst;
  inst->subtractive =
    ((flags & KDU_WINDOW_MODEL_SUBTRACTIVE) || background_full)?1:0;
  inst->absolute_bin_id = 0;
  inst->atomic =
    ((tmin < tmax) || (cmin < cmax) || (rmin < rmax) || (pmin < pmax))?1:0;
  if (inst->subtractive && (limit < 0))
    limit = 0;
  else if (limit >= (INT_MAX>>1))
    limit = (INT_MAX>>1)-1;
  if (limit > 0)
    limit += limit + ((flags & KDU_WINDOW_MODEL_LAYERS)?1:0);
  inst->limit = limit; 
  inst->tmin = tmin;
  inst->tmax = tmax;
  inst->cmin = (kdu_int16) cmin;
  inst->cmax = (kdu_int16) cmax;
  inst->rmin = (kdu_int16) rmin;
  inst->rmax = (kdu_int16) rmax;
  inst->pmin = pmin;
  inst->pmax = pmax;
}

/*****************************************************************************/
/*                  kdu_window_model::get_meta_instructions                  */
/*****************************************************************************/

int kdu_window_model::get_meta_instructions(kdu_long &bin_id, int buf[])
{
  buf[0] = buf[1] = 0;
  if (stateless && (bin_id < 0))
    return 0; // Cannot make untargeted requests in stateless mode
  int cnt=0;
  kdwm_instruction *ip, *ip_prev, *ip_next;
  for (ip_prev=NULL, ip=meta_head; ip != NULL; ip_prev=ip, ip=ip_next)
    { 
      ip_next = ip->next;
      assert(ip->absolute_bin_id);
      if (bin_id < 0)
        { 
          if (!ip->atomic)
            continue;
          assert(ip->bin_id >= 0);
        }
      else if ((bin_id != ip->bin_id) && (ip->bin_id >= 0))
        continue; // Not a match

      if (background_full && (cnt == 0))
        buf[cnt++] = -1; // Start by affirming that the cache is complete
      if (ip->subtractive)
        { // Place exclusive upper bound N in buf[1]
          assert(ip->limit >= 0);
          cnt = 2;
          if ((buf[1] <= 0) || (buf[1] > ip->limit))
            { 
              buf[1] = ip->limit+1;
              if ((buf[0] < 0) || (buf[0] >= buf[1]))
                buf[0]=buf[1]-1; // Decrease lower bound so range is non-empty
            }
        }
      else
        { // Place inclusive lower bound P in buf[0]
          if (ip->limit < 0)
            { cnt = 1; buf[0] = -1; buf[1] = 0; }
          else if ((buf[0] >= 0) && (ip->limit > buf[0]))
            { 
              cnt = (cnt<1)?1:cnt;
              buf[0] = ip->limit;
              if ((buf[1] > 0) && (buf[1] <= buf[0]))
                buf[1]=buf[0]+1; // Increase upper bound so range is non-empty
            }
        }
      if (ip->atomic)
        { // Strip atomic instructions
          if (ip_next == NULL)
            meta_tail = ip_prev;
          if (ip_prev != NULL)
            ip_prev->next = ip_next;
          else
            meta_head = ip_next;
          ip->next = free_instructions;
          free_instructions = ip;
          ip = ip_prev; // So loop variables get correctly updated
        }
      if (bin_id < 0)
        { 
          bin_id = ip->bin_id;
          break;
        }
    }
  return cnt;
}

/*****************************************************************************/
/*                 kdu_window_model::get_first_atomic_stream                 */
/*****************************************************************************/

int kdu_window_model::get_first_atomic_stream()
{
  kdwm_stream_context *sc;
  for (sc=contexts; sc != NULL; sc=sc->next)
    if (sc->is_atomic())
      { 
        kdwm_instruction *ip;
        for (ip=sc->head; ip != NULL; ip=ip->next)
          if (ip->atomic)
            return sc->smin;
      }
  return -1;
}

/*****************************************************************************/
/*        kdu_window_model::get_header_instructions (specific tnum)          */
/*****************************************************************************/

int kdu_window_model::get_header_instructions(int stream_idx, int tnum,
                                              int buf[])
{
  buf[0] = buf[1] = 0;
  int cnt=0;
  kdwm_stream_context *sc;
  for (sc=contexts; sc != NULL; sc = sc->next)
    { 
      if ((sc->smin >= 0) &&
          ((sc->smin > stream_idx) || (sc->smax < stream_idx)))
        continue;
      kdwm_instruction *ip, *ip_prev, *ip_next;
      for (ip_prev=NULL, ip=sc->head; ip != NULL; ip_prev=ip, ip=ip_next)
        { 
          ip_next = ip->next;
          if (!ip->absolute_bin_id)
            continue; // Not a match
          if (tnum < 0)
            { 
              if (ip->databin_class != KDU_MAIN_HEADER_DATABIN)
                continue; // Not a match
            }
          else
            { 
              if ((ip->databin_class != KDU_TILE_HEADER_DATABIN) ||
                  (ip->bin_id != (kdu_long) tnum))
                continue; // Not a match
            }
          if (background_full && (cnt == 0))
            buf[cnt++] = -1; // Start by affirming that the cache is complete
          if (ip->subtractive)
            { // Place exclusive upper bound N in buf[1]
              assert(ip->limit >= 0);
              cnt = 2;
              if ((buf[1] <= 0) || (buf[1] > ip->limit))
                { 
                  buf[1] = ip->limit+1;
                  if ((buf[0] < 0) || (buf[0] >= buf[1]))
                    buf[0]=buf[1]-1; // Decrease lower bound so range non-empty
                }
            }
          else
            { // Place inclusive lower bound P in buf[0]
              if (ip->limit < 0)
                { cnt = 1; buf[0] = -1; buf[1] = 0; }
              else if ((buf[0] >= 0) && (ip->limit > buf[0]))
                { 
                  cnt = (cnt<1)?1:cnt;
                  buf[0] = ip->limit;
                  if ((buf[1] > 0) && (buf[1] <= buf[0]))
                    buf[1]=buf[0]+1; // Increase upper bound so range non-empty
                }
            }
          if (ip->atomic && sc->is_atomic())
            { // Strip atomic instructions
              if (ip_next == NULL)
                { 
                  assert(sc->tail == ip);
                  sc->tail = ip_prev;
                }
              if (ip_prev != NULL)
                ip_prev->next = ip_next;
              else
                { 
                  assert(ip == sc->head);
                  sc->head = ip->next;
                }
              ip->next = free_instructions;
              free_instructions = ip;
              ip = ip_prev; // So loop variables get correctly updated
            }
        }
    }
  return cnt;
}

/*****************************************************************************/
/*         kdu_window_model::get_header_instructions (non-specific)          */
/*****************************************************************************/

int kdu_window_model::get_header_instructions(int stream_idx, int *tnum,
                                              int buf[])
{
  buf[0] = buf[1] = 0;
  if (stateless)
    return 0; // Cannot have non-specific requests in stateless mode
  assert(!background_full);
  int cnt=0;
  kdwm_stream_context *sc;
  for (sc=contexts; sc != NULL; sc = sc->next)
    { 
      if ((sc->smin != stream_idx) && (sc->smax != stream_idx))
        continue;
      assert(sc->is_atomic());
      kdwm_instruction *ip, *ip_prev, *ip_next;
      for (ip_prev=NULL, ip=sc->head; ip != NULL; ip_prev=ip, ip=ip_next)
        { 
          ip_next = ip->next;
          if ((!ip->absolute_bin_id) || !ip->atomic)
            continue; // Not a match
          if (ip->databin_class == KDU_MAIN_HEADER_DATABIN)
            *tnum = -1;
          else if (ip->databin_class == KDU_TILE_HEADER_DATABIN)
            *tnum = (int)(ip->bin_id);          
          else
            continue; // Not a match
          if (ip->subtractive)
            { // Place exclusive upper bound N in buf[1]
              assert(ip->limit >= 0);
              cnt = 2;
              buf[1] = ip->limit+1;
            }
          else
            { // Place inclusive lower bound P in buf[0]
              cnt = 1;
              buf[0] = ip->limit;
            }
          if (ip_next == NULL)
            { 
              assert(sc->tail == ip);
              sc->tail = ip_prev;
            }
          if (ip_prev != NULL)
            ip_prev->next = ip_next;
          else
            { 
              assert(ip == sc->head);
              sc->head = ip_next;
            }
          ip->next = free_instructions;
          free_instructions = ip;
          return cnt;
        }
    }
  return 0;
}

/*****************************************************************************/
/*         kdu_window_model::get_precinct_instructions (non-specific)        */
/*****************************************************************************/

int kdu_window_model::get_precinct_instructions(int stream_idx, int &tnum,
                                                int &cnum, int &rnum,
                                                kdu_long &pnum, int buf[])
{
  buf[0] = buf[1] = 0;
  if (stateless)
    return 0; // Cannot have non-specific requests in stateless mode
  assert(!background_full);
  int cnt=0;
  kdwm_stream_context *sc;
  for (sc=contexts; sc != NULL; sc = sc->next)
    { 
      if ((sc->smin != stream_idx) && (sc->smax != stream_idx))
        continue;
      assert(sc->is_atomic());
      kdwm_instruction *ip, *ip_prev, *ip_next;
      for (ip_prev=NULL, ip=sc->head; ip != NULL; ip_prev=ip, ip=ip_next)
        { 
          ip_next = ip->next;
          if ((ip->databin_class != KDU_PRECINCT_DATABIN) || !ip->atomic)
            continue;
          if (ip->absolute_bin_id)
            { tnum = cnum = rnum = -1; pnum = ip->bin_id; }
          else
            { tnum=ip->tmin; cnum=ip->cmin; rnum=ip->rmin; pnum=ip->pmin; }
          if (ip->subtractive)
            { // Place exclusive upper bound N in buf[1]
              assert(ip->limit >= 0);
              cnt = 2;
              buf[1] = ip->limit+2;
            }
          else
            { // Place inclusive lower bound P in buf[0]
              cnt = 1;
              buf[0] = ip->limit;
            }
          
          if (ip_next == NULL)
            { 
              assert(sc->tail == ip);
              sc->tail = ip_prev;
            }
          if (ip_prev != NULL)
            ip_prev->next = ip_next;
          else
            { 
              assert(ip == sc->head);
              sc->head = ip_next;
            }
          ip->next = free_instructions;
          free_instructions = ip;
          return cnt;
        }
    }
  return 0;  
}

/*****************************************************************************/
/*                   kdu_window_model::get_precinct_block                    */
/*****************************************************************************/

bool
  kdu_window_model::get_precinct_block(int stream_idx, int tnum, int cnum,
                                       int rnum, int t_across, int p_across,
                                       kdu_long id_base, kdu_long id_gap,
                                       kdu_dims region, int buf[])
{
  assert((region.pos.x >= 0) && (region.pos.y >= 0) &&
         ((region.pos.x+region.size.x) <= p_across));
  bool found_something=false;
  kdwm_stream_context *sc;
  for (sc=contexts; sc != NULL; sc = sc->next)
    { 
      if ((sc->smin >= 0) &&
          (sc->smin > stream_idx) || (sc->smax < stream_idx))
        continue; // Incompatible stream context
      kdwm_instruction *ip, *ip_prev, *ip_next;
      for (ip_prev=NULL, ip=sc->head; ip != NULL; ip_prev=ip, ip=ip_next)
        { 
          ip_next = ip->next;
          if (ip->databin_class != KDU_PRECINCT_DATABIN)
            continue;
          kdu_dims ip_region; // Relative to `buf'
          if (ip->absolute_bin_id)
            { 
              if (ip->bin_id < 0)
                { ip_region.size = region.size; }
              else
                { 
                  kdu_long delta = ip->bin_id - id_base;
                  kdu_long x = delta / id_gap;
                  if (delta != (x * id_gap))
                    continue; // Belongs to a different tile-component
                  kdu_long y = x / p_across;  x -= y*p_across;
                  y -= region.pos.y;  x -= region.pos.x;
                  if ((y < 0) || (y >= (kdu_long) region.size.y) ||
                      (x < 0) || (x >= (kdu_long) region.size.x))
                    continue;
                  ip_region.pos.x = (int) x; ip_region.pos.y = (int) y;
                  ip_region.size.x = ip_region.size.y = 1;
                }
            }
          else
            { 
              if ((ip->cmin >= 0) && ((ip->cmin > cnum) || (ip->cmax < cnum)))
                continue;
              if ((ip->rmin >= 0) && ((ip->rmin > rnum) || (ip->rmax < rnum)))
                continue;
              if (ip->tmin >= 0)
                { 
                  int y1 = (tnum - ip->tmin) / t_across; // tile rows above us
                  int y2 = (ip->tmax - tnum) / t_across; // tile rows below us
                  if ((y1 < 0) || (y2 < 0))
                    continue;
                  int x1 = (tnum-ip->tmin)-y1*t_across; // tile cols to left
                  int x2 = (ip->tmax-tnum)-y2*t_across; // tile cols to right
                  if ((x1 < 0) || (x2 < 0))
                    continue;
                }
              if (ip->pmin < 0)
                ip_region.size = region.size; // Affects all of `buf'
              else
                { 
                  ip_region.pos.y = (int)(ip->pmin / p_across);
                  ip_region.pos.x = (int)(ip->pmin - ip_region.pos.y *
                                          ((kdu_long) p_across));
                  ip_region.size.y = (int)(ip->pmax / p_across);
                  ip_region.size.x = (int)(ip->pmax - ip_region.size.y *
                                           ((kdu_long) p_across));
                    // Above, ip_region.size temporarily holds bottom-right pos
                  ip_region.size.x++; // Adjust `ip_region.size' just beyond
                  ip_region.size.y++; // bottom right pos wrt start of res.
                  ip_region.pos -= region.pos; // Make relative to `buf'
                  ip_region.size -= region.pos; // Also relative to `buf'
                  if (ip_region.pos.x < 0) ip_region.pos.x = 0;
                  if (ip_region.pos.y < 0) ip_region.pos.y = 0;
                  if (ip_region.size.x > region.size.x)
                    ip_region.size.x = region.size.x;
                  if (ip_region.size.y > region.size.y)
                    ip_region.size.y = region.size.y;
                  ip_region.size -= ip_region.pos;
                    // Above leaves `ip_region' with overlapping region wrt buf
                  if ((ip_region.size.x < 0) || (ip_region.size.y < 0))
                    continue;
                }
            }
          int *bp, m, n, gap_p;
          if (!found_something)
            { // Initialize `buf' for the first time
              if (background_full)
                { // Set `buf' to add whole of each bin to cache model first
                  for (bp=buf, m=region.size.y*region.size.x; m>0; m--, bp+=2)
                    { bp[0] = -1; bp[1] = 0; }
                }
              else
                { // Set `buf' to empty
                  for (bp=buf, m=region.size.y*region.size.x; m>0; m--, bp+=2)
                    { bp[0] = 0; bp[1] = 0; }
                }
              found_something = true;
            }
          gap_p = (region.size.x-ip_region.size.x)<<1;
          bp = buf + ((ip_region.pos.x+ip_region.pos.y*region.size.x)<<1);
          if (ip->subtractive)
            { // Place exclusive upper bound in buf_p[1]
              for (m=ip_region.size.y; m > 0; m--, bp+=gap_p)
                for (n=ip_region.size.x; n > 0; n--, bp+=2)
                  { 
                    int limit = ip->limit; // Value may change below
                    assert(limit >= 0);
                    if ((bp[1]^limit) & 1)
                      limit = 0; // Incompatible upper bounds -> empty cache
                    if ((bp[1] <= 0) || (bp[1] > limit))
                      { 
                        bp[1] = limit+2;
                        if (bp[0] < 0)
                          bp[0]=limit; // decr. lower bound so range non-empty
                        else if ((bp[0]^limit) & 1)
                          bp[0] = 0; // obliterate incompatible additive op
                        else if (bp[0] >= bp[1])
                          bp[0]=limit; // decr. lower bound so range non-empty
                      }
                  }
            }
          else
            { // Place inclusive lower bound in buf_p[0]
              int limit = ip->limit; // Value remains unchanged within loop
              assert(limit != 0); // Additive inst. with limit of 0 is no-op
              for (m=ip_region.size.y; m > 0; m--, bp+=gap_p)
                for (n=ip_region.size.x; n > 0; n--, bp+=2)
                  { 
                    if (limit < 0)
                      { bp[0] = -1; bp[1] = 0; continue; }
                    if ((bp[1] > 0) && ((bp[1]^limit) & 1))
                      continue; // Incompatible subtractive op obliterates us
                    if ((bp[0] >= 0) &&
                        (((bp[0]^limit) & 1) || (bp[0] < limit)))
                      { 
                        bp[0] = limit;
                        if ((bp[1] > 0) && (bp[1] <= bp[0]))
                          bp[1]=limit+2; // Inc. upper bound so range non-empty
                      }
                  }
            }
          if (ip->atomic && sc->is_atomic())
            { // Strip atomic instructions
              if (ip_next == NULL)
                { 
                  assert(sc->tail == ip);
                  sc->tail = ip_prev;
                }
              if (ip_prev != NULL)
                ip_prev->next = ip_next;
              else
                { 
                  assert(ip == sc->head);
                  sc->head = ip->next;
                }
              ip->next = free_instructions;
              free_instructions = ip;
              ip = ip_prev; // So loop variables get correctly updated
            }
        }  
    }  
  return found_something;
}

