###############################################
#   RASPBERRY PI CAMERA + TFT PROJECT MAKEFILE
###############################################

# Compilers
CC      = gcc
CXX     = g++
CFLAGS  = -Wall
CXXFLAGS = -Wall -std=gnu++20

# Include & libraries
INCLUDES = -I/usr/local/include
LIBS     = -L/usr/local/lib -lrpidisplaygl -llgpio -lrt

# Camera C sources
C_SRC = arduchip.c ov2640.c spi_linux.c i2c_linux.c jpeg_to_rgb565.c picojpeg.c
C_OBJ = $(C_SRC:.c=.o)

###############################################
# DEFAULT TARGET
###############################################
all: help

###############################################
# HELP MENU
###############################################
help:
	@echo "------------------------------------------------------"
	@echo "                 PROJECT BUILD HELP"
	@echo "------------------------------------------------------"
	@echo "CLEAN:"
	@echo "  make clean                     → Tüm .o ve derlemeleri sil"
	@echo
	@echo "TEST DERLEME:"
	@echo "  make test_arduchip             → ArduChip SPI test programını derle"
	@echo "  make test_hal                  → OV2640 I2C init testini derle"
	@echo
	@echo "TEST ÇALIŞTIRMA:"
	@echo "  make run_test_arduchip         → (sudo) ArduChip testini çalıştır"
	@echo "  make run_test_hal              → (sudo) OV2640 testini çalıştır"
	@echo
	@echo "KAMERA UYGULAMALARI DERLEME:"
	@echo "  make capture_app               → JPEG çekme uygulamasını derle"
	@echo "  make decode_test               → JPEG→RGB565 decode testini derle"
	@echo "  make capture_screen            → Kameradan alıp TFT'de gösteren uygulamayı derle"
	@echo
	@echo "UYGULAMA ÇALIŞTIRMA:"
	@echo "  make run_capture_app           → (sudo) capture_app'i çalıştır"
	@echo "  make run_decode_test          → (sudo) rgb565 dosyayı kaydeder"
	@echo "  make run_capture_screen        → (sudo) capture_screen'i çalıştır"
	@echo "------------------------------------------------------"


###############################################
# COMPILE C SOURCES
###############################################
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

###############################################
# TEST PROGRAMS (DERLEME)
###############################################

test_arduchip: test_arduchip.c arduchip.o spi_linux.o
	$(CC) test_arduchip.c arduchip.o spi_linux.o -o test_arduchip $(CFLAGS)

test_hal: test_hal.c ov2640.o spi_linux.o i2c_linux.o arduchip.o
	$(CC) test_hal.c ov2640.o spi_linux.o i2c_linux.o arduchip.o -o test_hal $(CFLAGS)

###############################################
# TEST PROGRAMS (ÇALIŞTIRMA)
###############################################
run_test_arduchip: test_arduchip
	sudo ./test_arduchip

run_test_hal: test_hal
	sudo ./test_hal

###############################################
# CAMERA CAPTURE PROGRAMS
###############################################

capture_app: capture_to_file.c $(C_OBJ)
	$(CC) capture_to_file.c $(C_OBJ) -o capture_app $(CFLAGS)

decode_test: decode_test.c jpeg_to_rgb565.o picojpeg.o
	$(CC) decode_test.c jpeg_to_rgb565.o picojpeg.o -o decode_test $(CFLAGS)

###############################################
# FULL PIPELINE → CAMERA + TFT
###############################################

capture_screen: capture_to_screen.cpp $(C_OBJ)
	$(CXX) capture_to_screen.cpp $(C_OBJ) -o capture_screen $(CXXFLAGS) $(INCLUDES) $(LIBS)

###############################################
# RUN TARGETS
###############################################

run_capture_app: capture_app
	sudo ./capture_app

run_decode_test: decode_test
	sudo ./decode_test

run_capture_screen: capture_screen
	sudo ./capture_screen

###############################################
# CLEAN
###############################################
clean:
	rm -f *.o test_arduchip test_hal capture_app decode_test capture_screen frame.jpg frame.rgb565
	@echo "Temizlik tamamlandı." 