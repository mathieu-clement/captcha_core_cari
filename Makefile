CC=clang
CFLAGS=-Wall -std=c99 -g 
LDFLAGS=-lm

all: remove_noise segmenter	

lib_captcha_common:
	$(CC) -o captcha_common.o \
		$(CFLAGS) `pkg-config --cflags MagickCore` \
		-fPIC -c captcha_common.c
	#$(CC) -shared -o libcaptcha_common.so captcha_common.o
	#ar rcs libcaptcha_common.a captcha_common.o

remove_noise: lib_captcha_common
	$(CC) -o remove_noise \
		$(CFLAGS) `pkg-config --cflags MagickCore` \
		remove_noise.c captcha_common.o \
		$(LDFLAGS) `pkg-config --libs MagickCore`


segmenter: lib_captcha_common
	$(CC) -o segmenter $(CFLAGS) segmenter.c captcha_common.o $(LDFLAGS)

clean:
	rm -f remove_noise segmenter segmenter_pixels libcaptcha_common.so libcaptcha_common.a
