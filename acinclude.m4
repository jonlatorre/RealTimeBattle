dnl RTB_EXPAND_DIR(VARNAME, DIR)
dnl expands occurrences of ${prefix} and ${exec_prefix} in the given DIR,
dnl and assigns the resulting string to VARNAME
dnl example: RTB_EXPAND_DIR(ROBOTDIR, "${RTB_DIR}/Robots")
dnl eg, then: AC_DEFINE_UNQUOTED(ROBOTDIR, "$ROBOTDIR")
dnl by Alexandre Oliva 
dnl from http://www.cygnus.com/ml/automake/1998-Aug/0040.html
AC_DEFUN(RTB_EXPAND_DIR, [
        $1=$2
        $1=`(
            test "x$prefix" = xNONE && prefix="$ac_default_prefix"
            test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
            eval echo \""[$]$1"\"
        )`
])
