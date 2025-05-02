/*
 *  xutoj.c,v 1.7 2002/03/24 01:25:13 hiroo Exp
 *  Canna: $Id: xutoj.c,v 1.3.2.1 2003/01/11 17:53:57 aida_s Exp $
 */

/*
 * FreeWnn is a network-extensible Kana-to-Kanji conversion system.
 * This file is part of FreeWnn.
 * 
 * Copyright Kyoto University Research Institute for Mathematical Sciences
 *                 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright OMRON Corporation. 1987, 1988, 1989, 1990, 1991, 1992, 1999
 * Copyright ASTEC, Inc. 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright FreeWnn Project 1999, 2000, 2002
 *
 * Maintainer:  FreeWnn Project   <freewnn@tomo.gr.jp>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define NEED_CR

#if STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif /* STDC_HEADERS */

#include "commonhd.h"
#include "wnn_config.h"
#include "wnn_os.h"

#define ECNS_IS_UCNS 1          /* The trust CNS is CNS11643 based on ISO2022,
                                   but the CNS is binded on EUC */

#define NON_LIMIT       0x7FFFFFFF

#define G0      0
#define G1      1
#define G2      2
#define G3      3
#define SS      4
#define GL      1
#define GR      2
#define LS0     0x0f
#define LS1     0x0e
#define LS1R    0x7e
#define LS2     0x6e
#define LS2R    0x7d
#define LS3     0x6f
#define LS3R    0x7c
#define SS2     0x8e
#define SS3     0x8f


#define CS_MASK         0x8080
#define CS0_MASK        0x0000
#define CS1_MASK        0x8080
#define CS2_MASK        0x0080
#define CS3_MASK        0x8000

#define CS1     0
#define CS2     1
#define CS3     2

#define UJIS_CSWIDTH    "2,1,2"
#define UGB_CSWIDTH     "2,1,2"
#define UKSC_CSWIDTH    "2,1,2"

typedef struct _CSWidthTable
{
  int cs1, cs2, cs3;
}
CSWidthTable;

typedef struct _cswidth_name_struct
{
  char *lang;
  char *name;
  char *def_name;
}
cswidth_name_struct;

typedef struct _DesignateTable
{
  unsigned char *code;
  unsigned int mask;
}
DesignateTable;

static int _etc_cs[3] = { 2, 1, 2 };
static int _etc_cs_len[3] = { 2, 1, 2 };
static int cs_mask[3] = { 0x8080, 0x0080, 0x8000 };

static int default_glr_mode[3] = { 0, G0, G1 };
static int save_glr_mode[2];
static int default_gn_len[4] = { 1, 1, 1, 2 };
static unsigned int default_gn_mask[4] = { 0x00, 0x80, 0x80, 0x8000 };
static DesignateTable default_designate[4] = {
  {(unsigned char *) "(B", 0x0},
  {(unsigned char *) NULL, 0x0}
};

static int *glr_mode = default_glr_mode;
static int *gn_len = default_gn_len;
static unsigned int *gn_mask = default_gn_mask;

static unsigned char save_seq[6] = { '\0' };
static int save_seq_len = 0;
static int pending_esc = 0;
static unsigned char pending = '\0';
static w_char pending_mask = (w_char) 0;

static DesignateTable *designate = default_designate;

#ifdef  JAPANESE
static DesignateTable JIS_designate[] = {
  {(unsigned char *) "(B", 0x0},
  {(unsigned char *) "(J", 0x0},
  {(unsigned char *) "(I", 0x80},
  {(unsigned char *) "$B", 0x8080},
  {(unsigned char *) "$(B", 0x8080},
  {(unsigned char *) ")I", 0x80},
  {(unsigned char *) "$)B", 0x8080},
  {(unsigned char *) "$)D", 0x8000},
  {(unsigned char *) "$(D", 0x8000},
  {(unsigned char *) NULL, 0x0}
};
#endif /* JAPANESE */

#ifdef  CHINESE
#ifndef ECNS_IS_UCNS
static DesignateTable CNS_designate[] = {
  {(unsigned char *) "(B", 0x0},
  {(unsigned char *) "$)0", 0x8080},
  {(unsigned char *) "$*1", 0x8000},
  {NULL, 0x0}
};
#endif /* ECNS_IS_UCNS */
#endif /* CHINESE */

#ifdef  KOREAN
static DesignateTable KSC_designate[] = {
  {(unsigned char *) "(B", 0x0},
  {(unsigned char *) "$(C", 0x8080},
  {(unsigned char *) "$)C", 0x8080},
  {(unsigned char *) NULL, 0x0}
};
#endif /* KOREAN */

#if defined(JAPANESE) || defined(CHINESE) || defined(KOREAN)
static w_char tmp_w_buf[1000];
#endif

static void
set_gn (dg)
     DesignateTable *dg;
{
  register char *p = (char *) dg->code;
  register int len = 1, gn = 0;

  if (!strcmp (p, "$B"))
    {                           /* JIS */
      gn_len[0] = 2;
      gn_mask[0] = dg->mask;
      return;
    }
  if (*p == '$')
    {
      len = 2;
      p++;
    }
  if (*p >= '(' && *p <= '+')
    gn = *p - '(';
  else if (*p >= '-' && *p <= '/')
    gn = *p - '+';
  else
    return;
  gn_len[gn] = len;
  gn_mask[gn] = dg->mask;
}

static int
check_designate (ec, eend, ret_buf)
     unsigned char *ec, *eend, **ret_buf;
{
  register unsigned char *c = ec;
  register int i, j, ok = 0;

  *ret_buf = NULL;
  for (i = save_seq_len; c < eend; c++)
    {
      ok = 0;
      save_seq[i++] = *c;
      save_seq[i] = '\0';
      for (j = 0; designate[j].code; j++)
        {
          if (!strncmp ((char *) save_seq, (char *) designate[j].code, i))
            {
              if (i == strlen ((char *) designate[j].code))
                {
                  set_gn (&designate[j]);
                  save_seq_len = 0;
                  return (c - ec);
                }
              ok = 1;
              break;
            }
        }
      if (ok == 0)
        {
          *ret_buf = save_seq;
          save_seq_len = 0;
          return (c - ec);
        }
    }
  save_seq_len = i;
  return (c - ec - 1);
}

int
flush_designate (buf)
     w_char *buf;
{
  register w_char *c = buf;
  register int i;

  if (pending_esc)
    {
      *c++ = ESC;
      pending_esc = 0;
      return (1);
    }
  if (save_seq_len == 0)
    return (0);
  *c++ = ESC;
  for (i = 0; i < save_seq_len; i++)
    {
      *c++ = save_seq[i];
    }
  save_seq_len = 0;
  return ((char *) c - (char *) buf);
}

int
extc_to_intc (intc, extc, esiz)
     w_char *intc;
     unsigned char *extc;
     int esiz;
{
  unsigned char *eend = extc + esiz;
  register unsigned char *ec = extc;
  register w_char *ic = intc;
  register int LorR = 0, i;
  w_char tmp;
  int ret, len;
  unsigned char *ret_buf;
  register unsigned char *p;

  for (; ec < eend; ec++)
    {
      if (pending_esc)
        {
          pending_esc = 0;
          goto ESC_SWITCH;
        }
      if (pending)
        {
          *ic++ = ((pending << 8 | *ec) & 0x7f7f) | pending_mask;
          pending = '\0';
          continue;
        }
      switch (*ec)
        {
#ifdef JIS7
        case LS0:
          glr_mode[GL] = G0;
          break;
        case LS1:
          glr_mode[GL] = G1;
          break;
#endif /* JIS7 */
        case SS2:
          save_glr_mode[GL] = glr_mode[GL];
          glr_mode[GL] = (G2 | SS);
          break;
        case SS3:
          save_glr_mode[GL] = glr_mode[GL];
          glr_mode[GL] = (G3 | SS);
          break;
        case ESC:
          if (++ec == eend)
            {
              pending_esc = 1;
              break;
            }
        ESC_SWITCH:
          switch (*ec)
            {
#ifndef CANNA /* This should be removed even if not Canna */
            case LS1R:
              glr_mode[GR] = G1;
              break;
            case LS2:
              glr_mode[GL] = G2;
              break;
            case LS2R:
              glr_mode[GR] = G2;
              break;
            case LS3:
              glr_mode[GL] = G3;
              break;
            case LS3R:
              glr_mode[GR] = G3;
              break;
#endif /* CANNA */
            default:
              ret = check_designate (ec, eend, &ret_buf);
              ec += ret;
              if (ret_buf)
                {
                  *ic++ = ESC;
                  for (p = ret_buf; *p; p++)
                    *ic++ = *p;
                }
              break;
            }
          break;
        default:
          LorR = 0;
          if (*ec >= 0x20 && *ec <= 0x7f)
            {                   /* GL */
              LorR = GL;
            }
          else if (*ec >= 0xa0 && *ec <= 0xff)
            {                   /* GR */
              LorR = GR;
            }
          if (LorR)
            {
              len = gn_len[(glr_mode[LorR] & 0x3)];
              if ((ec + len) > eend)
                {
                  pending = *ec;
                  pending_mask = gn_mask[(glr_mode[LorR] & 0x3)];
                }
              else
                {
                  for (tmp = (w_char) 0, i = 0; i < len; i++)
                    {
                      tmp = ((tmp << 8 | *ec++) & 0x7f7f) | gn_mask[(glr_mode[LorR] & 0x3)];
                    }
                  if (len)
                    ec -= 1;
                  *ic++ = tmp;
                  if (glr_mode[LorR] & SS)
                    glr_mode[LorR] = save_glr_mode[LorR];
                }
            }
          else
            {
              *ic++ = *ec;
            }
        }
    }
  return ((char *) ic - (char *) intc);
}

