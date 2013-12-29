cd /home/mathieu/Dropbox/work/decode_captcha/cari \
    && ./generate_all.sh \
    && cd /home/mathieu/Dropbox/work/prout/out/production/prout \
    && java captcha.knn.KnnClassifierTest /home/mathieu/Dropbox/work/decode_captcha/cari/knn_train.txt /home/mathieu/Dropbox/work/decode_captcha/cari/knn_test.txt $1 $2 true
