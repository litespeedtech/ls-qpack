# This scenario is designed to reach 'case DATA_STATE_LFPBNR_READ_VAL_PLAIN'
# in parse_header_data(). It has a number of repeated names with values that
# would be too small for Huffman coding.
TABLE_SIZE=256
AGGRESSIVE=1
RISKED_STREAMS=1
QIF=$(cat<<'EOQ'
dude	n
dude	nu
dude	where is my car?

EOQ
)
