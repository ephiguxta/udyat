test -c /dev/ttyUSB0 && {
	mkdir udyat
	cp udyat.ino udyat/
	arduino-cli compile --fqbn esp32:esp32:esp32 udyat/ && \
	arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 udyat/
	rm -rf udyat
} || {
	echo "Verifique o cabo USB"
}
