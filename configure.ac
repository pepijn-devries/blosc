AC_INIT([blosc], 0.1.0)

AC_LANG(C++)
AC_REQUIRE_CPP
AC_PROG_CXX

# default values
AC_SUBST([BLOSC_INCLUDE], "-I/usr/local/include")
AC_SUBST([BLOSC_LIBS], "/usr/local/lib/blosc.a")

# allow for override
AC_ARG_WITH([blosc],
            AC_HELP_STRING([--with-blosc=PREFIX],
                           [path to where blosc is installed]),
            [
             BLOSC_INCLUDE="-I${with_blosc}/include"
             BLOSC_LIBS="${with_blosc}/lib/blosc.a"
            ],
            [])

# create and report output
AC_CONFIG_FILES([src/Makevars])
AC_OUTPUT
echo
echo "Final src/Makevars"
cat src/Makevars