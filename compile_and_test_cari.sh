touch knn_train.txt
touch knn_test.txt
./generate_all.sh \
    && java -cp ../captcha_classifier/out/production/captcha_classifier captcha.knn.KnnClassifierTest knn_train.txt knn_test.txt 1 31 true
