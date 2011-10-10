AC_INIT(OpenGeoDa, [1.0], [openspace@sal.uiuc.edu])
AC_PREREQ([2.56])
AC_CONFIG_SRCDIR(../OpenGeoDa.cpp)
AC_CONFIG_FILES([Makefile])

AC_LANG([C++])
AC_REQUIRE_CPP
AC_PROG_CXX


# The test below is used to attempt to a link using the given code
# (prologue) and flags.

# TEST_LIB(NAME, CODE, [EXTRA_LIBS], [EXTRA_CFLAGS])
# --------------------------------------------
AC_DEFUN([TEST_LIB], [
AC_MSG_CHECKING([for $1 usability])
xLIBS=$LIBS
xCFLAGS=$CFLAGS
LIBS="$LIBS $3"
CFLAGS="$CFLAGS $4"
AC_LINK_IFELSE(
[AC_LANG_PROGRAM([$2])],
[AC_MSG_RESULT([yes])],
[AC_MSG_FAILURE([unable to link $1])])
LIBS=$xLIBS
CFLAGS=$xCFLAGS
])


AC_CHECK_PROG([UNAME], [uname], [[yes]], [[no]])
if test $UNAME == "no"
then
	AC_MSG_ERROR([[For some reason, you don't have uname.]])
fi

AC_MSG_CHECKING([[host system, and assuming it is the target system]])
case `uname` in
	Darwin)		arch=MAC;;
	MINGW*)		arch=WINDOWS;;
	Linux) 		arch=LINUX;;
	*)
	arch=UNKNOWN
	AC_MSG_WARN(
	[[We have not built OpenGeoDa on your system before.]]
	[[Please contact us if you experience any problems.]])
	;;
esac
AC_MSG_RESULT([$arch])
AC_SUBST([arch])

AC_MSG_CHECKING([output file name])
if test $arch == MAC
then
	output_file=OpenGeoDa.app
else
	output_file=OpenGeoDa
fi
AC_MSG_RESULT([$output_file])
AC_SUBST([output_file])

if test $arch == MAC
then
	TEST_LIB([CoreFoundation], 
	[#include <CoreFoundation/CoreFoundation.h>],
	[-framework CoreFoundation])
fi
AC_SUBST([corefoundation_libs])

AC_MSG_CHECKING([[how to link OpenGL libraries]])
case $arch in
	MAC)	
	opengl_libs="-framework OpenGL"
	;;
	WINDOWS)	
	opengl_libs="-lglu32 -lopengl32"
	;;
	*)
	opengl_libs="-lGLU -lGL"
	;;
esac
AC_MSG_RESULT([$opengl_libs])

if test $arch == "MAC"
then
	TEST_LIB([OpenGL], [
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>],
	[$opengl_libs])
else
	TEST_LIB([OpenGL], [
	#include <GL/gl.h>
	#include <GL/glu.h>],
	[$opengl_libs])
fi
AC_SUBST([opengl_libs])

# FIXME: these hard-coded values are not going to cut it in the long
# run.  There must be a better way...
case $arch in
	MAC)	
	lapack_libs="-framework Veclib"
	lapack_cflags=""
	;;
	WINDOWS)	
	lapack_libs="-L/usr/lib -llapack -lblas -lF77 -lI77"
	lapack_cflags="-I/usr/include"
	;;
	LINUX)
	# FIXME: copied from WINDOWS.  consider adjusting
	# static linking clapack.  Very important that libraries are linked
 	# in the correct order (lapack, blas, then libf2c).
	lapack_cflags=""
	lapack_libs="$HOME/CLAPACK-3.2/lapack.a $HOME/CLAPACK-3.2/blas.a $HOME/CLAPACK-3.2/F2CLIBS/libf2c.a"
	;;
esac
AC_MSG_CHECKING([[linker flags for lapack]])
AC_MSG_RESULT([$lapack_libs])
AC_MSG_CHECKING([[compiler flags for lapack]])
AC_MSG_RESULT([$lapack_cflags])

if test $arch == "MAC"
then
	TEST_LIB([lapack], [
	#include <vecLib/vecLib.h>],
	[$lapack_libs], [$lapack_cflags])
else
	TEST_LIB([lapack], [
	#include "../Regression/blaswrap.h"
	#include "../Regression/f2c.h"
	#include "../Regression/clapack.h"],
	[$lapack_libs], [$lapack_cflags])
fi
AC_SUBST([lapack_libs])
AC_SUBST([lapack_cflags])

AC_CHECK_PROG([WXCFG], [wx-config], [[yes]], [[no]])
if test $WXCFG == "yes"
then
	AC_MSG_CHECKING([[wxWidgets libraries using wx-config]])
	wx_libs=`wx-config --libs std,gl`
	AC_MSG_RESULT([$wx_libs])

	AC_MSG_CHECKING([[wxWidgets cxxflags using wx-config]])
	wx_cxxflags=`wx-config --cxxflags`
	AC_MSG_RESULT([$wx_cxxflags])

	if test $arch == "MAC"
	then
		AC_MSG_CHECKING([[how to invoke Rez]])
		rez_command=`wx-config --rezflags`
		AC_MSG_RESULT([$rez_command])
	fi
else
	AC_MSG_ERROR(
	[[wx-config not present -- are you sure the]]
	[[wxWidgets libraries are properly installed?]])
fi
AC_SUBST([wx_libs])
AC_SUBST([wx_cxxflags])
AC_SUBST([rez_command])

AC_OUTPUT
