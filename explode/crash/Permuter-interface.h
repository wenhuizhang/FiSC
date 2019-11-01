#if 0 /* -*-perl-*- */
# /* 

# Copyright (C) 2008 Junfeng Yang (junfeng@cs.columbia.edu)
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA


# This file is both a perl script and a header file.  The idea is
# borrowed from Dave Mazieres's sfs code (specifically,
# async/callback.h).


# WHAT IS THIS?

# This file declares and implements eXplode's crash-checking
# interface.  There are 4 methods users use to check for crashes,
# check_all_crashes(), check_now_crashes(), begin_commit() and
# end_commit().  All methods are template methods to provide users
# freedom on choosing the name and signature for the check method.
# Here is an example:
#
# mutate () {
#     write data to filename
#     fsync(filename)
#     check_now_crashes(this, check_fsync, filename, data);
# }
#
# To check fsync, the user's test driver implements a check_fsync
# method, which takes two arguments, the file name and the data
# sync'ed.  User can then call check_now_crashes as shown in
# the example.  eplode will call check_fsync with arguments
# filename and data.
#
# In general, all crash-checking methods take one pointer argument
# to the test driver class, one optional pointer to the check
# method, and up to $NA arbitrary arguments.



# WHAT DO THESE METHODS CHECK?
#
# (1) check_now_crashes().  Check all possible crashes that
# can happen now.
#
# (2) check_all_crashes().  Check all crashes from the last time
# we checked crashes or called forget_old_crashes() until now.
#
# (3) Users can use the two methods, begin_commit() and
# end_commit(), together with the enum commit_epoch_t (declared in
# Permuter.h) to check atomic commits.  Below is a contrived
# example below that shows this use.
#
# mutate() {
#     ...
#     begin_commit(this, arg...);
#     transaction_commit(...);
#     end_commit(this, arg...);
#     ...
# }
#
# check(commit_epoch_t epoch, arg...) {
#     switch(epoch) {
#         case before_commit: // check recovered state is the old state
#         case in_commit: // check recovered state is either old or new
#         case after_commit: // check recovered state is the new state
#     };
# }
#
# In the mutate method, user calls begin_commit() before the start
# of an atomic commit, and end_commit() afterwards.  As in
# check_all_crashes() or check_now_crashes(), user can pass up
# to $NA arbitrary arguments.  
#
#                 begin_commit    end_commit
# --------------------^---------------^--------------------
# |<--before_commit-->|<--in_commit-->|<--after_commit-->|
#
# The calls to begin_commit() and end_commit() split the time into
# three epochs, before_commit, in_commit and after_commit, shown
# in the above figure.
#
# eXplode notifies the user-provided check method about the three
# epoches through the first parameter it passes into check.  
# User can then perform the specific checks based on the epoch.

eval 'exec perl -w -S $0 ${1+"$@"}'
    if 0;

use strict;

my $NA = 5;

sub jc {
    join (', ', @_);
}

sub mklist ($$) {
    my ($pat, $n) = @_;
    my @res = map { my $ret = $pat; $ret =~ s/%/$_/g; $ret; } (1 .. $n);
    wantarray ? @res : jc (@res);
}