int
through (x, y, z)
     char *x, *y;
     int z;
{
  bcopy (y, x, z);
  return z;
}

int
ibit8_to_ebit8 (ebit8, ibit8, ibsiz)
     unsigned char *ebit8;
     w_char *ibit8;
     int ibsiz;
{
  register unsigned char *eb = ebit8;
  register w_char *ib = ibit8;

  for (; ibsiz > 0; ibsiz -= sizeof (w_char))
    {
      *eb++ = *ib++ & 0xff;
    }
  return ((char *) eb - (char *) ebit8);
}

/** cswidth functions **/
unsigned int
create_cswidth (s)
     char *s;
{
  char tmp[2];
  int cs = 0, css = 0, i;

  if (!s || !*s)
    return (0);

  tmp[0] = tmp[1] = '\0';
  for (i = 2; i >= 0; i--)
    {
      tmp[0] = *s;
      cs = atoi (tmp);
      if (cs > 0 && cs < 3)
        css = (cs << (i * 8 + 4)) | css;
      if (!*++s)
        {
          if (cs > 0 && cs < 3)
            css = (cs << (i * 8)) | css;
          break;
        }
      if (*s == ':')
        {
          if (!*++s)
            {
              if (cs > 0 && cs < 3)
                css = (cs << (i * 8)) | css;
              break;
            }
          tmp[0] = *s;
          cs = atoi (tmp);
          s++;
        }
      if (cs > 0 && cs < 3)
        css = (cs << (i * 8)) | css;
      if (!*s || *s != ',' || !*++s)
        break;
    }
  return (css);
}

void
set_cswidth (id)
     register unsigned int id;
{
  _etc_cs[CS1] = (id >> 20) & 0xf;
  _etc_cs[CS2] = (id >> 12) & 0xf;
  _etc_cs[CS3] = (id >> 4) & 0xf;
  _etc_cs_len[CS1] = (id >> 16) & 0xf;
  _etc_cs_len[CS2] = (id >> 8) & 0xf;
  _etc_cs_len[CS3] = id & 0xf;
  return;
}

static cswidth_name_struct cs_width_name[] = {
  {WNN_J_LANG, "JCSWIDTH", "2,1,2"},
  {WNN_C_LANG, "CCSWIDTH", "2,1,2"},
  {WNN_K_LANG, "KCSWIDTH", "2"},
  {WNN_T_LANG, "TCSWIDTH", "2,1,2"},
  {NULL, NULL}
};

char *
get_cswidth_name (lang)
     register char *lang;
{
  register cswidth_name_struct *p;
  register char *name;
  extern char *getenv ();

  if (!lang || !*lang)
    {
      return (getenv ("CSWIDTH"));
    }

  for (p = cs_width_name; p->lang; p++)
    {
      if (!strncmp (lang, p->lang, strlen (lang)))
        {
          if ((name = getenv (p->name)) != NULL)
            {
              return (name);
            }
          else if ((name = getenv ("CSWIDTH")) != NULL)
            {
              return (name);
            }
          else
            {
              return (p->def_name);
            }
        }
    }
  return (NULL);
}

int
get_cswidth (cs)
     int cs;
{
  return (_etc_cs[cs]);
}

int
get_cswidth_by_char (c)
     register unsigned char c;
{
  if (c < SS2 || (c < 0xa0 && c > SS3))
    return (1);
  if (c == SS2)
    return (_etc_cs[CS2] + 1);
  if (c == SS3)
    return (_etc_cs[CS3] + 1);
  return (_etc_cs[CS1]);
}

int
get_cs_mask (cs)
     int cs;
{
  return (cs_mask[cs]);
}

int
columnlen (eeuc)
     unsigned char *eeuc;
{
  register int n = 0;
  register unsigned char *c, x;
  register int cs_id;

  for (c = eeuc; *c;)
    {
      x = *c;
      if (x & 0x80)
        {
          cs_id = ((x == SS2) ? CS2 : ((x == SS3) ? CS3 : CS1));
          if (cs_id == CS2 || cs_id == CS3)
            c++;
          n += _etc_cs_len[cs_id];
          c += _etc_cs[cs_id];
        }
      else
        {
          n++;
          c++;
        }
    }
  return (n);
}

int
columnlen_w (ieuc)
     w_char *ieuc;
{
  register int n = 0;
  register w_char *c, x;
  register int cs_id, mask;

  for (c = ieuc; *c; c++)
    {
      x = *c;
      mask = x & CS_MASK;
      if (mask == CS0_MASK)
        {
          n++;
        }
      else
        {
          cs_id = (mask == cs_mask[CS3]) ? CS3 : ((mask == cs_mask[CS2]) ? CS2 : CS1);
          n += _etc_cs_len[cs_id];
        }
    }
  return (n);
}

int
ieuc_to_eeuc (eeuc, ieuc, iesiz)
     unsigned char *eeuc;
     w_char *ieuc;
     int iesiz;
{
  register int x;
  register w_char *ie;
  register unsigned char *ee;
  register int cs_id, mask, non_limit = 0;
  ie = ieuc;
  ee = eeuc;

  if (iesiz == -1)
    non_limit = 1;
  for (; (non_limit ? (*ie) : (iesiz > 0)); iesiz -= sizeof (w_char))
    {
      x = *ie++;
      mask = x & CS_MASK;
      if (mask == CS0_MASK || x == 0xffff)
        {
          *ee++ = x;
        }
      else
        {
          cs_id = (mask == cs_mask[CS3]) ? CS3 : ((mask == cs_mask[CS2]) ? CS2 : CS1);
          if (_etc_cs[cs_id] <= 0)
            continue;
          if (cs_id == CS2)
            *ee++ = SS2;
          else if (cs_id == CS3)
            *ee++ = SS3;
          if (_etc_cs[cs_id] > 1)
            *ee++ = (x >> 8) | 0x80;
          if (_etc_cs[cs_id] > 0)
            *ee++ = (x & 0xff) | 0x80;
        }
    }
  return ((char *) ee - (char *) eeuc);
}


int
eeuc_to_ieuc (ieuc, eeuc, eesiz)
     w_char *ieuc;
     unsigned char *eeuc;
     register int eesiz;
{
  register unsigned char x;
  register w_char *ie;
  register unsigned char *ee;
  register int cs_id, non_limit = 0;
  ie = ieuc;
  ee = eeuc;

  if (eesiz == -1)
    non_limit = 1;
  for (; (non_limit ? (*ee) : (eesiz > 0));)
    {
      x = *ee++;
      if (x > 0x9f || x == SS2 || x == SS3)
        {
          cs_id = ((x == SS2) ? CS2 : ((x == SS3) ? CS3 : CS1));
          if (cs_id == CS2 || cs_id == CS3)
            x = *ee++;
          if (_etc_cs[cs_id] <= 0)
            continue;
          if (_etc_cs[cs_id] > 1)
            {
              *ie = (w_char) (x & 0x7f) << 8;
              x = *ee++;
            }
          else
            {
              *ie = (w_char) 0;
            }
          *ie |= (x & 0x7f);
          *ie++ |= cs_mask[cs_id];
          eesiz -= _etc_cs[cs_id] + 1;
        }
      else
        {
          *ie++ = x;
          eesiz--;
        }
    }
  return ((char *) ie - (char *) ieuc);
}

#ifdef nodef
void
wnn_delete_ss2 (s, n)
     register unsigned int *s;
     register int n;
{
  register unsigned int x;

  for (; n != 0 && (x = *s); n--, s++)
    {
      if ((x & 0xff00) == 0x8e00)
        *s &= ~0xff00;
      if (x == 0xffffffff)
        break;
    }
}
#endif

void
wnn_delete_w_ss2 (s, n)
     register w_char *s;
     register int n;
{
  register w_char x;

  for (; n != 0 && (x = *s); n--, s++)
    {
      if ((x & 0xff00) == 0x8e00)
        *s &= ~0xff00;
    }
}

