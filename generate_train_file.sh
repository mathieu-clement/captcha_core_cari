#!/bin/sh

for f in `dirname $0`/samples/segmented/output/*_[123]*
do
    bname=`basename $f`
    export SAMPLE=$bname
#    echo $bname
    ./samples/complete_run.sh /home/mathieu/Dropbox/work/decode_captcha/cari/knn_train.txt
done
