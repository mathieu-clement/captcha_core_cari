#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename;
use Data::Dumper;

my $cari_PATH='/home/mathieu/git_repositories/captcha/captcha_core_cari';

my $input_image=$ARGV[0];
my $input_image_basename=basename $input_image;
# Remember $$ stands for the Process ID (PID)
my $remove_noise_output_file="/tmp/" . $$ . "_" . $input_image_basename . "_rn.txt";
my $segmenter_output_file="/tmp/" . $$ . "_" . $input_image_basename . "_sg.txt";

# Noise Removal and Binarization
`$cari_PATH/remove_noise "$input_image" "$remove_noise_output_file"`;

# Segmentation and Feature Extraction
`$cari_PATH/segmenter "$remove_noise_output_file" > "$segmenter_output_file"`;

# Parse the feature file
open(my $fh, "<", $segmenter_output_file) or die "Could not open $segmenter_output_file";

my @features = ();
my @reading_order;
while (<$fh>) {
    push @features, $1 if (/CODED FEATURES (.*)/);
    @reading_order = ($_ =~ /READING ORDER (\d) ?(\d)? ?(\d)? ?(\d)? ?(\d)? ?(\d)?/);
}
close $fh;

# Remove useless space in front of group
@reading_order = map { /(\d+)/ and int($1) } @reading_order;

my @truc = ($features[0] =~ / */g);
my @truc2 = grep(/ /, @truc);
my $nb_features = scalar(@truc2);
my $in = "";

foreach(@reading_order) {
    $in .= $features[$_-1];
    $in .= "\n";
}

#my $out = `echo "$in" | java -cp $KNN_CLASSPATH captcha.knn.KnnClassifier 1 $cari_PATH/knn_train.txt $nb_features 2>/dev/null`;
my $out = `echo "$in" | php $cari_PATH/captcha_cari_test.php`;
print $out;
print "\n";

unlink $remove_noise_output_file;
unlink $segmenter_output_file;