sub pchecker_c_a ($) 
{
    my ($a) = @_;

    my $type = "checker_c_${a}";
    my $tmpparam = jc('class C', mklist('class A%', $a));
    my $Alist = mklist('A%', $a);
    my $adecl = join("\n", mklist("    A% a%;", $a));
    my $aargs = jc('const P &cc', 'cb_t ff', mklist('const A% &aa%', $a));
    my $ainit = jc('c(cc)', 'f(ff)', mklist('a%(aa%)', $a));
    my $alist = mklist('a%', $a);

    print <<EOF;

template<$tmpparam>
class $type: public checker {
    typedef C* P;
    typedef int (C::*cb_t) ($Alist);
    P c;
    cb_t f;
${adecl}
public:
    $type($aargs)
    : $ainit {}
    int operator() (void)
    { return ((*c).*f) ($alist);}
};
EOF

    my $bc_Alist = jc('commit_epoch_t', mklist('A%', $a));
    my $bc_alist = jc('before_commit', mklist('a%', $a)); 

    print <<EOF;
template<$tmpparam>
class bc_${type}: public checker {
    typedef C* P;
    typedef int (C::*cb_t) ($bc_Alist);
    P c;
    cb_t f;
${adecl}
public:
    bc_${type}($aargs)
    : $ainit {}
    int operator() (void)
    { return ((*c).*f) ($bc_alist);}
};
EOF

    my $ac_Alist = $bc_Alist;
    my $ic_alist = jc('in_commit', mklist('a%', $a));
    my $ac_alist = jc('after_commit', mklist('a%', $a));

    print <<EOF;
template<$tmpparam>
class ac_${type}: public checker {
    typedef C* P;
    typedef int (C::*cb_t) ($ac_Alist);
    Permuter *perm;
    P c;
    cb_t f;
${adecl}
public:
    ac_${type}(Permuter* p, $aargs)
    : perm(p), $ainit {}
    int operator() (void)
    { 
        if(!perm->time_is_present())
            return ((*c).*f) ($ic_alist);
        else
            return ((*c).*f) ($ac_alist);
    }
};

EOF

    my $tmpalist = jc('C', mklist('A%', $a));
    my $specparam = jc('class C', mklist('class A%, class AA%', $a));
    my $specaargs = jc('C* cc', "int (C::*ff)($Alist)", 
                       mklist('const AA% &aa%', $a));
    my $c_Alist = jc('enum commit_epoch_t', mklist('A%', $a));
    my $c_specaargs = jc('C* cc', "int (C::*ff)($c_Alist)", 
                         mklist('const AA% &aa%', $a));
    my $ac_specaargs = jc('Permuter *', $c_specaargs);

    $alist = jc('cc', 'ff', mklist('aa%', $a));
    $ac_alist = jc('this', 'cc', 'ff', mklist('aa%', $a));

    print <<EOF;

template<$specparam>
checker* new_checker($specaargs)
{
    return new $type<$tmpalist>($alist);
}

template<$specparam>
checker* new_bc_checker($c_specaargs)
{
    return new bc_${type}<$tmpalist>($alist);
}

template<$specparam>
checker* new_ac_checker($ac_specaargs)
{
    return new ac_${type}<$tmpalist>($ac_alist);
}

EOF

    print <<EOF;

// These APIs allow users to pass any member functions as the check method
template<$specparam>
void check_all_crashes_ex($specaargs)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker($alist));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<$specparam>
void check_now_crashes_ex($specaargs)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker($alist));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<$specparam>
void begin_commit_ex($c_specaargs)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker($alist));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<$specparam>
void end_commit_ex($c_specaargs)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker($ac_alist));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

EOF


    $specparam = jc('class C', mklist('class AA%', $a));
    $specaargs = jc('C* cc', mklist('const AA% &aa%', $a));
    $c_specaargs = jc('C* cc', mklist('const AA% &aa%', $a));

    $alist = jc('cc', '&C::check', mklist('aa%', $a));
    $ac_alist = jc('this', 'cc', '&C::check', mklist('aa%', $a));

    print <<EOF;

// These APIs use the default C::check method to check crashes
template<$specparam>
void check_all_crashes_ex($specaargs)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker($alist));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<$specparam>
void check_now_crashes_ex($specaargs)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker($alist));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<$specparam>
void begin_commit_ex($c_specaargs)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker($alist));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<$specparam>
void end_commit_ex($c_specaargs)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker($ac_alist));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

EOF

}

sub pfile () {

    my $a;
    for ($a = 0; $a <= $NA; $a++) {
        pchecker_c_a($a); 
    }
}

seek DATA, 0, 0;
while (<DATA>) {
    print;
    last if m/^\#endif \/\* perl \*\/$/;
}

print <<'EOF';

#ifndef _PERMUTER_INTERFACE_H_INCLUDED_
#define _PERMUTER_INTERFACE_H_INCLUDED_ 1

EOF

pfile;

print "\n", '#endif /', '* !_PERMUTER_INTERFACE_H_INCLUDED_ *', '/', "\n";

__END__
# */
#endif /* perl */

#ifndef _PERMUTER_INTERFACE_H_INCLUDED_
#define _PERMUTER_INTERFACE_H_INCLUDED_ 1


template<class C>
class checker_c_0: public checker {
    typedef C* P;
    typedef int (C::*cb_t) ();
    P c;
    cb_t f;

public:
    checker_c_0(const P &cc, cb_t ff)
    : c(cc), f(ff) {}
    int operator() (void)
    { return ((*c).*f) ();}
};
template<class C>
class bc_checker_c_0: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t);
    P c;
    cb_t f;

