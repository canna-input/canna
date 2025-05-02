#include "RKintern.h"
#include <stdio.h>
#include "ccompat.h"

#if !defined( HYOUJUN_GRAM )
#ifdef USE_OBSOLETE_STYLE_FILENAME
#define HYOUJUN_GRAM "/usr/lib/canna/dic/canna/fuzokugo.d"
#else
#define HYOUJUN_GRAM "/usr/lib/canna/dic/canna/fuzokugo.cbd"
#endif
#endif

char	*program;

static char *
basename(name)
     char	*name;
{
  char	*s = name + strlen(name);
  
  while (s-- > name)
    if (*s == '/')
      return(++s);
  return(name);
}

static void
usage()
{
  fprintf(stderr, "usage: %s [option] hinshi...\n", program);
  fprintf(stderr, "\t-d grammar-dic\n");
  exit(1);
}

main(ac, av)
    int ac;
    char *av[];
{
  char *fzk = NULL;
  struct RkKxGram *gram;
  int i;

  program = basename(av[0]);
  if (!(++av, --ac))
    usage();
  if (!strcmp(av[0], "-d")) {
    if (!(++av, --ac))
      usage();
    fzk = av[0];
    if (!(++av, --ac))
      usage();
  }
  if (!fzk)
    fzk = HYOUJUN_GRAM;
  if (!(gram = RkOpenGram(fzk))) {
    fprintf(stderr, "Warning: cannot open grammar file %s.\n", fzk);
    exit(1);
  }

  for (i = 0; i < ac; i++) {
    int j, row;
    char *cj;

    if (av[i][0] == '#')
      av[i]++;
    row = RkGetGramNum(gram, av[i]);
    if (row < 0) {
      fprintf(stderr, "%s: unknown hinshi '%s'.\n", program, av[i]);
      return 1;
    }

    fprintf(stdout, "before %s:\n", av[i]); 
    for (j = 0; j < gram->ng_rowcol; j++) { 
	if (TestGram(GetGramRow(gram, j), row))
	    fprintf(stdout, " %s", gram->ng_strtab[j]); 
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "after %s:\n", av[i]); 
    cj = GetGramRow(gram, row);
    for (j = 0; j < gram->ng_rowcol; j++) { 
	if (TestGram(cj, j))
	    fprintf(stdout, " %s", gram->ng_strtab[j]); 
    }
    fprintf(stdout, "\n");
  }

  return 0;
}
