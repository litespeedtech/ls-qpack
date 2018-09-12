#!/usr/bin/env perl
#
# Given a QIF file, randomize cookie headers
#
# Usage: randomize-cookies.pl input.qif > output.qif

@chars = ('0' .. '9', 'a' .. 'z', 'A' .. 'Z', split '', '/+*');

sub randomify {
    $val = shift;

    if (exists($cached_vals{$val})) {
        return $cached_vals{$val};
    }

    @val = split '', $val;
    $seen_eq = 0;
    for ($i = 0; $i < @val; ++$i)
    {
        if ($val[$i] ne '=' or $seen_eq++)
        {
            $rand = $rand[$i] //= int(rand(@chars));
            # Output is a function of both position and value:
            $val[$i] = $chars[ ($rand + ord($val[$i])) % @chars ];
        }
    }

    return $cached_vals{$val} = join '', @val;
}

while (<>) {
    chomp;
    if (m/^cookie\t(.*)/) {
        print "cookie\t", randomify($1), "\n";
    } elsif (m/^set-cookie\t(.*)/) {
        print "set-cookie\t", join('; ', map randomify($_),
                                                    split /;\s+/, $1), "\n";
    } else {
        print $_, "\n";
    }
}