#ifdef nodef
int
wnn_byte_count (in)
     register int in;
{
  return (((in < 0xa0 && in != 0x00 && in != 0x8e) || in == 0xff) ? 1 : 2);
}
#endif

#define ASCII           0

#ifdef  JAPANESE
#define HANKAKU_JIS_IN  '\016'
#define HANKAKU_JIS_OUT '\017'

#define HANKAKU_JIS     2
#define ZENKAKU_JIS     1
#define ZENKAKU_JIS_HOJYO       3

static unsigned char *j;
static w_char *iu;
static unsigned char *eu;
static unsigned char *sj;
static unsigned char tmp_buf[2000];

static void
putj (x)
     int x;
{
  *j++ = x;
}

static void
puteu (x)
     int x;
{
  *eu++ = x;
}

static void
putsj (x)
     int x;
{
  *sj++ = x;
}

static void
putsjw (x)
     int x;
{
  *sj++ = x >> 8;
  *sj++ = x;
}

static int oj_mode = ASCII;     /* 出力時のｊｉｓコードのモード */
static int jtosj ();
extern int eujis_to_iujis ();

/* convert JIS code to shift-JIS code */
static int
jtosj (high, low)
     unsigned high, low;
{
  if (high & 1)
    low += 0x1f;
  else
    low += 0x7d;
  if (low >= 0x7f)
    low++;
  high = ((high - 0x21) >> 1) + 0x81;
  if (high > 0x9f)
    high += 0x40;
  return ((high << 8) | low);
}

/* convert shift-JIS to JIS code */
static int
sjtoj (high, low)
     register unsigned high, low;
{
  high -= (high <= 0x9f) ? 0x71 : 0xb1;
  high = high * 2;
  if (low > 0x7f)
    low--;
  if (low >= 0x9e)
    {
      high += 2;
      low -= 0x7d;
    }
  else
    {
      high++;
      low -= 0x1f;
    }
  return ((high << 8) | low);
}

static void
jis_change_mode (mode, new_mode)
     int *mode;
     int new_mode;
{
  if (*mode == new_mode)
    return;
  switch (*mode)
    {
    case ZENKAKU_JIS:
    case ZENKAKU_JIS_HOJYO:
      /* designate ISO-8859-1 rather than JIS X 0201 */
      /* putj('\033'); putj('('); putj('J');break; */
      putj ('\033');
      putj ('(');
      putj ('B');
      break;
#ifdef  JIS7
    case HANKAKU_JIS:
      putj (HANKAKU_JIS_OUT);
      break;
#endif /* JIS7 */
    default:;
    }
  *mode = new_mode;
  switch (new_mode)
    {
    case ZENKAKU_JIS:
      putj ('\033');
      putj ('$');
      putj ('B');
      break;
    case ZENKAKU_JIS_HOJYO:
      putj ('\033');
      putj ('$');
      putj ('(');
      putj ('D');
      break;
#ifdef  JIS7
    case HANKAKU_JIS:
      putj (HANKAKU_JIS_IN);
      break;
#endif /* JIS7 */
    default:;
    }
}

#ifdef  JIS7
/*      内部 U-jis を 7bit jis コードに変換します
        文字列の長さを返します                  */
extern int
iujis_to_jis (jis, iujis, iusiz)
     unsigned char *jis;        /*      jisコードになったものをおくbuf  */
     w_char *iujis;             /*      iujisコードのものをおいてくるbuf */
     int iusiz;                 /*      iujis の大きさ                  */
{
  int x;
  j = jis;
  iu = iujis;
  for (; iusiz > 0; iusiz -= sizeof (w_char))
    {
      x = *iu++;
      if (((x & 0xFF00) == 0x8E00) || ((x & 0xFF80) == 0x80))
        {
          jis_change_mode (&oj_mode, HANKAKU_JIS);
          putj (x & 0x7f);
        }
      else if ((x & 0x8080) == 0x8080)
        {
          jis_change_mode (&oj_mode, ZENKAKU_JIS);
          putj ((x >> 8) & 0x7f);
          putj (x & 0x7f);
        }
      else if (x & 0x8000)
        {
          jis_change_mode (&oj_mode, ZENKAKU_JIS_HOJYO);
          putj ((x >> 8) & 0x7f);
          putj (x & 0x7f);
        }
      else
        {
          jis_change_mode (&oj_mode, ASCII);
          putj (x);
        }
    }
  jis_change_mode (&oj_mode, ASCII);
  return (j - jis);
}
#endif /* JIS7 */

/*      内部 U-jis を 8bit jis コードに変換します
        文字列の長さを返します                  */
extern int
iujis_to_jis8 (jis, iujis, iusiz)
     unsigned char *jis;        /*      jisコードになったものをおくbuf  */
     w_char *iujis;             /*      iujisコードのものをおいてくるbuf */
     int iusiz;                 /*      iujis の大きさ                  */
{
  int x;
  j = jis;
  iu = iujis;
  for (; iusiz > 0; iusiz -= sizeof (w_char))
    {
      x = *iu++;
      if (((x & 0xFF00) == 0x8E00) || ((x & 0xFF80) == 0x80))
        {
          jis_change_mode (&oj_mode, ASCII);
          putj (x & 0xff);
        }
      else if ((x & 0x8080) == 0x8080)
        {
          jis_change_mode (&oj_mode, ZENKAKU_JIS);
          putj ((x >> 8) & 0x7f);
          putj (x & 0x7f);
        }
      else if (x & 0x8000)
        {
          jis_change_mode (&oj_mode, ZENKAKU_JIS_HOJYO);
          putj ((x >> 8) & 0x7f);
          putj (x & 0x7f);
        }
      else
        {
          jis_change_mode (&oj_mode, ASCII);
          putj (x);
        }
    }
  jis_change_mode (&oj_mode, ASCII);
  return (j - jis);
}


#ifdef  JIS7
/*      外部 U-jis を 7bit jis コードに変換します       */
extern int
eujis_to_jis (jis, eujis, eusiz)
     unsigned char *jis, *eujis;
     int eusiz;
{
  static int kanji1 = 0;
  static char kanji1_code = 0;
  /* 0: normal
     1: get SS2
     2: get kanji 1 byte */
  /*
     int oj_mode;
   */
  int x;
  j = jis;
  eu = eujis;
  /* ADD KURI */
  if (kanji1 != 0)
    {
      if (kanji1 == 2)
        {
          putj (kanji1_code & 0x7f);
        }
      putj (*eu & 0x7f);
      eusiz -= sizeof (char);
      kanji1 = 0;
    }
  /* ADD KURI end */
  /*
     for(oj_mode=ASCII;eusiz>0;eusiz-=sizeof(char)){
   */
  for (; eusiz > 0; eusiz -= sizeof (char))
    {
      x = *eu++;
      if ((x & 0xFF) == 0x8E)
        {
          jis_change_mode (&oj_mode, HANKAKU_JIS);
          if (eusiz > 1)
            {
              putj (*eu++ & 0x7f);
              eusiz -= sizeof (char);
            }
          else
            {
              kanji1 = 1;
            }
        }
      else if (x & 0x80)
        {
          jis_change_mode (&oj_mode, ZENKAKU_JIS);
          if (eusiz > 1)
            {
              putj (x & 0x7f);
              putj (*eu++ & 0x7f);
              eusiz -= sizeof (char);
            }
          else
            {
              kanji1 = 2;
              kanji1_code = x & 0x7f;
            }
        }
      else
        {
          jis_change_mode (&oj_mode, ASCII);
          putj (x);
        }
    }
  if (kanji1 == 0)
    jis_change_mode (&oj_mode, ASCII);
  return (j - jis);
}
#endif /* JIS7 */

/*      外部 U-jis を 8bit jis コードに変換します       */
extern int
eujis_to_jis8 (jis, eujis, eusiz)
     unsigned char *jis, *eujis;
     int eusiz;
{
  static int kanji1 = 0;
  static unsigned char kanji1_code = 0;
  /* 0: normal
     1: get SS2
     2: get kanji 1 byte */
  /*
     int oj_mode;
   */
  int x;
  j = jis;
  eu = eujis;
  /* ADD KURI */
  if (kanji1 != 0)
    {
      if (kanji1 == 2)
        {
          putj (kanji1_code & 0x7f);
          putj (*eu & 0x7f);
        }
      else
        {
          putj (*eu);
        }
      eusiz -= sizeof (char);
      kanji1 = 0;
      eu++;
    }
  /* ADD KURI end */
  /*
     for(oj_mode=ASCII;eusiz>0;eusiz-=sizeof(char)){
   */
  for (; eusiz > 0; eusiz -= sizeof (char))
    {
      x = *eu++;
      if ((x & 0xFF) == 0x8E)
        {
          jis_change_mode (&oj_mode, ASCII);
          if (eusiz > 1)
            {
              putj (*eu++);
              eusiz -= sizeof (char);
            }
          else
            {
              kanji1 = 1;
            }
        }
      else if (x & 0x80)
        {
          jis_change_mode (&oj_mode, ZENKAKU_JIS);
          if (eusiz > 1)
            {
              putj (x & 0x7f);
              putj (*eu++ & 0x7f);
              eusiz -= sizeof (char);
            }
          else
            {
              kanji1 = 2;
              kanji1_code = x;
            }
        }
      else
        {
          jis_change_mode (&oj_mode, ASCII);
          putj (x);
        }
    }
  if (kanji1 == 0)
    jis_change_mode (&oj_mode, ASCII);
  return (j - jis);
}

