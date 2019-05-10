# This scenario is designed to reach the 'case HUFF_DEC_END_DST'
# in lsqpack_dec_enc_in().
TABLE_SIZE=256
AGGRESSIVE=0
RISKED_STREAMS=0
QIF=$(cat<<'EOQ'
aaaaaaaaaaaaaaaaaaaaaaaaa	aaaa
bbbbbbbbbbbbbbbbbbbbbbbbb	bbbb

aaaaaaaaaaaaaaaaaaaaaaaaa	aaaa
bbbbbbbbbbbbbbbbbbbbbbbbb	bbbb

EOQ
)