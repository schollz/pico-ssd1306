do: build
	cd build && make -j32

build:
	mkdir -p build
	cd build && cmake ..

clean:
	rm -rf build