/*      内部 U-jis を 外部 U-jis コードに変換します     */
extern int
iujis_to_eujis (eujis, iujis, iusiz)
     unsigned char *eujis;
     w_char *iujis;
     int iusiz;
{
  static int first = 0;
  static unsigned int cswidth_id;

  if (first == 0)
    {
      cswidth_id = create_cswidth (UJIS_CSWIDTH);
      first++;
    }
  set_cswidth (cswidth_id);
  return (ieuc_to_eeuc (eujis, iujis, iusiz));
}

int
jis_to_eujis (eujis, jis, jsiz)
     unsigned char *eujis, *jis;
     int jsiz;
{
  int len;

  designate = JIS_designate;
  len = extc_to_intc (tmp_w_buf, jis, jsiz);
  return (iujis_to_eujis (eujis, tmp_w_buf, len));
}

/*
 *      Shifted JIS
 */

/*      外部 U-jis を S-jis コードに変換します
        文字列の長さを返します                  */
extern int
eujis_to_sjis (sjis, eujis, eusiz)
     unsigned char *sjis;       /*      sjisコードになったものをおくbuf */
     unsigned char *eujis;      /*      eujisコードのものをおいてくるbuf */
     int eusiz;                 /*      eujis の大きさ                  */
{
  register int x;
  int save = 0;
  sj = sjis;
  eu = eujis;
  if (save && eusiz > 0)
    {
      if (save == 0x8e)
        {
          putsj (*eu++ | 0x80);
        }
      else
        {
          putsjw (jtosj (save & 0x7F, *eu++ & 0x7F));
        }
      eusiz--;
    }
  for (; eusiz > 0;)
    {
      x = *eu++;
      eusiz--;
      if (x & 0x80)
        {
          if (eusiz <= 0)
            {
              save = x;
              break;
            }
          if (x == 0x8e)
            {
              putsj (*eu++ | 0x80);
            }
          else
            {
              putsjw (jtosj (x & 0x7F, *eu++ & 0x7F));
            }
          eusiz--;
        }
      else
        {
          putsj (x);
        }
    }
  return (sj - sjis);
}

/*      内部 U-jis を S-jis コードに変換します
        文字列の長さを返します                  */
extern int
iujis_to_sjis (sjis, iujis, iusiz)
     unsigned char *sjis;       /*      sjisコードになったものをおくbuf */
     w_char *iujis;             /*      iujisコードのものをおいてくるbuf */
     int iusiz;                 /*      iujis の大きさ                  */
{
  register int x;
  sj = sjis;
  iu = iujis;
  for (; iusiz > 0; iusiz -= sizeof (w_char))
    {
      if ((x = *iu++) & 0xff00)
        {
          if ((x & 0xff00) == 0x8e00)
            {
              putsj ((x & 0xff) | 0x80);
            }
          else
            {
              putsjw (jtosj ((x >> 8) & 0x7f, x & 0x7f));
            }
        }
      else
        {
          putsj (x);
        }
    }
  return (sj - sjis);
}

int
sjis_to_iujis (iujis, sjis, ssiz)
     w_char *iujis;             /* iujisコードになったものをおくbuf */
     unsigned char *sjis;       /* sjisコードのものをおいてくるbuf */
     int ssiz;                  /* sjis の大き */
{
  register int x;
  int save = 0;
  sj = sjis;
  iu = iujis;
  if (save && ssiz > 0)
    {
      *iu++ = (sjtoj (save, *sj++) | 0x8080);
      ssiz--;
      save = 0;
    }
  for (; ssiz > 0;)
    {
      x = *sj++;
      ssiz--;
      if (x & 0x80)
        {
          if (ssiz <= 0)
            {
              save = x;
              break;
            }
          *iu++ = ((sjtoj (x, *sj++)) | 0x8080);
          ssiz--;
        }
      else
        {
          *iu++ = (x);
        }
    }
  return ((char *) iu - (char *) iujis);
}

int
sjis_to_eujis (eujis, sjis, ssiz)
     unsigned char *eujis;      /*      eujisコードになったものをおくbuf        */
     unsigned char *sjis;       /*      sjisコードのものをおいてくるbuf */
     int ssiz;                  /*      sjis の大きさ                   */
{
  register int x;
  unsigned char *sj;
  int save = 0;
  sj = sjis;
  eu = eujis;
  if (save && ssiz > 0)
    {
      x = (sjtoj (save, *sj++) | 0x8080);
      puteu (x >> 8);
      puteu (x);
      ssiz--;
      save = 0;
    }
  for (; ssiz > 0;)
    {
      x = *sj++;
      ssiz--;
      if (x & 0x80)
        {
          if (ssiz <= 0)
            {
              save = x;
              break;
            }
          x = (sjtoj (x, *sj++) | 0x8080);      /* 変えました KUWA */
          puteu (x >> 8);
          puteu (x);
          ssiz--;
        }
      else
        {
          puteu (x);
        }
    }
  return (eu - eujis);
}

#ifdef  JIS7
int
sjis_to_jis (jis, sjis, siz)
     unsigned char *jis, *sjis;
     int siz;
{
  int len;
  len = sjis_to_eujis (tmp_buf, sjis, siz);
  return (eujis_to_jis (jis, tmp_buf, len));
}
#endif /* JIS7 */

int
sjis_to_jis8 (jis, sjis, siz)
     unsigned char *jis, *sjis;
     int siz;
{
  int len;
  len = sjis_to_eujis (tmp_buf, sjis, siz);
  return (eujis_to_jis8 (jis, tmp_buf, len));
}

int
jis_to_iujis (iujis, jis, jsiz)
     w_char *iujis;
     unsigned char *jis;
     int jsiz;
{
  designate = JIS_designate;
  return (extc_to_intc (iujis, jis, jsiz));
}

int
jis_to_sjis (sjis, jis, siz)
     unsigned char *sjis, *jis;
     int siz;
{
  int len;
  len = jis_to_iujis (tmp_w_buf, jis, siz);
  return (iujis_to_sjis (sjis, tmp_w_buf, len));
}

int
eujis_to_iujis (iujis, eujis, eusiz)
     w_char *iujis;
     unsigned char *eujis;
     int eusiz;
{
  static int first = 0;
  static unsigned int cswidth_id;

  if (first == 0)
    {
      cswidth_id = create_cswidth (UJIS_CSWIDTH);
      first++;
    }
  set_cswidth (cswidth_id);
  return (eeuc_to_ieuc (iujis, eujis, eusiz));
}

#endif /* JAPANESE */

#ifdef  CHINESE

#define CNS11643_1      2
#define CNS11643_2      1

/* The following facts are helpful for understanding:
        * _W,           means Wei in Pinyin
        * _Q,           means Wei in Pinyin. 
        * 0x5e = 94,    num of wchar in one _Q at 1xxxxxxx 1xxxxxxx hand
        * 0x3f = 63,    Num of Wchar in one _Q at 1xxxxxxx 0xxxxxxx hand 
        * Almost all the numbers in HEX are given for showing the _Q or _W
*/

#define CNS_NUM_Q               0x5e    /* num of wchar in one _Q at hand */
#define BIG5_NUM_11_Q           0x5e    /* num of wchar in one _Q at 1xxxxxxx 1xxxxxxx hand */
#define BIG5_NUM_10_Q           0x3f    /* num of wchar in one _Q at 1xxxxxxx 0xxxxxxx hand */

#define CNS_HANZI_11_START_Q    0x24    /* At 1xxxxxxx 1xxxxxxx hand */
#define CNS_HANZI_11_START_W    0x01    /* At 1xxxxxxx 1xxxxxxx hand */
#define CNS_HANZI_11_END_Q      0x5D    /* At 1xxxxxxx 1xxxxxxx hand */
#define CNS_HANZI_11_END_W      0x2B    /* At 1xxxxxxx 1xxxxxxx hand */
#define CNS_HANZI_11_START_QM   0x01    /* At 1xxxxxxx 1xxxxxxx hand */
#define CNS_HANZI_11_END_QM     0x05    /* At 1xxxxxxx 1xxxxxxx hand */

#define CNS_HANZI_10_START_Q    0x01    /* At 1xxxxxxx 0xxxxxxx hand */
#define CNS_HANZI_10_START_W    0x01    /* At 1xxxxxxx 0xxxxxxx hand */
#define CNS_HANZI_10_END_Q      0x52    /* At 1xxxxxxx 0xxxxxxx hand */
#define CNS_HANZI_10_END_W      0x24    /* At 1xxxxxxx 0xxxxxxx hand */

