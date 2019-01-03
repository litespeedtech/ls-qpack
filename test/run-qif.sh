#!/bin/bash
#
# Script to run a QIF file.  The following environment variables are expected
# to be set:
#
#   AGGRESSIVE
#   TABLE_SIZE
#   IMMEDIATE_ACK
#   RISKED_STREAMS

set -x
set -e

for NEED in interop-encode interop-decode sort-qif.pl; do
    type $NEED
    if [ $? -ne 0 ]; then
        echo $NEED is not in $PATH 1>&2
        exit 1
    fi
done

function cleanup {
    rm -vf $QIF_FILE
    rm -vf $BIN_FILE
    rm -vf $RESULTING_QIF_FILE
    rm -vf $ENCODE_LOG
    rmdir $DIR
}

if [ -z "$DO_CLEANUP" -o "$DO_CLEANUP" != 0 ]; then
    trap cleanup EXIT
fi

DIR=/tmp/run-qif-out-$$$RANDOM
ENCODE_LOG=$DIR/encode.log
QIF_FILE=$DIR/qif
BIN_FILE=$DIR/out
RESULTING_QIF_FILE=$DIR/qif-result
mkdir -p $DIR

if [ "$AGGRESSIVE" = 1 ]; then
    ENCODE_ARGS="$ENCODE_ARGS -A"
fi

if [ "$IMMEDIATE_ACK" = 1 ]; then
    ENCODE_ARGS="$ENCODE_ARGS -a 1"
fi

if [ -n "$RISKED_STREAMS" ]; then
    ENCODE_ARGS="$ENCODE_ARGS -s $RISKED_STREAMS"
    DECODE_ARGS="$DECODE_ARGS -s $RISKED_STREAMS"
fi

if [ -n "$TABLE_SIZE" ]; then
    ENCODE_ARGS="$ENCODE_ARGS -t $TABLE_SIZE"
    DECODE_ARGS="$DECODE_ARGS -t $TABLE_SIZE"
fi

cp -v $1 $QIF_FILE

interop-encode $ENCODE_ARGS -i $QIF_FILE -o $BIN_FILE 2>$ENCODE_LOG
interop-decode $DECODE_ARGS -i $BIN_FILE -o $RESULTING_QIF_FILE
diff <(sort-qif.pl --strip $QIF_FILE) \
                <(sort-qif.pl --strip $RESULTING_QIF_FILE)
