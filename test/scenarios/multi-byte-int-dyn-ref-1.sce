# This scenario is designed to reach the `case EEA_INS_NAMEREF_DYNAMIC`
# in the `lsqpack_enc_encode` function. It is intended to fail the integer
# encoding due to buffer length. To trigger this case, many names must be
# inserted into the dynamic table so that the integer encoding requires
# more than one byte.
TABLE_SIZE=16384
AGGRESSIVE=1
IMMEDIATE_ACK=0
RISKED_STREAMS=1000
QIF=$(cat<<'EOQ'
aaaaaaaaaaaaa	aaaaaaaaaaaaaaaaaaaaaaaaaa
bbbbbbbbbbbbb	bbbbbbbbbbbbbbbbbbbbbbbbbb
ccccccccccccc	cccccccccccccccccccccccccc
ddddddddddddd	dddddddddddddddddddddddddd
eeeeeeeeeeeee	eeeeeeeeeeeeeeeeeeeeeeeeee
fffffffffffff	ffffffffffffffffffffffffff
ggggggggggggg	gggggggggggggggggggggggggg
aaaa	oneeeeeeeee
bbbb	oneeeeeeeee
cccc	oneeeeeeeee
dddd	oneeeeeeeee
eeee	oneeeeeeeee
ffff	oneeeeeeeee
gggg	oneeeeeeeee
hhhh	oneeeeeeeee
iiii	oneeeeeeeee
jjjj	oneeeeeeeee
kkkk	oneeeeeeeee
llll	oneeeeeeeee
mmmm	oneeeeeeeee
nnnn	oneeeeeeeee
oooo	oneeeeeeeee
pppp	oneeeeeeeee
qqqq	oneeeeeeeee
rrrr	oneeeeeeeee
ssss	oneeeeeeeee
tttt	oneeeeeeeee
uuuu	oneeeeeeeee
vvvv	oneeeeeeeee
wwww	oneeeeeeeee
xxxx	oneeeeeeeee
yyyy	oneeeeeeeee
zzzz	oneeeeeeeee
aabb	oneeeeeeeee
bbcc	oneeeeeeeee
ccdd	oneeeeeeeee
ddee	oneeeeeeeee
eeff	oneeeeeeeee
ffgg	oneeeeeeeee
gghh	oneeeeeeeee
hhii	oneeeeeeeee
iijj	oneeeeeeeee
jjkk	oneeeeeeeee
kkll	oneeeeeeeee
llmm	oneeeeeeeee
mmnn	oneeeeeeeee
nnoo	oneeeeeeeee
oopp	oneeeeeeeee
ppqq	oneeeeeeeee
qqrr	oneeeeeeeee
rrss	oneeeeeeeee
sstt	oneeeeeeeee
ttuu	oneeeeeeeee
uuvv	oneeeeeeeee
vvww	oneeeeeeeee
wwxx	oneeeeeeeee
xxyy	oneeeeeeeee
yyzz	oneeeeeeeee

bbbb	two
cccc	two
dddd	two
eeee	two
ffff	two
gggg	two
hhhh	two
yyyy	two
zzzz	two
aabb	two
bbcc	two
ccdd	two
ddee	two
eeff	two
ffgg	two
gghh	two
hhii	two
iijj	two
jjkk	two
kkll	two
llmm	two
mmnn	two
nnoo	two
oopp	two
ppqq	two
qqrr	two
rrss	two
sstt	two
ttuu	two
uuvv	two
vvww	two
wwxx	two
xxyy	two
yyzz	two
aaaa	two

aaaa	three
bbbb	three

EOQ
)