#define BIG5_HANZI_11_START_Q   0x04    /* At 1xxxxxxx 1xxxxxxx hand */
#define BIG5_HANZI_11_START_W   0x01    /* At 1xxxxxxx 1xxxxxxx hand */
#define BIG5_HANZI_11_END_Q     0x25    /* At 1xxxxxxx 1xxxxxxx hand */
#define BIG5_HANZI_11_END_W     0x5e    /* At 1xxxxxxx 1xxxxxxx hand */
#define BIG5_HANZI_11_START_QM  0x01    /* At 1xxxxxxx 1xxxxxxx hand */
#define BIG5_HANZI_11_END_QM    0x03    /* At 1xxxxxxx 1xxxxxxx hand */

#define BIG5_HANZI_10_START_Q   0x04    /* At 1xxxxxxx 0xxxxxxx hand */
#define BIG5_HANZI_10_START_W   0x20    /* At 1xxxxxxx 0xxxxxxx hand */
#define BIG5_HANZI_10_END_Q     0x26    /* At 1xxxxxxx 0xxxxxxx hand */
#define BIG5_HANZI_10_END_W     0x5e    /* At 1xxxxxxx 0xxxxxxx hand */

#define BIG5_HANZI_11_START_Q2  0x29    /* At 1xxxxxxx 1xxxxxxx hand */
#define BIG5_HANZI_11_START_W2  0x01    /* At 1xxxxxxx 1xxxxxxx hand */
#define BIG5_HANZI_11_END_Q2    0x59    /* At 1xxxxxxx 1xxxxxxx hand */
#define BIG5_HANZI_11_END_W2    0x35    /* At 1xxxxxxx 1xxxxxxx hand */

#define BIG5_HANZI_10_START_Q2  0x29    /* At 1xxxxxxx 0xxxxxxx hand */
#define BIG5_HANZI_10_START_W2  0x20    /* At 1xxxxxxx 0xxxxxxx hand */
#define BIG5_HANZI_10_END_Q2    0x59    /* At 1xxxxxxx 0xxxxxxx hand */
#define BIG5_HANZI_10_END_W2    0x5e    /* At 1xxxxxxx 0xxxxxxx hand */

#define CNS_SYMBOL_11_START     0
#define CNS_SYMBOL_11_END       0
#define BIG5_SYMBOL_10_START    0
#define BIG5_SYMBOL_10_END      0
#define BIG5_SYMBOL_11_START    0
#define BIG5_SYMBOL_11_END      0

#define BIG5_TO_CNS     0x0000  /* Flag for convert from BIG5 to CNS */
#define CNS_TO_BIG5     0x0001  /* Flag for convert from CNS to BIG5 */

#define BIG5_1NUMS      ((BIG5_NUM_10_Q*0x23)+(BIG5_NUM_11_Q*0x22))
                                /* Numbers of level 1 */
#define BIG5_1TO2_SKIP  (0x25*(BIG5_NUM_10_Q+BIG5_NUM_11_Q))
                                /*Location of starting level 2 */

#define CNS_SPACE       0x256D  /* for unconvert character code */

/* not sequential character code on CNS level 2 */
#define CNS_XK          0xa121  /* XK on Tsang Jye */
#define CNS_ONLN        0xa14c  /* ONLN on Tsang Jye */
#define CNS_MSOK        0xa24d  /* MSOK on Tsang Jye */
#define CNS_TNKM        0xbf6a  /* TNKM on Tsang Jye */
#define CNS_CYIB        0xd54b  /* CYIB on Tsang Jye */
#define CNS_YIHXO       0xda28  /* YIHXO on Tsang Jye */
#define CNS_MDMR        0xdd74  /* MDMR on Tsang Jye */
#define CNS_COLH        0xe42f  /* COLH on Tsang Jye */
#define CNS_ODC         0xe761  /* ODC on Tsang Jye */
#define CNS_OKHAE       0xe934  /* OKHAE on Tsang Jye */
#define CNS_HBBM        0xe64d  /* HBBM on Tsang Jye */
#define CNS_CJTC        0xea4b  /* CJTC on Tsang Jye */
#define CNS_LNNXU       0xf166  /* LNNXU on Tsang Jye */
#define CNS_YPYBP       0xf244  /* YPYBP on Tsang Jye */
#define CNS_FDDH        0xf240  /* FDDH on Tsang Jye */
#define CNS_HOOMA       0xd722  /* HOOMA on Tsang Jye */

/* not sequential character code on BIG5 level 2 */
#define BIG5_XK         0xc940  /* XK on Tsang Jye */
#define BIG5_ONLN       0xc9be  /* ONLN on Tsang Jye */
#define BIG5_MSOK       0xcaf7  /* MSOK on Tsang Jye */
#define BIG5_TNKM       0xd77a  /* TNKM on Tsang Jye */
#define BIG5_CYIB       0xebf1  /* CYIB on Tsang Jye */
#define BIG5_YIHXO      0xf0cb  /* YIHXO on Tsang Jye */
#define BIG5_MDMR       0xf056  /* MDMR on Tsang Jye */
#define BIG5_COLH       0xeeeb  /* COLH on Tsang Jye */
#define BIG5_ODC        0xf16b  /* ODC on Tsang Jye */
#define BIG5_OKHAE      0xf268  /* OKHAE on Tsang Jye */
#define BIG5_HBBM       0xf4b5  /* HBBM on Tsang Jye */
#define BIG5_CJTC       0xf663  /* CJTC on Tsang Jye */
#define BIG5_LNNXU      0xf9c4  /* LNNXU on Tsang Jye */
#define BIG5_YPYBP      0xf9d5  /* YPYBP on Tsang Jye */
#define BIG5_FDDH       0xf9c6  /* FDDH on Tsang Jye */
#define BIG5_HOOMA      0xecde  /* HOOMA on Tsang Jye */
#define BIG5_MU         0xc94a  /* MU on Tsang Jye */
#define BIG5_GRHNE      0xddfc  /* GRHNE on Tsang Jye */

/* This function checks if the given code is really a Hanzi in the original 
code definition determined by "which".  If so, it returns 1. And otherwise
it returns 0)
*/
static int
_is_hanzi (code, which)
     w_char code;
     int which;
{
  register unsigned char high, low;

  if (which == CNS_TO_BIG5)
    {
      if ((code & 0x8080) == 0x8080)
        {                       /* 1xxxxxxx 1xxxxxxx Case */
          high = (code >> 8) - 0xa0;
          low = (code & 0xff) - 0xa0;
          if (((high >= CNS_HANZI_11_START_Q && high < CNS_HANZI_11_END_Q) &&
               (low >= CNS_HANZI_11_START_W && low <= CNS_NUM_Q)) ||
              ((high == CNS_HANZI_11_END_Q) &&
               (low >= CNS_HANZI_11_START_W && low <= CNS_HANZI_11_END_W)) || ((high >= CNS_HANZI_11_START_QM && high < CNS_HANZI_11_END_QM) && (low >= CNS_HANZI_11_START_W && low <= CNS_NUM_Q)))
            {
              return (1);
            }
          else
            {
              return (0);
            }
        }
      else if ((code & 0x8080) == 0x8000)
        {                       /* 1xxxxxxx 0xxxxxxx Case */
          high = (code >> 8) - 0xa0;
          low = (code & 0xff) - 0x20;
          if (((high >= CNS_HANZI_10_START_Q && high < CNS_HANZI_10_END_Q) &&
               (low >= CNS_HANZI_10_START_W && low <= CNS_NUM_Q)) || ((high == CNS_HANZI_10_END_Q) && (low >= CNS_HANZI_10_START_W && low <= CNS_HANZI_10_END_W)))
            {
              return (1);
            }
          else
            {
              return (0);
            }
        }
      else
        {
          return (0);
        }
    }
  else if (which == BIG5_TO_CNS)
    {
      if ((code & 0x8080) == 0x8080)
        {                       /* 1xxxxxxx 1xxxxxxx Case */
          high = (code >> 8) - 0xa0;
          low = (code & 0xff) - 0xa0;
          if (((high >= BIG5_HANZI_11_START_Q && high <= BIG5_HANZI_11_END_Q) &&
               (low >= BIG5_HANZI_11_START_W && low <= BIG5_HANZI_11_END_W)) ||
              ((high >= BIG5_HANZI_11_START_QM && high <= BIG5_HANZI_11_END_QM) && (low >= BIG5_HANZI_11_START_W && low <= BIG5_HANZI_11_END_W)))
            {
              return (1);
            }
          else if (((high >= BIG5_HANZI_11_START_Q2 && high < BIG5_HANZI_11_END_Q2) &&
                    (low >= BIG5_HANZI_11_START_W2 && low <= BIG5_HANZI_11_END_W)) || ((high == BIG5_HANZI_11_END_Q2) && (low >= BIG5_HANZI_11_START_W2 && low <= BIG5_HANZI_11_END_W2)))
            {
              return (1);
            }
          else
            {
              return (0);
            }
        }
      else if ((code & 0x8080) == 0x8000)
        {                       /* 1xxxxxxx 0xxxxxxx Case */
          high = (code >> 8) - 0xa0;
          low = (code & 0xff) - 0x20;
          if ((high >= BIG5_HANZI_10_START_Q && high <= BIG5_HANZI_10_END_Q) && (low >= BIG5_HANZI_10_START_W && low <= BIG5_HANZI_10_END_W))
            {
              return (1);
            }
          else if ((high >= BIG5_HANZI_10_START_Q2 && high <= BIG5_HANZI_10_END_Q2) && (low >= BIG5_HANZI_10_START_W2 && low <= BIG5_HANZI_10_END_W2))
            {
              return (1);
            }
          else
            {
              return (0);
            }
        }
      else
        {
          return (0);
        }
    }
  return (0);
}

