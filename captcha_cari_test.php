<?php
$train_file = (dirname(__FILE__) . "/knn_multiple.net");
if (!is_file($train_file))
    die("The file knn_multiple.net has not been created! Please run simple_train.php to generate it");

$ann = fann_create_from_file($train_file);
if (!$ann)
    die("ANN could not be created");

while (($line = fgets(STDIN)) !== false) {
    $inputStrings = explode(' ', $line);
    $inputIntegers = array();
    foreach ($inputStrings as $input) {
            if(preg_match('/[0-9]/', $input)) {
                array_push($inputIntegers, $input);
            }
    }
    if(count($inputIntegers) > 3) {
        $calc_out = fann_run($ann, $inputIntegers);
        $guess = array_keys($calc_out, max($calc_out))[0];
        print(chr($guess+ord('0')));
    }
}

fann_destroy($ann);
