# This scenario is designed to reach the 'case HUFF_DEC_END_DST'
# in parse_header_data() when Huffman encoding is turned on.
TABLE_SIZE=256
AGGRESSIVE=1
RISKED_STREAMS=1
QIF=$(cat<<'EOQ'
aaaa	aaa
aaaa	aaaaa
aaaa	aaaaaaaa
aaaa	aaaaaaaaaaaaa
aaaa	aaaaaaaaaaaaaaaaaaaaa

EOQ
)
