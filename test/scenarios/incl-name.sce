# This scenario is designed to reach 'case DATA_STATE_READ_LFONR_NAME_PLAIN'
# in parse_header_data(). It contains header names that are be too small
# for Huffman coding.
TABLE_SIZE=256
AGGRESSIVE=1
RISKED_STREAMS=0
QIF=$(cat<<'EOQ'
yo	I
no	do not
se	know

EOQ
)