/* convert one Hanzi from BIG5 (or CNS) to CNS (or BIG5) depend on the 
   parameter "which".  This function works under the asumpsion that the 
   give code is really a Hanzi under the original code definition.  Given 
   code value can be checked by function "_is_hanzi()" in the same file.
   The result Hanzi code is always returned.
*/
static unsigned int
_convert (code, which)
     register w_char code;
     int which;
{
  unsigned int qu, wei;         /* counting from   1 ------     */
  register unsigned int location;       /* counting from   0 ----       */
  unsigned int loc_wei;         /* counting from   0 ----       */
  int plant;

  if (which == CNS_TO_BIG5)
    {
      if ((code & 0x8080) == 0x8080)
        {                       /* 1xxxxxxx 1xxxxxxx Case */
          if (code <= 0xa5fe && code >= 0xa1a1)
            {
              wei = (code & 0x7f);
              switch (((code & 0xff00) >> 8) - 0xa0)
                {
                case (1):
                  if (wei > 0x5f)
                    {
                      return (0xa1a1 + (wei - 0x60));
                    }
                  else
                    {
                      return (0xa140 + (wei - 0x21));
                    }
                case (2):
                  if (wei > 0x5f)
                    {
                      return (0xa2a1 + (wei - 0x60));
                    }
                  else
                    {
                      return (0xa240 + (wei - 0x21));
                    }
                case (3):
                  if (wei > 0x40)
                    {
                      return (0xa2a1 + (wei - 0x41));
                    }
                  else
                    {
                      return (0xa25f + (wei - 0x21));
                    }
                case (4):
                  if (wei > 0x70)
                    {
                      return (0xa340 + (wei - 0x71));
                    }
                  else
                    {
                      return (0xa2af + (wei - 0x21));
                    }
                case (5):
                  if (wei > 0x51)
                    {
                      return (0xa3a1 + (wei - 0x52));
                    }
                  else
                    {
                      return (0xa34e + (wei - 0x21));
                    }
                }
            }
          location = ((code >> 8) - 0xa0 - CNS_HANZI_11_START_Q) * CNS_NUM_Q + (code & 0xff) - 0xa0 - 1;
          if ((code > 0xebd0 && code <= 0xefdb) || (code > 0xf5c5 && code <= 0xf7c6) || (code > 0xf8ad && code <= 0xf9e1))
            location--;
          else if (code == 0xebd0)
            return (0xbe52);
          else if (code == 0xf5b5)
            return (0xc2cb);
          else if (code == 0xf8ad)
            return (0xc456);
        }
      else
        {                       /* 1xxxxxxx 0xxxxxxx Case */
          location = ((code >> 8) - 0xa0 - CNS_HANZI_10_START_Q) * CNS_NUM_Q + (code & 0xff) - 0x20 - 1;
          location += BIG5_1TO2_SKIP;
          if (code > CNS_YIHXO && code <= 0xdb3e)
            location--;
          else if ((code >= 0xa12b && code < CNS_ONLN) ||
                   (code >= 0xa17d && code < CNS_MSOK) ||
                   (code >= 0xa439 && code <= 0xb87d) ||
                   (code > CNS_TNKM && code <= 0xc423) ||
                   (code > CNS_CYIB && code < CNS_HOOMA) ||
                   (code >= 0xdc6a && code < CNS_MDMR) ||
                   (code >= 0xe039 && code <= 0xe242) || (code > CNS_OKHAE && code <= 0xe961) || (code > CNS_CJTC && code <= 0xec51) || (code > CNS_LNNXU && code <= 0xf233))
            location++;
          else if ((code >= 0xb87e && code < CNS_TNKM) ||
                   (code >= 0xc424 && code < CNS_CYIB) ||
                   (code >= 0xe243 && code <= 0xe336) ||
                   (code > CNS_COLH && code <= 0xe437) ||
                   (code > CNS_ODC && code < CNS_OKHAE) || (code >= 0xe962 && code < CNS_CJTC) || (code >= 0xec52 && code < CNS_LNNXU) || (code == 0xf234) || (code > CNS_FDDH && code <= CNS_YPYBP))
            location += 2;
          else if ((code >= 0xe337 && code < CNS_COLH) || (code >= 0xe438 && code <= 0xe572) || (code > CNS_HBBM && code < CNS_ODC) || (code >= 0xf235 && code < CNS_FDDH))
            location += 3;
          else if (code >= 0xe573 && code < CNS_HBBM)
            location += 4;
          else if (code == CNS_ONLN)
            return (BIG5_ONLN);
          else if (code == CNS_MSOK)
            return (BIG5_MSOK);
          else if (code == CNS_TNKM)
            return (BIG5_TNKM);
          else if (code == CNS_CYIB)
            return (BIG5_CYIB);
          else if (code == CNS_YIHXO)
            return (BIG5_YIHXO);
          else if (code == CNS_MDMR)
            return (BIG5_MDMR);
          else if (code == CNS_COLH)
            return (BIG5_COLH);
          else if (code == CNS_ODC)
            return (BIG5_ODC);
          else if (code == CNS_OKHAE)
            return (BIG5_OKHAE);
          else if (code == CNS_HBBM)
            return (BIG5_HBBM);
          else if (code == CNS_CJTC)
            return (BIG5_CJTC);
          else if (code == CNS_LNNXU)
            return (BIG5_LNNXU);
          else if (code == CNS_YPYBP)
            return (BIG5_YPYBP);
          else if (code == CNS_HOOMA)
            return (BIG5_HOOMA);
          else if (code == CNS_FDDH)
            return (BIG5_FDDH);
        }
      qu = location / (BIG5_NUM_10_Q + BIG5_NUM_11_Q) + BIG5_HANZI_11_START_Q;
      loc_wei = location % (BIG5_NUM_10_Q + BIG5_NUM_11_Q);

      if (loc_wei < BIG5_NUM_10_Q)
        {                       /* 1xxxxxxx 0xxxxxxx Case */
          plant = 0xa020;
          wei = loc_wei + BIG5_HANZI_10_START_W;
        }
      else
        {
          plant = 0xa0a0;
          wei = loc_wei - BIG5_NUM_10_Q + 1;    /* 1xxxxxxx 1xxxxxxx Case */
        }
      return ((qu << 8) + wei + plant);
    }
  else if (which == BIG5_TO_CNS)
    {
      if (code >= 0xa3cd && code <= 0xa140)
        {
          if (code <= 0xa1bf)
            {
              if (code & 0x80)
                {
                  return (0xa1e0 + (code - 0xa1a1));
                }
              else
                {
                  return (0xa1a1 + (code - 0xa140));
                }
            }
          else if (code <= 0xa25e)
            {
              if (code & 0x80)
                {
                  return (0xa2a1 + (code - 0xa1c0));
                }
              else
                {
                  return (0xa2e0 + (code - 0xa240));
                }
            }
          else if (code <= 0xa2ae)
            {
              if (code & 0x80)
                {
                  return (0xa3c1 + (code - 0xa2a1));
                }
              else
                {
                  return (0xa3a1 + (code - 0xa25f));
                }
            }
          else if (code <= 0xa34e)
            {
              if (code & 0x80)
                {
                  return (0xa4a1 + (code - 0xa2af));
                }
              else
                {
                  return (0xa4f1 + (code - 0xa340));
                }
            }
          else
            {
              if (code & 0x80)
                {
                  return (0xa5d2 + (code - 0xa3a1));
                }
              else
                {
                  return (0xa5a1 + (code - 0xa34e));
                }
            }
        }
      if ((code & 0x8080) == 0x8080)
        {                       /* 1xxxxxxx 1xxxxxxx Case */
          location = (((code >> 8) - 0xa0 - BIG5_HANZI_11_START_Q) * (BIG5_NUM_10_Q + BIG5_NUM_11_Q) + (code & 0xff) - 0xa0 + BIG5_NUM_10_Q - 1);

        }
      else
        {                       /* 1xxxxxxx 0xxxxxxx Case */
          location = (((code >> 8) - 0xa0 - BIG5_HANZI_10_START_Q) * (BIG5_NUM_10_Q + BIG5_NUM_11_Q) + (code & 0xff) - 0x20 - BIG5_HANZI_10_START_W);
        }
      if (location < BIG5_1NUMS)
        {                       /* 1xxxxxxx 0xxxxxxx  Case */
          plant = 0xa0a0;
          qu = location / BIG5_NUM_11_Q + CNS_HANZI_11_START_Q;
          wei = location % BIG5_NUM_11_Q + CNS_HANZI_11_START_W;
          if ((code >= 0xbbc8 && code < 0xbe52) || (code >= 0xc1ab && code < 0xc2cb) || (code >= 0xc361 && code < 0xc456))
            location++;
          else if (code == 0xbe52)
            return (0xebd0);
          else if (code == 0xc2cb)
            return (0xf5b5);
          else if (code == 0xc456)
            return (0xf8ad);
        }
      else
        {                       /* 1xxxxxxx 1xxxxxxx Case */
          location -= BIG5_1TO2_SKIP;   /* For level two */
          if (code >= 0xeb5b && code < BIG5_CYIB)
            location++;
          else if ((code > BIG5_MU && code <= 0xc96b) ||
                   (code > BIG5_ONLN && code <= 0xc9ec) ||
                   (code > BIG5_MSOK && code < BIG5_TNKM) ||
                   (code >= 0xdba7 && code < BIG5_GRHNE) ||
                   (code >= 0xe8a3 && code <= 0xe975) ||
                   (code > BIG5_HOOMA && code <= 0xeda9) ||
                   (code > BIG5_COLH && code < BIG5_MDMR) || (code >= 0xf466 && code < BIG5_HBBM) || (code >= 0xf4fd && code < BIG5_CJTC) || (code >= 0xf977 && code < BIG5_LNNXU))
            location--;
          else if ((code > BIG5_TNKM && code <= 0xdba6) ||
                   (code > BIG5_GRHNE && code <= 0xe8a2) ||
                   (code > BIG5_MDMR && code < BIG5_YIHXO) ||
                   (code >= 0xf163 && code < BIG5_ODC) ||
                   (code >= 0xf375 && code <= 0xf465) || (code > BIG5_HBBM && code <= 0xf4fc) || (code > BIG5_CJTC && code <= 0xf976) || (code == 0xf9c5) || (code >= 0xf9d2 && code <= BIG5_YPYBP))
            location -= 2;
          else if ((code > BIG5_YIHXO && code <= 0xf162) || (code > BIG5_ODC && code < BIG5_OKHAE) || (code >= 0xf2c3 && code <= 0xf374) || (code > BIG5_FDDH && code <= 0xf9d1))
            location -= 3;
          else if (code > BIG5_OKHAE && code <= 0xf2c2)
            location -= 4;
          else if (code == BIG5_ONLN)
            return (CNS_ONLN);
          else if (code == BIG5_MSOK)
            return (CNS_MSOK);
          else if (code == BIG5_TNKM)
            return (CNS_TNKM);
          else if (code == BIG5_CYIB)
            return (CNS_CYIB);
          else if (code == BIG5_YIHXO)
            return (CNS_YIHXO);
          else if (code == BIG5_MDMR)
            return (CNS_MDMR);
          else if (code == BIG5_COLH)
            return (CNS_COLH);
          else if (code == BIG5_ODC)
            return (CNS_ODC);
          else if (code == BIG5_OKHAE)
            return (CNS_OKHAE);
          else if (code == BIG5_HBBM)
            return (CNS_HBBM);
          else if (code == BIG5_CJTC)
            return (CNS_CJTC);
          else if (code == BIG5_LNNXU)
            return (CNS_LNNXU);
          else if (code == BIG5_FDDH)
            return (CNS_FDDH);
          else if (code == BIG5_YPYBP)
            return (CNS_YPYBP);
          else if (code == BIG5_HOOMA)
            return (CNS_HOOMA);
          else if (code == BIG5_MU)
            return (CNS_SPACE);
          else if (code == BIG5_GRHNE)
            return (CNS_SPACE);

          plant = 0xa020;
          qu = location / BIG5_NUM_11_Q + CNS_HANZI_10_START_Q;
          wei = location % BIG5_NUM_11_Q + CNS_HANZI_10_START_W;
        }
      return ((qu << 8) + wei + plant);
    }
  return (0);
}

