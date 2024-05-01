all:
	$(CXX) -DWLR_USE_UNSTABLE -shared -fPIC --no-gnu-unique main.cpp monocleLayout.cpp -o monocleLayoutPlugin.so -g `pkg-config --cflags pixman-1 libdrm hyprland` -std=c++2b
clean:
	rm ./monocleLayoutPlugin.so