public:
    bc_checker_c_0(const P &cc, cb_t ff)
    : c(cc), f(ff) {}
    int operator() (void)
    { return ((*c).*f) (before_commit);}
};
template<class C>
class ac_checker_c_0: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t);
    Permuter *perm;
    P c;
    cb_t f;

public:
    ac_checker_c_0(Permuter* p, const P &cc, cb_t ff)
    : perm(p), c(cc), f(ff) {}
    int operator() (void)
    { 
        if(!perm->time_is_present())
            return ((*c).*f) (in_commit);
        else
            return ((*c).*f) (after_commit);
    }
};


template<class C>
checker* new_checker(C* cc, int (C::*ff)())
{
    return new checker_c_0<C>(cc, ff);
}

template<class C>
checker* new_bc_checker(C* cc, int (C::*ff)(enum commit_epoch_t))
{
    return new bc_checker_c_0<C>(cc, ff);
}

template<class C>
checker* new_ac_checker(Permuter *, C* cc, int (C::*ff)(enum commit_epoch_t))
{
    return new ac_checker_c_0<C>(this, cc, ff);
}


// These APIs allow users to pass any member functions as the check method
template<class C>
void check_all_crashes_ex(C* cc, int (C::*ff)())
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C>
void check_now_crashes_ex(C* cc, int (C::*ff)())
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C>
void begin_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t))
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, ff));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C>
void end_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t))
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, ff));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


// These APIs use the default C::check method to check crashes
template<class C>
void check_all_crashes_ex(C* cc)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C>
void check_now_crashes_ex(C* cc)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C>
void begin_commit_ex(C* cc)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, &C::check));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C>
void end_commit_ex(C* cc)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, &C::check));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


template<class C, class A1>
class checker_c_1: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (A1);
    P c;
    cb_t f;
    A1 a1;
public:
    checker_c_1(const P &cc, cb_t ff, const A1 &aa1)
    : c(cc), f(ff), a1(aa1) {}
    int operator() (void)
    { return ((*c).*f) (a1);}
};
template<class C, class A1>
class bc_checker_c_1: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1);
    P c;
    cb_t f;
    A1 a1;
public:
    bc_checker_c_1(const P &cc, cb_t ff, const A1 &aa1)
    : c(cc), f(ff), a1(aa1) {}
    int operator() (void)
    { return ((*c).*f) (before_commit, a1);}
};
template<class C, class A1>
class ac_checker_c_1: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1);
    Permuter *perm;
    P c;
    cb_t f;
    A1 a1;
public:
    ac_checker_c_1(Permuter* p, const P &cc, cb_t ff, const A1 &aa1)
    : perm(p), c(cc), f(ff), a1(aa1) {}
    int operator() (void)
    { 
        if(!perm->time_is_present())
            return ((*c).*f) (in_commit, a1);
        else
            return ((*c).*f) (after_commit, a1);
    }
};


template<class C, class A1, class AA1>
checker* new_checker(C* cc, int (C::*ff)(A1), const AA1 &aa1)
{
    return new checker_c_1<C, A1>(cc, ff, aa1);
}

template<class C, class A1, class AA1>
checker* new_bc_checker(C* cc, int (C::*ff)(enum commit_epoch_t, A1), const AA1 &aa1)
{
    return new bc_checker_c_1<C, A1>(cc, ff, aa1);
}

template<class C, class A1, class AA1>
checker* new_ac_checker(Permuter *, C* cc, int (C::*ff)(enum commit_epoch_t, A1), const AA1 &aa1)
{
    return new ac_checker_c_1<C, A1>(this, cc, ff, aa1);
}


// These APIs allow users to pass any member functions as the check method
template<class C, class A1, class AA1>
void check_all_crashes_ex(C* cc, int (C::*ff)(A1), const AA1 &aa1)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1>
void check_now_crashes_ex(C* cc, int (C::*ff)(A1), const AA1 &aa1)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1>
void begin_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1), const AA1 &aa1)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, ff, aa1));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1>
void end_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1), const AA1 &aa1)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, ff, aa1));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


