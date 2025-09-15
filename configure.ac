AC_INIT([blosc], [see.DESCRIPTION.file], [https://github.com/pepijn-devries/blosc/issues])
AC_CONFIG_SRCDIR([src/compress.cpp])
AC_CONFIG_AUX_DIR([tools])
AC_CANONICAL_HOST

m4_include([tools/pkg.m4])

# Find the compiler and compiler flags used by R.
: ${R_HOME=`R RHOME`}
if test -z "${R_HOME}"; then
  echo "could not determine R_HOME"
  exit 1
fi

# Find the compiler and compiler flags to use
RBIN="${R_HOME}/bin/R"
CXX=`"${RBIN}" CMD config CXX`
CXXFLAGS=`"${RBIN}" CMD config CXXFLAGS`
CPPFLAGS=`"${RBIN}" CMD config CPPFLAGS`
AC_LANG(C++)
AC_PROG_CPP

m4_pattern_allow([PKG_CONFIG_NAME])
PKG_CONFIG_NAME="blosc"
PKG_LIBS="-lblosc"
PKG_CPPFLAGS=""

PKG_PROG_PKG_CONFIG

# Check if building against a custom installation of blosc or system blosc
if test "x${INCLUDE_DIR}${LIB_DIR}" = x; then
  if test [ -n "$PKG_CONFIG" ] ; then
    # Check if blosc is installed
    PKGCONFIG_CFLAGS=`"${PKG_CONFIG}" --cflags --silence-errors "${PKG_CONFIG_NAME}"`
    case "${host_os}" in
      darwin*)
        PKGCONFIG_LIBS=`"${PKG_CONFIG}" --libs --static "${PKG_CONFIG_NAME}"`
      ;;
      *)
        PKGCONFIG_LIBS=`"${PKG_CONFIG}" --libs "${PKG_CONFIG_NAME}"`
      ;;
    esac
  fi
  if test "x${PKGCONFIG_CFLAGS}${PKGCONFIG_LIBS}" = x; then
    case "${host_os}" in
      darwin*)
      cpre=''
      for pre in /usr /usr/X11 /usr/X11R6 /usr/local /opt /opt/R/$(uname -m) /sw; do
        if test -e "${pre}/include/blosc.h"; then
          cpre=${pre}; break
        fi
      done
      if test -n "${cpre}"; then
        PKG_CPPFLAGS="-I${cpre}/include"
        if test "${cpre}" = /usr; then
          PKG_LIBS="-lblosc -llz4 -lzstd -lz"
        else
          PKG_LIBS="-L${cpre}/lib -lblosc -llz4 -lzstd -lz"
        fi
      fi
    ;;
    esac
  else
    echo "Found pkg-config cflags and libs!"
    PKG_CPPFLAGS="${PKGCONFIG_CFLAGS}"
    PKG_LIBS="${PKGCONFIG_LIBS}"
  fi
else
  echo "Found INCLUDE_DIR and/or LIB_DIR!"
  PKG_CPPFLAGS="-I${INCLUDE_DIR} ${PKG_CPPFLAGS}"
  PKG_LIBS="-L${LIB_DIR} ${PKG_LIBS}"
fi

if test "x${PKG_CPPFLAGS}${PKG_LIBS}" = x; then
    echo "
  -----------------------------------------------------------------------

   Configuration failed to find 'blosc.h'.

   Try installing:
     * deb: libblosc-dev (Debian, Ubuntu, etc)
     * rpm: blosc-devel (Fedora, CentOS, RHEL)
     * MacOS: call from R:
         'source(\"https://mac.R-project.org/bin/install.R\");install.libs(\"blosc\")'
   and try again.

   If the blosc library is installed on your system but the R package
   configuration is unable to find it, you can specify the include and
   lib path to blosc with:

   given you downloaded a tar-gz archive:
   
     R CMD INSTALL blosc-.tar.gz --configure-vars='INCLUDE_DIR=/path/to/include LIB_DIR=/path/to/lib'

   or cloned the GitHub blosc repository into a directory:
     R CMD INSTALL blosc/ --configure-vars='INCLUDE_DIR=/path/to/include LIB_DIR=/path/to/lib'

   or download and install blosc in R using
     install.packages('blosc', type='source', configure.vars='LIB_DIR=-L/path/to/libs INCLUDE_DIR=-I/path/to/headers')

  -----------------------------------------------------------------------

"

    AC_MSG_FAILURE([package dependency requirements 'blosc' could not be satisfied.])
fi

echo "
  --------------------------------------------------
  Configuration for ${PACKAGE_NAME}

    cppflags: ${PKG_CPPFLAGS}
    libs:     ${PKG_LIBS}

  --------------------------------------------------
"

## check headers only after we're done with pkg-config
AC_CHECK_HEADERS([blosc.h],, [AC_MSG_ERROR([blosc headers are not usable.
Please make sure you have installed development files for blosc.])])

# check that the version of blosc works with R package.
AC_MSG_CHECKING([whether available blosc version is good enough])
blosc_inst_ok=no
${CXX} ${CPPFLAGS} ${PKG_CPPFLAGS} ${CFLAGS} -E tools/version.cpp >/dev/null 2>&1 && blosc_inst_ok=yes
AC_MSG_RESULT([${blosc_inst_ok}])

AC_MSG_CHECKING([whether blosc programs can be compiled])
AC_LINK_IFELSE([AC_LANG_SOURCE([
#include <blosc.h>
int main(void) {
  blosc_init();
  int n = blosc_get_nthreads();
  return 0;
}
])],[AC_MSG_RESULT(yes)],
[AC_MSG_ERROR([Cannot compile a simple blosc program. See config.log for details.])])


AC_SUBST([PKG_CPPFLAGS], ["${PKG_CPPFLAGS}"])
AC_SUBST([PKG_LIBS], ["${PKG_LIBS}"])
AC_CONFIG_FILES([src/Makevars])

AC_OUTPUT

