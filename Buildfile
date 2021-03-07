local.install:
		sudo apt-get -y install build-essential
		sudo apt-get -y install cmake
		sudo apt-get -y install libcurl4-openssl-dev
		sudo apt-get -y install libedit-dev

local.build:
		cmake src/.
		make Connery

local.clean:
		make clean || true
		rm cmake_install.cmake || true
		rm CMakeCache.txt || true
		rm -r CMakeFiles || true
		rm -r stdlib || true
		rm Makefile || true

local.run:
		./Connery

docker.build:
		docker build --tag connerylang .

docker.clean:
		docker rmi connerylang --force

docker.run:
		docker run -it connerylang