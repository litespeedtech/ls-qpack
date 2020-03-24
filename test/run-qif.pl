#!/usr/bin/perl
#
# Script to run a QIF file.

use strict;
use warnings;

use File::Compare qw(compare);
use File::Copy qw(copy);
use File::Path qw(remove_tree);
use File::Spec::Functions qw(catfile);
use Getopt::Long qw(GetOptions);

my $cleanup = 1;

GetOptions(
    "aggressive=i"      => \my $aggressive,
    "table-size=i"      => \my $table_size,
    "immed-ack=i"       => \my $immed_ack,
    "risked-streams=i"  => \my $risked_streams,
    "no-cleanup"        => sub { $cleanup = 0 },
);

# To reduce the number of tests from doubling, check HTTP/1.x mode in some
# input, but not others.  The value of this setting is easy to determine.
my $http1x = ($aggressive + $table_size + $immed_ack + $risked_streams) & 1;

my $dir = catfile(($ENV{TMP} || $ENV{TEMP} || "/tmp"),
                                                "run-qif-out-" . rand . $$);
mkdir $dir or die "cannot create temp directory $dir";
if (!$cleanup)
{
    print "created temp directory: $dir\n";
}

END {
    if (defined($dir) && $cleanup) {
        remove_tree($dir);
    }
}

my $encode_log = catfile($dir, "encode.log");
my $qif_file = catfile($dir, "qif");
my $bin_file = catfile($dir, "out");
my $resulting_qif_file = catfile($dir, "qif-result");

my ($encode_args, $decode_args) = ('', '');
if ($aggressive) {
    $encode_args="$encode_args -A";
}

if ($immed_ack) {
    $encode_args="$encode_args -a 1";
}

if (defined $risked_streams) {
    $encode_args = "$encode_args -s $risked_streams";
    $decode_args = "$decode_args -s $risked_streams";
}

if (defined $table_size) {
    $encode_args = "$encode_args -t $table_size";
    $decode_args = "$decode_args -t $table_size";
}

copy($ARGV[0], $qif_file) or die "cannot copy original $ARGV[0] to $qif_file";

if ($^O eq 'MSWin32') {
    system('interop-encode', $encode_args, '-i', $qif_file, '-o', $bin_file)
        and die "interop-encode failed";
    system('interop-decode', $decode_args, '-m', '1', '-i', $bin_file, '-o', $resulting_qif_file, '-H', $http1x)
        and die "interop-decode failed";
} else {
    system("interop-encode $encode_args -i $qif_file -o $bin_file")
        and die "interop-encode failed";
    system("interop-decode $decode_args -m 1 -i $bin_file -o $resulting_qif_file -H $http1x")
        and die "interop-decode failed";
}

sub sort_qif {
    no warnings 'uninitialized';
    my ($in, $out) = @_;
    local $/ = "\n\n";
    open F, $in or die "cannot open $in for reading: $!";
    my @chunks = map $$_[1],
		 sort { $$a[0] <=> $$b[0] }
		 map { /^#\s*stream\s+(\d+)/; [ $1, $_ ] }
		 <F>;
    close F;
    for (@chunks) {
        s/^#.*\n//mg;
    }
    open F, ">", $out or die "cannot open $out for writing: $!";
    print F @chunks;
    close F;
}

sort_qif($qif_file, "$qif_file.canonical");
sort_qif($resulting_qif_file, "$resulting_qif_file.canonical");

exit compare "$qif_file.canonical", "$resulting_qif_file.canonical";
