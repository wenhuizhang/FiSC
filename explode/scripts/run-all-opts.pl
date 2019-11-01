#!/usr/bin/perl -w

usage() if scalar @ARGV != 3;

$checker = shift @ARGV;
$optdir = shift @ARGV;
$outputdir = shift @ARGV;

mkdir $outputdir, 0777;

@all_opts = glob("$optdir/*.options");
foreach $opt (sort @all_opts) {
    run_opt($opt);
}

sub run_opt
{
    local($opt) = @_;
    local($dir) = $opt =~ /\/([^\/]+)$/;

    print $dir, "\n";
    $dir =~ s/\s+\.options//;
    $dir = $outputdir . "/" . $dir;
    mkdir $dir, 0777;
    local($log = $dir."/log");
    local($cmd = "$checker -o $opt -sexplode::trace_dir=$dir &> $log");

    if($opt =~ /(msdos|vfat|ntfs|hfs)/) {
	$cmd = "sudo $cmd";
    }

    print "running command <$cmd>\n";
    system $cmd;

    print "recording dmesg output\n";
    system "dmesg > $dir/dmesg";

    print "examining dmesg output\n";
    check_dmesg("$dir/dmesg");
}

sub check_dmesg
{
    local($dmesg) = @_;
    open DMESG, $dmesg || die $!;
    local $content = do {local $/; <DMESG>};
    close DMESG;

    if($content =~ /kernel BUG/) {
	print $content;
	exit(1);
    }
}

sub usage
{
    print "$0 <checker> <dir to all options> <output dir>\n";
    exit(1);
}
