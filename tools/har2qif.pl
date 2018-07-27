#!/usr/bin/env perl
#
# har2qif.pl    - Convert HAR file to a QIF file.
#
# QIF stands for "QPACK Interop Format."  It is meant to be easy to parse,
# write, and compare.  The idea is that the QIF input to an encoder can
# be compared to QIF output produced by the decoder using `diff(1)':
#
#   sh$ har2qif -requests my.har > in.qif
#   sh$ ./qpack-encoder-A in.qif > binary-format    # See wiki
#   sh$ ./qpack-decoder-B binary-format > out.qif
#   sh$ diff in.qif out.qif && echo "Success!"
#
# The QIF format is plain text.  Each header field in on a separate line,
# with name and value separated by the TAB character.  An empty line
# signifies the end of a header set.  Lines beginning with '#' are ignored.
#
# HTTP/2 header sets are left untouched, while non-HTTP/2 header sets are
# transformed to resemble HTTP/2:
#   - Header names are lowercased
#   - `Host' header name is removed from requests
#   - Requests get :method, :scheme, :authority, and :path pseudo-headers
#   - Responses get :status pseudo-headers

use strict;
use warnings;

use Getopt::Long;
use JSON qw(decode_json);
use URI;

my $key = 'request';
my $limit;

GetOptions( "help" => sub {
    print <<USAGE;
Usage: $0 [options] [file.har] > file.qif

Options:
    -requests       Print headers from requests.  This is the default.
    -responses      Print headers from responses.
    -limit N        Limit number of header sets to N.  The default
                      is no limit.

If file.har is not specified, the HAR is read from stdin
USAGE

    exit;
},
            "requests"  => sub { $key = "request", },
            "responses" => sub { $key = "response", },
            "limit=i"   => \$limit,
);

my $json = decode_json( do { undef $/; <> });
my @messages = map $$_{$key}, @{ $json->{log}{entries} };
if (defined($limit))
{
    splice @messages, $limit;
}

my @header_sets = do {
    if ($key eq 'request') {
        map req_header_set($_), @messages
    } else {
        map resp_header_set($_), @messages
    }
};

for (@header_sets) {
    print map "$$_[0]\t$$_[1]\n", @$_;
    print "\n";
}

exit;

# Looking at capitalization of the first header is a more reliable means
# of determining HTTP version than relying on httpVersion field.
#
sub is_http2 {
    my $message = shift;
    return $$message{headers}[0]{name} =~ /^[a-z:]/;
}

sub req_header_set {
    my $message = shift;
    if (!is_http2($message)) {
        my @headers = map [ lc($$_{name}), $$_{value}, ],
                      grep $$_{name} ne 'Host', @{ $$message{headers} };
        my $uri = URI->new($$message{url});
        return [
            [ ':method',    $$message{method}, ],
            [ ':scheme',    $uri->scheme, ],
            [ ':authority', $uri->authority, ],
            [ ':path',      $uri->path_query, ],
            @headers,
        ];
    } else {
        return [ map [ $$_{name}, $$_{value}, ], @{ $$message{headers} } ]
    }
}

sub resp_header_set {
    my $message = shift;
    if (!is_http2($message)) {
        my @headers = map [ lc($$_{name}), $$_{value}, ],
                                                @{ $$message{headers} };
        my $uri = URI->new($$message{url});
        return [
            [ ':status',    $$message{status}, ],
            @headers,
        ];
    } else {
        return [ map [ $$_{name}, $$_{value}, ], @{ $$message{headers} } ]
    }
}
