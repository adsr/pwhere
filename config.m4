PHP_ARG_ENABLE([pxtrace],
  [whether to enable pxtrace support],
  [AS_HELP_STRING([--enable-pxtrace],
    [Enable pxtrace support])],
  [no])

if test "$PHP_PXTRACE" != "no"; then
  AC_DEFINE(HAVE_PXTRACE, 1, [ Have pxtrace support ])
  PHP_NEW_EXTENSION(pxtrace, pxtrace.c, $ext_shared)
fi
