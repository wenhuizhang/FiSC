dnl
dnl Use -ldb only if we need it.
dnl
AC_DEFUN(SFS_FIND_DB,
[AC_CHECK_FUNC(dbopen)
if test $ac_cv_func_dbopen = no; then
	AC_CHECK_LIB(db, dbopen)
	if test $ac_cv_lib_db_dbopen = no; then
	  AC_MSG_ERROR([Could not find library for dbopen!])
	fi
fi
])

dnl
dnl SFS_TRY_SLEEPYCAT_VERSION(vers, dir)
dnl
AC_DEFUN(SFS_TRY_SLEEPYCAT_VERSION,
[vers=$1
dir=$2
majvers=`echo $vers | sed -e 's/\..*//'`
minvers=`echo $vers | sed -e 's/[^.]*\.//' -e 's/\..*//'`
escvers=`echo $vers | sed -e 's/\./\\./g'`
catvers=`echo $vers | sed -e 's/\.//g'`
: sfs_try_sleepycat_version $vers $dir $majvers $minvers $escvers $catvers

unset db_header
unset db_library

for header in \
	$dir/include/db$vers/db.h $dir/include/db$catvers/db.h \
	$dir/include/db$majvers/db.h \
	$dir/include/db$catvers.h $dir/include/db$majvers.h \
	$dir/include/db.h
do
    test -f $header || continue
    AC_EGREP_CPP(^db_version_is $majvers *\. *$minvers *\$,
[#include "$header"
db_version_is DB_VERSION_MAJOR.DB_VERSION_MINOR
], db_header=$header; break)
done

if test "$db_header"; then
    for vdir in "$dir/lib/db$catvers" "$dir/lib/db$majvers" \
		"$dir/lib/db" "$dir/lib"
    do
        for library in $vdir/libdb-$vers.a $vdir/libdb-$vers.la \
	    $vdir/libdb$catvers.la $vdir/libdb.la $vdir/libdb$catvers.a
        do
    	if test -f $library; then
    	    db_library=$library
    	    break 2;
    	fi
        done
    done
    if test -z "$db_library"; then
	case $db_header in
	*/db.h)
	    test -f $dir/lib/libdb.a && db_library=$dir/lib/libdb.a
	    ;;
	esac
    fi
    if test "$db_library"; then
	case $db_header in
	*/db.h)
	    CPPFLAGS="$CPPFLAGS -I"`dirname $db_header`
	    ;;
	*)
	    ln -s $db_header db.h
	    ;;
	esac
	case $db_library in
	*.la)
	    DB_LIB=$db_library
	    ;;
	*.a)
	    minusl=`echo $db_library | sed -e 's/^.*\/lib\(.*\)\.a$/-l\1/'`
	    DB_LIB=-L`dirname $db_library`" $minusl"
	    ;;
	*/lib*.so.*)
	    minusl=`echo $db_library | sed -e 's/^.*\/lib\(.*\)\.so\..*/-l\1/'`
	    DB_LIB=-L`dirname $db_library`" $minusl"
	    ;;
	esac
    fi
fi])

dnl
dnl SFS_SLEEPYCAT(v1 v2 v3 ..., required)
dnl
dnl   Find BekeleyDB version v1, v2, or v3...
dnl      required can be "no" if DB is not required
dnl
AC_DEFUN(SFS_SLEEPYCAT,
[AC_ARG_WITH(db,
--with-db[[[=/usr/local]]]    specify path for BerkeleyDB (from sleepycat.com))
AC_SUBST(DB_DIR)
AC_CONFIG_SUBDIRS($DB_DIR)
AC_SUBST(DB_LIB)
unset DB_LIB

rm -f db.h

for vers in $1; do
    DB_DIR=`cd $srcdir && echo db-$vers.*/dist/`
    if test -d "$srcdir/$DB_DIR"; then
        DB_DIR=`echo $DB_DIR | sed -e 's!/$!!'`
	break
    else
	unset DB_DIR
    fi
done

test -z "${with_db+set}" && with_db=yes

AC_MSG_CHECKING(for BerkeleyDB library)
if test "$DB_DIR" -a "$with_db" = yes; then
    CPPFLAGS="$CPPFLAGS "'-I$(top_builddir)/'"$DB_DIR/dist"
    DB_LIB='$(top_builddir)/'"$DB_DIR/dist/.libs/libdb-*.a"
    AC_MSG_RESULT([using distribution in $DB_DIR subdirectory])
elif test x"$with_db" != xno; then
    if test "$with_db" = yes; then
	for vers in $1; do
	    for dir in "$prefix/BerkeleyDB.$vers" \
			"/usr/BerkeleyDB.$vers" \
			"/usr/local/BerkeleyDB.$vers" \
			$prefix /usr /usr/local; do
		SFS_TRY_SLEEPYCAT_VERSION($vers, $dir)
		test -z "$DB_LIB" || break 2
	    done
	done
    else
	for vers in $1; do
	    SFS_TRY_SLEEPYCAT_VERSION($vers, $with_db)
	    test -z "$DB_LIB" || break
	done
	test -z "$DB_LIB" && AC_MSG_ERROR(Cannot find BerkeleyDB in $with_db)
    fi
fi

if test x"$DB_LIB" != x; then
    AC_MSG_RESULT($DB_LIB)
    USE_DB=yes
else
    AC_MSG_RESULT(no)
    USE_DB=no
    if test "$2" != "no"; then
        AC_MSG_ERROR(Cannot find BerkeleyDB)
    fi
fi

AM_CONDITIONAL(USE_DB, test "$USE_DB" = yes)
])
