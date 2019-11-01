#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(explode, 0.1pre)
AM_INIT_AUTOMAKE
# AC_CONFIG_SRCDIR([berkeleydb/BerkeleyDB.C])
AC_CONFIG_HEADER([config.h])

AC_CANONICAL_HOST

dnl temporary flags
dnl path to fs progs root
AC_ARG_WITH(fsprog-root,
	[--with-fsprog-root=DIR	Root directory to FS utilties.],
	[FSPROG_ROOT=$withval],
	[FSPROG_ROOT="/"]) dnl "$PWD/$ac_top_srcdir/check"])
if test -d "$FSPROG_ROOT/sbin"; then
   echo "Using fsprog root $FSPROG_ROOT."
else
   echo "can't find fsprog under $FSPROG_ROOT. "\
	"specify with option --with-fsprog-root!"
   exit 1
fi
AC_DEFINE_UNQUOTED(FSPROG_ROOT, "$FSPROG_ROOT",
      [Define to be the root directory where fs progs are located])

AC_ARG_ENABLE(exception,
	[--enable-exception=yes	throw an exception on errors or aborts],
	,
	[enaable_exception="yes"])
if test "${enable_exception}" != no; then
   AC_DEFINE([CONF_USE_EXCEPTION], 1,
     [Define to throw an exception on errors or aborts])
fi

#AC_ARG_ENABLE(new-interface,
#	[--enable-new-interface=no	use new checking interface.],
#	,
#	[enable_new_interface="no"])
#if test "${enable_new_interface}" != no; then
#   AC_DEFINE([CONF_NEW_INTERFACE], 1,
#     [Define to use the new checking interface.])
#fi

AC_ARG_ENABLE(diagnosis,
	[--enable-diagnosis=no experimental. automatic diagnose],
	,
	[enable_diagnosis="no"])
if test "${enable_diagnosis}" != no; then
   AC_DEFINE([CONF_DIAGNOSIS], 1,
     [Define to enable automatic diagnosis of errors])
fi

dnl those are real flags that should be used
dnl select kernel modules based on host os type
case $host_os in
     *linux*)

     dnl setup kernel path
     AC_ARG_WITH(kernel,
		[--with-kernel=DIR	Directory to the kernel to be checked.],
		[KERNEL_DIR=$withval],
		[KERNEL_DIR="$PWD/$ac_top_srcdir/kernel/linux-2.6.24.3"])
		if test -d $KERNEL_DIR; then
		  echo "Using $KERNEL_DIR to build kernel modules."
                  EKM=ekm-linux-2.6
		  RDD=rdd-linux-2.6
                  MTD=mtd-linux-2.6

                  AC_CONFIG_FILES(ekm-linux-2.6/Makefile 
                                  rdd-linux-2.6/Makefile
                                  mtd-linux-2.6/Makefile
                                  )

  		  AC_DEFINE([CONF_EKM_LOG_SYSCALL], 1,
		    [Define to log system calls])
		  AC_DEFINE([CONF_EKM_CHOICE], 1,
		    [Define to enable ekm choice])
		  AC_DEFINE([CONF_EKM_THREAD], 1,
		    [Define to enable ekm thread scheduling control])
		  AC_DEFINE([CONF_EKM_GENERATE], 1,
		    [Define to enable auto-generation of ekm logging code])
		else
                  if test "${KERNEL_DIR}" == "no"; then 
                    echo "Building user-space components only"
                  else 
                     echo "Can't find kernel at $KERNEL_DIR! specify with --with-kernel"
                     echo "If you only want to build the user-space commponents, use   "
                     echo "   --with-kernel=no                                         "
		     exit 1
                  fi
		fi
     ;;
     *freebsd*)	
	EKM=ekm-bsd-6.0
	RDD=rdd-bsd-6.0
        MTD=mtd-bsd-6.0
        KERNEL_DIR="yes"
        echo "Using BSD modules"
dnl        AC_CONFIG_FILES(ekm-bsd-6.0/Makefile 
dnl                        rdd-bsd-6.0/Makefile)

     ;;
     *)
     	echo "$host_os not supported!"
	exit 1
     ;;
esac

dnl set up symlinks for kernel modules

if test "${KERNEL_DIR}" != no; then
        #AC_CHECK_HEADERS([boost/shared_ptr.hpp])
	AC_SUBST(KERNEL_DIR)
        AC_SUBST(EKM)
        AC_SUBST(RDD)
        AC_SUBST(MTD)
        echo "creating kernel module symlinks"
        rm -f ekm rdd mtd
        ln -s ${EKM} ekm
        ln -s ${RDD} rdd
        ln -s ${MTD} mtd
fi


AC_SUBST(CONF_EKM_GENERATE)

dnl check for berkeley db
SFS_SLEEPYCAT(4.6 4.5 4.4 4.3 4.2 4.0 4 3, no)
AC_SUBST(DBLIB)
AC_SUBST(DBDEPS)
if test "${with_db3}" != no; then
	DBLIB="$DBLIB $DB_LIB"
        #AC_CHECK_HEADERS([db.h])
        echo "will check berkeley db"
else
	dnl AC_MSG_ERROR("Sleepycat is required to build")
        echo "will not check berkeley db"
fi
AM_CONDITIONAL([CHECK_BDB], [test x$with_db3 != xno])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h unistd.h execinfo.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT64_T
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_PROG_GCC_TRADITIONAL
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit bzero dup2 fdatasync ftruncate getcwd gethostbyname lchown memset mkdir rmdir select socket strchr strdup strerror strrchr strstr strtoul backtrace backtrace_symbols])

CPPFLAGS="$CPPFLAGS "'-I$(top_srcdir) -I$(top_srcdir)/mcl -I$(top_srcdir)/rpc -I$(top_srcdir)/crash'

AC_SUBST(CPPFLAGS)

LIBMC='$(top_builddir)/mcl/libmc.a'
AC_SUBST(LIBMC)

LIBCRASH='$(top_builddir)/crash/libcrash.a'
AC_SUBST(LIBCRASH)

AC_CONFIG_FILES(Makefile
		scripts/Makefile
		mcl/Makefile
		storage/Makefile
		driver/Makefile
		rpc/Makefile
		crash/Makefile
		check/Makefile
		testsuite/Makefile
		)

AC_OUTPUT
