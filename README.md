[![Build Status](https://travis-ci.org/litespeedtech/ls-qpack.svg?branch=master)](https://travis-ci.org/litespeedtech/ls-qpack)

# ls-qpack
QPACK compression library for use with HTTP/QUIC

## Introduction

QPACK is a new compression mechanism for use with IETF QUIC.  It is
in the process of being standardazed by the QUIC Working Group.  This
library is meant to aid in the development of the QUIC protocol.  At
the same time, we aim to make it production quality from the start, as
we will use it in our own software products.

## API

The API is documented in the header file, [include/lsqpack.h](include/lsqpack.h).
The interface is designed with the stream APIs in
[lsquic-client](https://github.com/litespeedtech/lsquic-client) in mind.  We want
to achieve zero-copy when possible.
