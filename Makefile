all: stb_image.h stb_image_write.h
	$(CXX) -O3 -g -std=gnu++26 -fopenmp gray_to_normal.cpp -o gtn

stb_image.h:
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h
	
stb_image_write.h:
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h

install:
	sudo cp ./gtn /usr/local/bin/

uninstall:
	sudo rm /usr/local/bin/gtn