// These APIs use the default C::check method to check crashes
template<class C, class AA1>
void check_all_crashes_ex(C* cc, const AA1 &aa1)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1>
void check_now_crashes_ex(C* cc, const AA1 &aa1)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1>
void begin_commit_ex(C* cc, const AA1 &aa1)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, &C::check, aa1));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1>
void end_commit_ex(C* cc, const AA1 &aa1)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, &C::check, aa1));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


template<class C, class A1, class A2>
class checker_c_2: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (A1, A2);
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
public:
    checker_c_2(const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2)
    : c(cc), f(ff), a1(aa1), a2(aa2) {}
    int operator() (void)
    { return ((*c).*f) (a1, a2);}
};
template<class C, class A1, class A2>
class bc_checker_c_2: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1, A2);
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
public:
    bc_checker_c_2(const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2)
    : c(cc), f(ff), a1(aa1), a2(aa2) {}
    int operator() (void)
    { return ((*c).*f) (before_commit, a1, a2);}
};
template<class C, class A1, class A2>
class ac_checker_c_2: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1, A2);
    Permuter *perm;
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
public:
    ac_checker_c_2(Permuter* p, const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2)
    : perm(p), c(cc), f(ff), a1(aa1), a2(aa2) {}
    int operator() (void)
    { 
        if(!perm->time_is_present())
            return ((*c).*f) (in_commit, a1, a2);
        else
            return ((*c).*f) (after_commit, a1, a2);
    }
};


template<class C, class A1, class AA1, class A2, class AA2>
checker* new_checker(C* cc, int (C::*ff)(A1, A2), const AA1 &aa1, const AA2 &aa2)
{
    return new checker_c_2<C, A1, A2>(cc, ff, aa1, aa2);
}

template<class C, class A1, class AA1, class A2, class AA2>
checker* new_bc_checker(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2), const AA1 &aa1, const AA2 &aa2)
{
    return new bc_checker_c_2<C, A1, A2>(cc, ff, aa1, aa2);
}

template<class C, class A1, class AA1, class A2, class AA2>
checker* new_ac_checker(Permuter *, C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2), const AA1 &aa1, const AA2 &aa2)
{
    return new ac_checker_c_2<C, A1, A2>(this, cc, ff, aa1, aa2);
}


// These APIs allow users to pass any member functions as the check method
template<class C, class A1, class AA1, class A2, class AA2>
void check_all_crashes_ex(C* cc, int (C::*ff)(A1, A2), const AA1 &aa1, const AA2 &aa2)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1, aa2));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2>
void check_now_crashes_ex(C* cc, int (C::*ff)(A1, A2), const AA1 &aa1, const AA2 &aa2)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1, aa2));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2>
void begin_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2), const AA1 &aa1, const AA2 &aa2)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, ff, aa1, aa2));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2>
void end_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2), const AA1 &aa1, const AA2 &aa2)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, ff, aa1, aa2));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


// These APIs use the default C::check method to check crashes
template<class C, class AA1, class AA2>
void check_all_crashes_ex(C* cc, const AA1 &aa1, const AA2 &aa2)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1, aa2));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2>
void check_now_crashes_ex(C* cc, const AA1 &aa1, const AA2 &aa2)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1, aa2));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2>
void begin_commit_ex(C* cc, const AA1 &aa1, const AA2 &aa2)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, &C::check, aa1, aa2));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2>
void end_commit_ex(C* cc, const AA1 &aa1, const AA2 &aa2)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, &C::check, aa1, aa2));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


template<class C, class A1, class A2, class A3>
class checker_c_3: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (A1, A2, A3);
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
public:
    checker_c_3(const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3)
    : c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3) {}
    int operator() (void)
    { return ((*c).*f) (a1, a2, a3);}
};
template<class C, class A1, class A2, class A3>
class bc_checker_c_3: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1, A2, A3);
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
public:
    bc_checker_c_3(const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3)
    : c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3) {}
    int operator() (void)
    { return ((*c).*f) (before_commit, a1, a2, a3);}
};
template<class C, class A1, class A2, class A3>
class ac_checker_c_3: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1, A2, A3);
    Permuter *perm;
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
public:
    ac_checker_c_3(Permuter* p, const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3)
    : perm(p), c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3) {}
    int operator() (void)
    { 
        if(!perm->time_is_present())
            return ((*c).*f) (in_commit, a1, a2, a3);
        else
            return ((*c).*f) (after_commit, a1, a2, a3);
    }
};


