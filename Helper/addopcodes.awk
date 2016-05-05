#!/usr/bin/awk
#
# This script appends additional token codes to the end of the
# parse.h file that lemon generates.  These extra token codes are
# not used by the parser.  But they are used by the tokenizer and/or
# the code generator.
#
#
BEGIN {
  max = 0
  printf "#ifndef _PARSER_H \n"
  printf "#define _PARSER_H \n"
}
/^#define TK_/ {
  print $0
  if( max<$3 ) max = $3
  Tarray[$3] = $2
}
END {
  len = max
  printf "#define TK_%-29s %4d\n", "TO_TEXT",         ++max
  Tarray[max] = "TK_TO_TEXT"
  printf "#define TK_%-29s %4d\n", "TO_BLOB",         ++max
  Tarray[max] = "TK_TO_BLOB"
  printf "#define TK_%-29s %4d\n", "TO_NUMERIC",      ++max
  Tarray[max] = "TK_TO_NUMERIC"
  printf "#define TK_%-29s %4d\n", "TO_INT",          ++max
  Tarray[max] = "TK_TO_INT"
  printf "#define TK_%-29s %4d\n", "TO_REAL",         ++max
  Tarray[max] = "TK_TO_REAL"
  printf "#define TK_%-29s %4d\n", "ISNOT",           ++max
  Tarray[max] = "TK_ISNOT"
  printf "#define TK_%-29s %4d\n", "END_OF_FILE",     ++max
  Tarray[max] = "TK_END_OF_FILE"
  printf "#define TK_%-29s %4d\n", "ILLEGAL",         ++max
  Tarray[max] = "TK_ILLEGAL"
  printf "#define TK_%-29s %4d\n", "SPACE",           ++max
  Tarray[max] = "TK_SPACE"
  printf "#define TK_%-29s %4d\n", "UNCLOSED_STRING", ++max
  Tarray[max] = "TK_UNCLOSED_STRING"
  printf "#define TK_%-29s %4d\n", "FUNCTION",        ++max
  Tarray[max] = "TK_FUNCTION"
  printf "#define TK_%-29s %4d\n", "AGG_FUNCTION",    ++max
  Tarray[max] = "TK_AGG_FUNCTION"
  printf "#define TK_%-29s %4d\n", "AGG_COLUMN",      ++max
  Tarray[max] = "TK_AGG_COLUMN"
  printf "#define TK_%-29s %4d\n", "CONST_FUNC",      ++max
  Tarray[max] = "TK_CONST_FUNC"
  printf "#define TK_%-29s %4d\n", "UMINUS",          ++max
  Tarray[max] = "TK_UMINUS"
  printf "#define TK_%-29s %4d\n", "UPLUS",           ++max
  Tarray[max] = "TK_UPLUS"

  printf "#define TK_%-29s %4d\n", "ID_TABLENAME",           ++max
  Tarray[max] = "TK_ID_TABLENAME"
	printf "#define TK_%-29s %4d\n", "ID_SEQUENCE",           ++max
	Tarray[max] = "TK_ID_SEQUENCE"
	printf "#define TK_%-29s %4d\n", "HINT",           ++max
	Tarray[max] = "TK_HINT"

  printf "#ifndef _TOKEN_NAME_ \n"
  printf "#define _TOKEN_NAME_ \n"
  printf "const char * const TokenName[] = { \"\", "
  for(k=1;k<=max;k++)
  {printf "\"%s\",",Tarray[k];}

  printf "};\n"
 printf "\n#endif"
 printf "\n#endif"
 printf "\n"
}


