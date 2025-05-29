# esme-linux

## Requirements
- windows
- WSL2
- docker desktop
- 200 GB storage

## Do everything in WSL terminal from now on
### Set git sername and e-mail
```{.sh}
  $ git config --global user.name "Your Name"
  $ git config --global user.email "your.email@example.com"
```

### Set docker to run without sudo
To work better with docker, without `sudo`, add your user to `docker group`.
  ```{.sh}
  $ sudo usermod -aG docker <your_user>
  ```

### clone this repo
  ```{.sh}
  $ git clone https://github.com/acrontech-solutions/esme-linux.git
  ```

### Build docker image
  ```{.sh}
  $ docker build --tag imx-yocto --build-arg DOCKER_WORKDIR=$(pwd) --build-arg USER=$(whoami) --build-arg host_uid=$(id -u) --build-arg host_gid=$(id -g) -f Dockerfile-Ubuntu-22.04 .
  ```

### Run docker image
  ```{.sh}
  docker run -it --rm --volume $(pwd):/home/$(whoami) imx-yocto
  ```
From now on, use the terminal in the docker container:
### checout and compile
  ```{.sh}
  repo init -u https://github.com/acrontech-solutions/esme-linux.git -b main -m imx-6.6.52-2.2.0.xml
  repo sync
  ```

## Make changes in the kernel
  ```{.sh}
  devtool modify linux-imx
  ```
It will create a directory with the kernel sources
Make modifications here, build it with:
  ```{.sh}
  devtool build linux-imx
  ```
commit changes, and create patch file:
  ```{.sh}
  git add .
  git commit -m "my linux modifications"
  git format-patch -1
  ```
copy patch file to the meta-acrontech directory and git add/commit/push

### Useful commands
list yocto layers:
  ```{.sh}
  bitbake-layers show-layers 
  ```


