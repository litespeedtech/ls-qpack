#!/usr/bin/env perl
#
# Given a QIF file, randomify cookie headers
#
# Usage: randomify-cookies.pl input.qif > output.qif

@chars = ('=', # '=' is special: we keep its position in the cookie
               '0' .. '9', 'a' .. 'z', 'A' .. 'Z', split '', '/+*');

sub randomify {
    @val = split '', shift;
    $seen_eq = 0;
    for ($i = 0; $i < @val; ++$i)
    {
        if ($val[$i] ne '=' or $seen_eq++)
        {
            $rand = $rand[$i] //= int(rand(@chars));
            # Output is a function of both position and value:
            $val[$i] = $chars[ ($rand + ord($val[$i])) % (@chars - 1) + 1 ];
        }
    }
    return join '', @val;
}

while (<>) {
    chomp;
    if (m/^cookie\t(.*)/) {
        print "cookie\t", randomify($1), "\n";
    } else {
        print $_, "\n";
    }
}
