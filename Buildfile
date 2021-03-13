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
		./Connery || cmake src/. && make Connery && ./Connery || sudo apt-get -y install build-essential; sudo apt-get -y install cmake; sudo apt-get -y install libcurl4-openssl-dev; sudo apt-get -y install libedit-dev; cmake src/. && make Connery && ./Connery || echo "There is nothing like a challenge to bring out the best in man."

docker.build:
		docker build --tag connerylang .

docker.clean:
		docker rmi connerylang --force

docker.run:
		docker run -it connerylang || docker build --tag connerylang . && docker run -it connerylang