template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3>
checker* new_checker(C* cc, int (C::*ff)(A1, A2, A3), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    return new checker_c_3<C, A1, A2, A3>(cc, ff, aa1, aa2, aa3);
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3>
checker* new_bc_checker(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    return new bc_checker_c_3<C, A1, A2, A3>(cc, ff, aa1, aa2, aa3);
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3>
checker* new_ac_checker(Permuter *, C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    return new ac_checker_c_3<C, A1, A2, A3>(this, cc, ff, aa1, aa2, aa3);
}


// These APIs allow users to pass any member functions as the check method
template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3>
void check_all_crashes_ex(C* cc, int (C::*ff)(A1, A2, A3), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1, aa2, aa3));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3>
void check_now_crashes_ex(C* cc, int (C::*ff)(A1, A2, A3), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1, aa2, aa3));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3>
void begin_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, ff, aa1, aa2, aa3));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3>
void end_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, ff, aa1, aa2, aa3));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


// These APIs use the default C::check method to check crashes
template<class C, class AA1, class AA2, class AA3>
void check_all_crashes_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1, aa2, aa3));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3>
void check_now_crashes_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1, aa2, aa3));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3>
void begin_commit_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, &C::check, aa1, aa2, aa3));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3>
void end_commit_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, &C::check, aa1, aa2, aa3));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


template<class C, class A1, class A2, class A3, class A4>
class checker_c_4: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (A1, A2, A3, A4);
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
    A4 a4;
public:
    checker_c_4(const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3, const A4 &aa4)
    : c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3), a4(aa4) {}
    int operator() (void)
    { return ((*c).*f) (a1, a2, a3, a4);}
};
template<class C, class A1, class A2, class A3, class A4>
class bc_checker_c_4: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1, A2, A3, A4);
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
    A4 a4;
public:
    bc_checker_c_4(const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3, const A4 &aa4)
    : c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3), a4(aa4) {}
    int operator() (void)
    { return ((*c).*f) (before_commit, a1, a2, a3, a4);}
};
template<class C, class A1, class A2, class A3, class A4>
class ac_checker_c_4: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1, A2, A3, A4);
    Permuter *perm;
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
    A4 a4;
public:
    ac_checker_c_4(Permuter* p, const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3, const A4 &aa4)
    : perm(p), c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3), a4(aa4) {}
    int operator() (void)
    { 
        if(!perm->time_is_present())
            return ((*c).*f) (in_commit, a1, a2, a3, a4);
        else
            return ((*c).*f) (after_commit, a1, a2, a3, a4);
    }
};


template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4>
checker* new_checker(C* cc, int (C::*ff)(A1, A2, A3, A4), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    return new checker_c_4<C, A1, A2, A3, A4>(cc, ff, aa1, aa2, aa3, aa4);
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4>
checker* new_bc_checker(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3, A4), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    return new bc_checker_c_4<C, A1, A2, A3, A4>(cc, ff, aa1, aa2, aa3, aa4);
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4>
checker* new_ac_checker(Permuter *, C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3, A4), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    return new ac_checker_c_4<C, A1, A2, A3, A4>(this, cc, ff, aa1, aa2, aa3, aa4);
}


// These APIs allow users to pass any member functions as the check method
template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4>
void check_all_crashes_ex(C* cc, int (C::*ff)(A1, A2, A3, A4), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1, aa2, aa3, aa4));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4>
void check_now_crashes_ex(C* cc, int (C::*ff)(A1, A2, A3, A4), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1, aa2, aa3, aa4));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4>
void begin_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3, A4), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, ff, aa1, aa2, aa3, aa4));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4>
void end_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3, A4), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, ff, aa1, aa2, aa3, aa4));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


// These APIs use the default C::check method to check crashes
template<class C, class AA1, class AA2, class AA3, class AA4>
void check_all_crashes_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1, aa2, aa3, aa4));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3, class AA4>
void check_now_crashes_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1, aa2, aa3, aa4));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3, class AA4>
void begin_commit_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, &C::check, aa1, aa2, aa3, aa4));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3, class AA4>
void end_commit_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, &C::check, aa1, aa2, aa3, aa4));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


template<class C, class A1, class A2, class A3, class A4, class A5>
class checker_c_5: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (A1, A2, A3, A4, A5);
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
    A4 a4;
    A5 a5;
