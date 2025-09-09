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
      for pre in /usr /usr/X11 /usr/X11R6 /usr/local /opt /opt/R/arm64 opt/R/x86_64 /sw; do
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

# check that the version of blosc works with R package.
AC_MSG_CHECKING([whether the blosc version will work in R package])
blosc_ver_ok=no
${CXX} ${CPPFLAGS} ${PKG_CFLAGS} ${CFLAGS} -E tools/version.cpp >/dev/null 2>&1 && libgit2_ver_ok=yes
AC_MSG_RESULT([${blosc_ver_ok}])

echo "
  --------------------------------------------------
  Configuration for ${PACKAGE_NAME}

    cppflags: ${PKG_CPPFLAGS}
    libs:     ${PKG_LIBS}

  --------------------------------------------------
"
AC_SUBST([PKG_CPPFLAGS], ["${PKG_CPPFLAGS}"])
AC_SUBST([PKG_LIBS], ["${PKG_LIBS}"])
AC_CONFIG_FILES([src/Makevars])

AC_OUTPUT

