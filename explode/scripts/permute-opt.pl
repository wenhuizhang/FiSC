#!/usr/bin/perl -w

usage() if scalar @ARGV != 2;

$topdir = shift @ARGV;
$permf = shift @ARGV;
%options = ();
@permutations = ();

@optfs = ("$topdir/explode.default.options",
	  "$topdir/explode.options");

foreach $optf (@optfs) {
    read_optf ($optf, \%options);
}

#emit_options(\%options, "/dev/stdout");
read_opt_permf($permf, \@permutations);

foreach $section_ref (@permutations) {
    push @num_choices, scalar @$section_ref;
    push @cur_choices, 0;
}

$n = 0;
$done = 0;
while(!$done) {
    print "generating permutation $n...";
    %options_copy = %options;
    %update_opts = ();
    for $i (0..$#permutations) {
	update_options(\%update_opts, $permutations[$i][$cur_choices[$i]]);
    }

    update_options(\%options_copy, \%update_opts);
    emit_options(\%options_copy, name_from_opt(\%update_opts));
    print "done\n";
    $n ++;

    $j = 0;
    while($j < scalar @permutations && 
	  (++$cur_choices[$j]) == $num_choices[$j]) {
	$cur_choices[$j] = 0;
	$j ++;
    }

    $done = 1 if($j >= scalar @permutations);
}

#    foreach $p2 (@$p1) {
#	foreach $opt (keys %$p2) {
#	    print $opt, " ", $p2->{$opt}, "\n";
#	}
#    }


sub read_optf
{
    my ($file, $optref) = @_;
    return unless -f $file;

    open OPTF, $file || die $!;
    while (<OPTF>) {
	# should have a preprocessing step, so we can have
	# branches in the option file
	# if(/^\#\@\s*if/) {
# 	    $state = "if";
# 	} elsif(/^\#\@\s*elif/) {
# 	    $state = "if";
# 	} elsif (/^\#\@\s*else/) {
# 	    $state = "if";
# 	} elsif (/^\#\@\s*endif/) {
# 	    $state = "if";
#	}
	next if /^\#/;
	if(/^([^\s]+)::([^\s]+)\s*$/) {
	    warn "warning: No value for $1::$2 in $file!\n";
	    exit(1);
	} elsif (/^([^\s:]+):([^\s:]+)/) {
	    warn "warning: by $1:$2 do you really mean $1::$2?\n";
	    exit(1);
	}
	next if !/^([^\s]+)::([^\s]+)\s+([^\s]+)\s*$/;
	$optref->{$1}{$2} = $3;
    }
    close OPTF;
}

sub read_opt_permf
{
    local ($file, $permref) = @_;
    local ($zero_nonselected = 0, %opts = (), 
	   $i = 0, $j, $choice_ref, $section_ref);

    open PERMF, $file || die $!;
    while(<PERMF>) {
	next if /^\#/;
	if(/^\s*BEGIN/../^\s*END/) {

	    if(/^\s*BEGIN_ZERO_NONSELECTED/) {
		$zero_nonselected = 1;
		%opts = ();
	    }

	    if(/^\s*END/) {
		if($zero_nonselected) {
		    $section_ref = $permref->[$i];
		    foreach $choice_ref (@$section_ref) {
			foreach $dom (keys %opts) {
			    foreach $key (keys %{$opts{$dom}}) {
				if(!defined($choice_ref->{$dom}{$key})) {
				    $choice_ref->{$dom}{$key} = 0;
				}
			    }
			}
		    }
		    $zero_nonselected = 0;
		}
		$i ++;
	    }

	    $j = 0 if(/^\s*BEGIN/);

	    $j ++ if (/^\s*OR/);

	    if(/^([^\s]+)::([^\s]+)\s+([^\s]+)\s*$/) {
		$dom = $1;
		$key = $2;
		$permref->[$i][$j]{$dom}{$key} = $3;
		if($zero_nonselected) {
		    $opts{$dom}{$key} = 1;
		}
	    }
	}
    }
    close PERMF;
}

sub emit_options
{
    my ($optref, $file) = @_;
    open OPT, ">$file" || die $!;
    print OPT "# permutation $n\n";
    foreach $dom (sort keys %$optref) {
	foreach $key (sort keys %{$optref->{$dom}}) {
	    $val = $optref->{$dom}{$key};
	    print OPT "$dom"."::$key $val\n";
	}
    }
    close OPT;
}

sub update_options
{
    my ($optref, $update_ref) = @_;
    foreach $dom (sort keys %$update_ref) {
	foreach $key (sort keys %{$update_ref->{$dom}}) {
	    #defined($optref->{$dom}{$key}) || die;
	    $optref->{$dom}{$key} = $update_ref->{$dom}{$key};
	}
    }
}

sub name_from_opt
{
    my ($optref) = @_;
    local $name = "";
    foreach $dom (sort keys %$optref) {
	foreach $key (sort keys %{$optref->{$dom}}) {
	    $val = $optref->{$dom}{$key};
	    $name .="$key"._."$val"."_";
	}
    }
    return $name. ".options";
}

sub usage
{
    print "$0 <dir to default options>  <permutation config>\n";
    exit(1);
}
