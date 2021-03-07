local.install:
		sudo apt-get update || true
		sudo apt-get -y install build-essential || true
		sudo apt-get -y install cmake || true
		sudo apt-get -y install libcurl4-openssl-dev || true
		sudo apt-get -y install libedit-dev || true

local.build:
		cmake src/. || true
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
		docker rmi connerylang

docker.run:
		docker run -it connerylang