public:
    checker_c_5(const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3, const A4 &aa4, const A5 &aa5)
    : c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3), a4(aa4), a5(aa5) {}
    int operator() (void)
    { return ((*c).*f) (a1, a2, a3, a4, a5);}
};
template<class C, class A1, class A2, class A3, class A4, class A5>
class bc_checker_c_5: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1, A2, A3, A4, A5);
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
    A4 a4;
    A5 a5;
public:
    bc_checker_c_5(const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3, const A4 &aa4, const A5 &aa5)
    : c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3), a4(aa4), a5(aa5) {}
    int operator() (void)
    { return ((*c).*f) (before_commit, a1, a2, a3, a4, a5);}
};
template<class C, class A1, class A2, class A3, class A4, class A5>
class ac_checker_c_5: public checker {
    typedef C* P;
    typedef int (C::*cb_t) (commit_epoch_t, A1, A2, A3, A4, A5);
    Permuter *perm;
    P c;
    cb_t f;
    A1 a1;
    A2 a2;
    A3 a3;
    A4 a4;
    A5 a5;
public:
    ac_checker_c_5(Permuter* p, const P &cc, cb_t ff, const A1 &aa1, const A2 &aa2, const A3 &aa3, const A4 &aa4, const A5 &aa5)
    : perm(p), c(cc), f(ff), a1(aa1), a2(aa2), a3(aa3), a4(aa4), a5(aa5) {}
    int operator() (void)
    { 
        if(!perm->time_is_present())
            return ((*c).*f) (in_commit, a1, a2, a3, a4, a5);
        else
            return ((*c).*f) (after_commit, a1, a2, a3, a4, a5);
    }
};


template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4, class A5, class AA5>
checker* new_checker(C* cc, int (C::*ff)(A1, A2, A3, A4, A5), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    return new checker_c_5<C, A1, A2, A3, A4, A5>(cc, ff, aa1, aa2, aa3, aa4, aa5);
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4, class A5, class AA5>
checker* new_bc_checker(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3, A4, A5), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    return new bc_checker_c_5<C, A1, A2, A3, A4, A5>(cc, ff, aa1, aa2, aa3, aa4, aa5);
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4, class A5, class AA5>
checker* new_ac_checker(Permuter *, C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3, A4, A5), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    return new ac_checker_c_5<C, A1, A2, A3, A4, A5>(this, cc, ff, aa1, aa2, aa3, aa4, aa5);
}


// These APIs allow users to pass any member functions as the check method
template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4, class A5, class AA5>
void check_all_crashes_ex(C* cc, int (C::*ff)(A1, A2, A3, A4, A5), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1, aa2, aa3, aa4, aa5));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4, class A5, class AA5>
void check_now_crashes_ex(C* cc, int (C::*ff)(A1, A2, A3, A4, A5), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, ff, aa1, aa2, aa3, aa4, aa5));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4, class A5, class AA5>
void begin_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3, A4, A5), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, ff, aa1, aa2, aa3, aa4, aa5));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class A1, class AA1, class A2, class AA2, class A3, class AA3, class A4, class AA4, class A5, class AA5>
void end_commit_ex(C* cc, int (C::*ff)(enum commit_epoch_t, A1, A2, A3, A4, A5), const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, ff, aa1, aa2, aa3, aa4, aa5));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


// These APIs use the default C::check method to check crashes
template<class C, class AA1, class AA2, class AA3, class AA4, class AA5>
void check_all_crashes_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1, aa2, aa3, aa4, aa5));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3, class AA4, class AA5>
void check_now_crashes_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_checker(cc, &C::check, aa1, aa2, aa3, aa4, aa5));
    checker_wants_now();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3, class AA4, class AA5>
void begin_commit_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_bc_checker(cc, &C::check, aa1, aa2, aa3, aa4, aa5));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}

template<class C, class AA1, class AA2, class AA3, class AA4, class AA5>
void end_commit_ex(C* cc, const AA1 &aa1, const AA2 &aa2, const AA3 &aa3, const AA4 &aa4, const AA5 &aa5)
{
    mark_cur_check();
    CheckingContext ctx(sim()->init_disks());
    std::auto_ptr<checker> c
        = std::auto_ptr<checker>(new_ac_checker(this, cc, &C::check, aa1, aa2, aa3, aa4, aa5));
    checker_wants_all();
    check_crashes(&ctx, c.get());
}


#endif /* !_PERMUTER_INTERFACE_H_INCLUDED_ */
