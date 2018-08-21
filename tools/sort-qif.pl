#!/usr/bin/env perl
#
# sort-qif.pl -- sort QIF file
#
# Sort QIF file.  This assumes that the header lists in the QIF file have a
# leading comment that looks like '# stream $number'.
#
# Usage: sort-qif.pl [--strip-comments] [files]

use Getopt::Long;

GetOptions("strip-comments" => \$strip_comments);

$/ = "\n\n";

@chunks = map $$_[1],
          sort { $$a[0] <=> $$b[0] }
          map { /^*#\s*stream\s+(\d+)/; [ $1, $_ ] }
          <>;

if ($strip_comments) {
    for (@chunks) {
        s/^#.*\n//mg;
    }
}

print @chunks;
