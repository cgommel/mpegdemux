dnl **************************************************************************
dnl * HH_DEFINE_VERSION(desc, name, maj, min, mic)
dnl **************************************************************************
dnl
AC_DEFUN(HH_DEFINE_VERSION,
[
AC_MSG_CHECKING([$1 version])

$2_MAJ=ifelse($3, , 0, $3)
$2_MIN=ifelse($4, , 0, $4)
$2_MIC=ifelse($5, , 0, $5)
$2_STR="${$2_MAJ}.${$2_MIN}.${$2_MIC}"

AC_MSG_RESULT(${$2_STR})

AC_SUBST($2_MAJ)
AC_SUBST($2_MIN)
AC_SUBST($2_MIC)
AC_SUBST($2_STR)

AC_DEFINE_UNQUOTED($2_MAJ, ${$2_MAJ})
AC_DEFINE_UNQUOTED($2_MIN, ${$2_MIN})
AC_DEFINE_UNQUOTED($2_MIC, ${$2_MIC})
AC_DEFINE_UNQUOTED($2_STR, "${$2_STR}")
])