#ifdef  ECNS_IS_UCNS
int
ecns_to_icns (icns, ucns, siz)
     w_char *icns;
     unsigned char *ucns;
     int siz;
{
  register w_char *i = icns;
  register unsigned char *u = ucns, *uend = ucns + siz, x;
  static w_char local_pending = (w_char) 0;
  static unsigned char shift_mode = '\0';

  if (siz <= 0)
    return (0);
  if (local_pending)
    {
      *i = local_pending | (*u++ & 0x7f);
      if (shift_mode == SS3)
        {
          *i++ |= 0x8000;
          shift_mode = '\0';
        }
      else
        {
          *i++ |= 0x8080;
        }
      local_pending = (w_char) 0;
    }
  if (shift_mode == SS2)
    {
      *i++ = *u++;
      shift_mode = '\0';
    }
  for (; u < uend;)
    {
      x = *u++;
      if (x == SS2)
        {
          if (u == uend)
            {
              shift_mode = SS2;
              break;
            }
          *i++ = *u++;
          break;
        }
      else if (x == SS3)
        {
          if (u == uend)
            {
              shift_mode = SS3;
              break;
            }
          *i = ((*u++ & 0x7f) << 8);
          if (u == uend)
            {
              local_pending = *i;
              break;
            }
          *i |= (*u++ & 0x7f);
          *i++ |= 0x8000;
        }
      else if (x > 0x7f)
        {
          *i = ((x & 0x7f) << 8);
          if (u == uend)
            {
              local_pending = *i;
              break;
            }
          *i |= (*u++ & 0x7f);
          *i++ |= 0x8080;
        }
      else
        {
          *i++ = x;
        }
    }
  return ((char *) i - (char *) icns);
}

int
icns_to_ecns (ucns, icns, siz)
     unsigned char *ucns;
     w_char *icns;
     int siz;
{
  register unsigned char *u = ucns;
  register w_char *i = icns, w;

  for (; siz > 0; siz -= sizeof (w_char))
    {
      w = *i++;
      if (!(w & 0xff80))
        {                       /* CS0 */
          *u++ = (unsigned char) w;
        }
      else if (!(w & 0xff00))
        {                       /* CS2 */
          *u++ = SS2;
          *u++ = (unsigned char) w;
        }
      else if (w & 0x80)
        {                       /* CS1 */
          *u++ = (unsigned char) ((w & 0xff00) >> 8);
          *u++ = (unsigned char) (w & 0xff);
        }
      else
        {                       /* CS3 */
          *u++ = SS3;
          *u++ = (unsigned char) ((w & 0xff00) >> 8);
          *u++ = (unsigned char) ((w & 0xff) | 0x80);
        }
    }
  return (u - ucns);
}
#else /* ECNS_IS_UCNS */

static int oc_mode = ASCII;
static unsigned char *cns;

static void
putcns (x)
     unsigned char x;
{
  *cns++ = x;
}

static void
cns_change_mode (mode, new_mode)
     int *mode;
     int new_mode;
{
  if (*mode == new_mode)
    return;
  switch (*mode = new_mode)
    {
    case CNS11643_1:
      putcns ('\033');
      putcns ('$');
      putcns (')');
      putcns ('0');
      break;
    case CNS11643_2:
      putcns ('\033');
      putcns ('$');
      putcns ('*');
      putcns ('1');
      break;
    case ASCII:
      /* designate ISO-8859-1 rather than JIS X 0201 */
      /* putcns('\033'); putcns('('); putcns('J'); break; */
      putcns ('\033');
      putcns ('(');
      putcns ('B');
      break;
    }
}

int
ecns_to_icns (icns, ecns, siz)
     w_char *icns;
     unsigned char *ecns;
     int siz;
{
  designate = CNS_designate;
  return (extc_to_intc (icns, ecns, siz));
}

int
icns_to_ecns (ecns, icns, siz)
     unsigned char *ecns;
     w_char *icns;
     int siz;
{
  register int i = siz;
  register w_char *ic, x;
  cns = ecns;

  for (; i > 0; i -= sizeof (w_char))
    {
      x = *ic++;
      if ((x & 0x8080) == 0x8080)
        {
          cns_change_mode (&oc_mode, CNS11643_1);
          putcns ((x >> 8) & 0xff);
          putcns (x & 0xff);
        }
      else if ((x & 0x8000) == 0x8000)
        {
          cns_change_mode (&oc_mode, CNS11643_2);
          putcns (((x >> 8) & 0x7f) | 0x80);
          putcns ((x & 0x7f) | 0x80);
        }
      else
        {
          cns_change_mode (&oc_mode, ASCII);
          putcns (x & 0x7f);
        }
    }
  cns_change_mode (&oc_mode, ASCII);
  return (cns - ecns);
}
#endif /* ECNS_IS_UCNS */

