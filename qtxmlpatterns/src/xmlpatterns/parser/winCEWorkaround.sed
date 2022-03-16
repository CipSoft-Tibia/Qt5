/\/\* Tokens\.  \*\// i\
\/\* These tokens are defined to nothing on Windows because they\'re\
 \* used in their documentation parser, for use in things like:\
 \*\
 \* int foo(IN char\* name, OUT char\* path);\
 \*\
 \* Hence this un-break fix. Note that this file was auto generated. *\/\
\#ifdef IN\
\#  undef IN\
\#endif\
\#ifdef INSTANCE\
\#  undef INSTANCE\
\#endif\
\#ifdef STRICT\
\#  undef STRICT\
\#endif\
\#ifdef SELF\
\#  undef SELF\
\#endif\
\/\* These tokens are defined in VxWorks kernel mode\
 \*\
 \* Hence this un-break fix. Note that this file was auto generated. *\/\
\#ifdef ERROR\
\#  undef ERROR\
\#endif\
\#ifdef IMPORT\
\#  undef IMPORT\
\#endif\
\#ifdef MAP\
\#  undef MAP\
\#endif\

