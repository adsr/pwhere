PHP_ARG_ENABLE([pwhere],
  [whether to enable pwhere support],
  [AS_HELP_STRING([--enable-pwhere],
    [Enable pwhere support])],
  [no])

if test "$PHP_PWHERE" != "no"; then
  AC_DEFINE(HAVE_PWHERE, 1, [ Have pwhere support ])
  PHP_NEW_EXTENSION(pwhere, pwhere.c, $ext_shared, , , , true)
fi
