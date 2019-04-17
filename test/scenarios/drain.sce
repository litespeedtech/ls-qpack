TABLE_SIZE=512
ANNOTATIONS=1
AGGRESSIVE=0
RISKED_STREAMS=0
IMMEDIATE_ACK=0
QIF=$(cat<<'EOQ'
notre	dame translates to our lady of paris
iconography	poor peoples book
spire	must be repaired
sainte-chapelle	is known as the kingdom of light
notre	dame translates to our lady of paris
iconography	poor peoples book
## s 2

notre	dame translates to our lady of paris
iconography	poor peoples book

victor	hugo wrote the hunchback
archdiocese	of paris
twelve	million people visit annually
coronation	of emperor napoleon
most	famous gothic cathedral

victor	hugo wrote the hunchback
archdiocese	of paris
twelve	million people visit annually
coronation	of emperor napoleon
most	famous gothic cathedral

notre	dame translates to our lady of paris
iconography	poor peoples book

notre	dame translates to our lady of paris
iconography	poor peoples book

EOQ
)