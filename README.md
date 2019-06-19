[![Build Status](https://travis-ci.org/litespeedtech/ls-qpack.svg?branch=master)](https://travis-ci.org/litespeedtech/ls-qpack)
[![Build status](https://ci.appveyor.com/api/projects/status/kat31lt42ds0rmom?svg=true)](https://ci.appveyor.com/project/litespeedtech/ls-qpack)
[![Build Status](https://api.cirrus-ci.com/github/litespeedtech/ls-qpack.svg)](https://cirrus-ci.com/github/litespeedtech/ls-qpack)

# ls-qpack
QPACK compression library for use with HTTP/3

## Introduction

QPACK is a new compression mechanism for use with HTTP/3.  It is
in the process of being standardazed by the QUIC Working Group.  This
library is meant to aid in the development of the HTTP/3 protocol.  At
the same time, we aim to make it production quality from the start, as
we will use it in our own software products.

## API

The API is documented in the header file, [lsqpack.h](lsqpack.h).
One example how it is used in real code can be seen in
[lsquic-client](https://github.com/litespeedtech/lsquic-client).  (At the
time of this writing, the IETF code is on a branch.  Looks for the latest.)