int
icns_to_big5 (big5, icns, siz)
     unsigned char *big5;
     w_char *icns;
     int siz;
{
  register unsigned char *d = big5;
  register w_char *s = icns;
  register int i = siz;
  short code_out;               /* Buffering one two-byte code  */

  if (d == NULL || s == NULL)
    return (-1);

  for (; i > 0; i -= sizeof (w_char))
    {
      if (!(*s & 0xff00))
        {                       /* Ascii */
          if (!(*s & 0x80))
            {
              *d++ = (unsigned char) (*s++ & 0x7f);
            }
          else
            {                   /* Single Shift */
              *d++ = SS2;
              *d++ = (unsigned char) (*s++ & 0xff);
            }
        }
      else if (_is_hanzi (*s, CNS_TO_BIG5))
        {
          code_out = _convert (*s++, CNS_TO_BIG5);
          *d++ = (code_out >> 8);
          *d++ = code_out & 0x00ff;
        }
      else
        {                       /* Strainge codes */
          *d++ = (unsigned char) ((*s & 0xff00) >> 8);
          *d++ = (unsigned char) (*s++ & 0xff);
        }
    }
  *d = '\0';
  return (d - big5);
}

int
ecns_to_big5 (big5, ecns, siz)
     unsigned char *big5, *ecns;
     int siz;
{
  int len;
  len = ecns_to_icns (tmp_w_buf, ecns, siz);
  return (icns_to_big5 (big5, tmp_w_buf, len));
}

int
big5_to_icns (icns, big5, siz)
     w_char *icns;
     unsigned char *big5;
     int siz;
{
  register w_char *d = icns;
  register unsigned char *s = big5;
  unsigned char *send = s + siz;
  unsigned short code_in;       /* Buffering one two-byte code  */

  if (d == NULL || s == NULL)
    return (-1);

  for (; s < send; s++)
    {
      if (!(*s & 0x80))
        {                       /* Ascii */
          *d++ = (w_char) * s;
        }
      else if (*s == 0x8e)
        {                       /* Single Shift */
          *d++ = (w_char) * (++s);
        }
      else
        {
          code_in = ((*s++) << 8);
          code_in |= *s;
          if (_is_hanzi (code_in, BIG5_TO_CNS))
            {
              *d++ = _convert (code_in, BIG5_TO_CNS);
            }
          else
            {                   /* Strainge codes */
              *d++ = code_in;
            }
        }
    }
  *d = (w_char) 0;
  return ((char *) d - (char *) icns);
}

int
big5_to_ecns (ecns, big5, siz)
     unsigned char *ecns, *big5;
     int siz;
{
  int len;
  len = big5_to_icns (tmp_w_buf, big5, siz);
  return (icns_to_ecns (ecns, tmp_w_buf, len));
}

int
iugb_to_eugb (eugb, iugb, siz)
     unsigned char *eugb;
     w_char *iugb;
     int siz;
{
  static int first = 0;
  static unsigned int cswidth_id;

  if (first == 0)
    {
      cswidth_id = create_cswidth (UGB_CSWIDTH);
      first++;
    }
  set_cswidth (cswidth_id);
  return (ieuc_to_eeuc (eugb, iugb, siz));
}

int
eugb_to_iugb (iugb, eugb, siz)
     w_char *iugb;
     unsigned char *eugb;
     int siz;
{
  static int first = 0;
  static unsigned int cswidth_id;

  if (first == 0)
    {
      cswidth_id = create_cswidth (UGB_CSWIDTH);
      first++;
    }
  set_cswidth (cswidth_id);
  return (eeuc_to_ieuc (iugb, eugb, siz));
}
#endif /* CHINESE */


#ifdef  KOREAN

#define ZENKAKU_KSC     1

static unsigned char *ks;
static w_char *iuk;
static unsigned char *euk;

static void
putks (x)
     int x;
{
  *ks++ = x;
}

static int oks_mode = ASCII;    /* 出力時のKSCコードのモード */
extern int euksc_to_iuksc ();

static void
ksc_change_mode (mode, new_mode)
     int *mode;
     int new_mode;
{
  if (*mode == new_mode)
    return;
  switch (*mode)
    {
    case ZENKAKU_KSC:
      putks ('\033');
      putks ('(');
      putks ('B');
      break;
    default:;
    }
  *mode = new_mode;
  switch (new_mode)
    {
    case ZENKAKU_KSC:
      putks ('\033');
      putks ('$');
      putks ('(');
      putks ('C');
      break;
    default:;
    }
}

/*      内部 U-ksc を ksc コードに変換します
        文字列の長さを返します                  */
extern int
iuksc_to_ksc (ksc, iuksc, iusiz)
     unsigned char *ksc;        /*      kscコードになったものをおくbuf  */
     w_char *iuksc;             /*      iukscコードのものをおいてくるbuf */
     int iusiz;                 /*      iuksc の大きさ                  */
{
  int x;
  ks = ksc;
  iuk = iuksc;
  for (; iusiz > 0; iusiz -= sizeof (w_char))
    {
      x = *iuk++;
      if ((x & 0x8080) == 0x8080)
        {
          ksc_change_mode (&oks_mode, ZENKAKU_KSC);
          putks ((x >> 8) & 0x7f);
          putks (x & 0x7f);
        }
      else
        {
          ksc_change_mode (&oks_mode, ASCII);
          putks (x);
        }
    }
  ksc_change_mode (&oks_mode, ASCII);
  return (ks - ksc);
}


/*      外部 U-ksc を ksc コードに変換します    */
extern int
euksc_to_ksc (ksc, euksc, eusiz)
     unsigned char *ksc, *euksc;
     int eusiz;
{
  static int kanji1 = 0;
  static unsigned char kanji1_code = 0;
  /* 0: normal
     1: get SS2
     2: get kanji 1 byte */
  int x;
  ks = ksc;
  euk = euksc;
  if (kanji1 != 0)
    {
      if (kanji1 == 2)
        {
          putks (kanji1_code & 0x7f);
          putks (*euk & 0x7f);
        }
      else
        {
          putks (*euk);
        }
      eusiz -= sizeof (char);
      kanji1 = 0;
      euk++;
    }
  for (; eusiz > 0; eusiz -= sizeof (char))
    {
      x = *euk++;
      if (x & 0x80)
        {
          ksc_change_mode (&oks_mode, ZENKAKU_KSC);
          if (eusiz > 1)
            {
              putks (x & 0x7f);
              putks (*euk++ & 0x7f);
              eusiz -= sizeof (char);
            }
          else
            {
              kanji1 = 2;
              kanji1_code = x;
            }
        }
      else
        {
          ksc_change_mode (&oks_mode, ASCII);
          putks (x);
        }
    }
  if (kanji1 == 0)
    ksc_change_mode (&oks_mode, ASCII);
  return (ks - ksc);
}

/*      内部 U-ksc を 外部 U-ksc コードに変換します     */
extern int
iuksc_to_euksc (euksc, iuksc, iusiz)
     unsigned char *euksc;
     w_char *iuksc;
     int iusiz;
{
  static int first = 0;
  static unsigned int cswidth_id;

  if (first == 0)
    {
      cswidth_id = create_cswidth (UKSC_CSWIDTH);
      first++;
    }
  set_cswidth (cswidth_id);
  return (ieuc_to_eeuc (euksc, iuksc, iusiz));
}

int
ksc_to_euksc (euksc, ksc, jsiz)
     unsigned char *euksc, *ksc;
     int jsiz;
{
  int len;

  designate = KSC_designate;
  len = extc_to_intc (tmp_w_buf, ksc, jsiz);
  return (iuksc_to_euksc (euksc, tmp_w_buf, len));
}

int
ksc_to_iuksc (iuksc, ksc, jsiz)
     w_char *iuksc;
     unsigned char *ksc;
     int jsiz;
{
  designate = KSC_designate;
  return (extc_to_intc (iuksc, ksc, jsiz));
}

int
euksc_to_iuksc (iuksc, euksc, eusiz)
     w_char *iuksc;
     unsigned char *euksc;
     int eusiz;
{
  static int first = 0;
  static unsigned int cswidth_id;

  if (first == 0)
    {
      cswidth_id = create_cswidth (UKSC_CSWIDTH);
      first++;
    }
  set_cswidth (cswidth_id);
  return (eeuc_to_ieuc (iuksc, euksc, eusiz));
}

#endif /* KOREAN */
