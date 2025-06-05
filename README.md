# esme-linux

## Requirements
- windows
- WSL2
- docker desktop
- 200 GB storage

## Do everything in WSL terminal from now on
### Set git sername and e-mail
```{.sh}
  git config --global user.name "Your Name"
  git config --global user.email "your.email@example.com"
```
### Set docker to run without sudo
To work better with docker, without `sudo`, add your user to `docker group`.
  ```{.sh}
  sudo usermod -aG docker $(whoami)
  ```
### clone this repo
  ```{.sh}
  git clone https://github.com/acrontech-solutions/esme-linux.git
  ```
### Build docker image
  ```{.sh}
  docker build --tag imx-yocto --build-arg DOCKER_WORKDIR=$(pwd) --build-arg USER=$(whoami) --build-arg host_uid=$(id -u) --build-arg host_gid=$(id -g) -f Dockerfile-Ubuntu-22.04 .
  ```
### Run docker image
This start a docker container in interactive mode, from now on, we will work on this container.
  ```{.sh}
  docker run -it --rm --volume $(pwd):/home/$(whoami) imx-yocto
  ```
### checkout and configure yocto project files
This will use the repo tool to get source files from different repositories:
  ```{.sh}
  repo init -u https://github.com/acrontech-solutions/esme-linux.git -b main -m imx-6.6.52-2.2.0.xml
  repo sync -j`nproc`
  ```
This will configure the yocto project for our imx93 device, and other settings:
  ```{.sh}
EULA=1 MACHINE=imx93-11x11-lpddr4x-evk DISTRO=fsl-imx-xwayland source imx-setup-release.sh -b build_fsl-imx-xwayland
cd ..
  ```
This will add the layer from acrontech which includes recipes for modifying device tree, kernel modules, images...
```{.sh}
bitbake-layers add-layer sources/meta-acrontech
```
### Compile an image
  ```{.sh}
bitbake acrontech-image
  ```
`target_image` can be :
| target_image  | description |
| ------------- | ------------- |
| imx-image-core  | An i.MX image with i.MX test applications to be used for Wayland backends. This image is used by our daily core testing.   |
| acrontech-image  | Image created by acrontech to develop solution for accessing lepton flir camera images realtime, and process them on linux. |

Compilation results are stored in this directory:
  ```{.sh}
build_fsl-imx-xwayland/tmp/deploy/images/imx93-11x11-lpddr4x-evk/
```
### Program an image
We use `uuu.exe` under windows to program the whole image to emmc storage:
in windows terminal, or power shell:
```{.sh}
./uuu.exe -b emmc_all acrontech-image-imx93-11x11-lpddr4x-evk.rootfs.wic.zst
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

## make changes on device tree only, and change in running linux
- export kernel
- make changes
- compile kernel
  ```{.sh}
  devtool modify linux-imx
  ...
  devtool build linux-imx  
  ```
- locate the compile .dtb file, copy to an SDCARD
- insert SDCARD to evk board
- boot linux
- mount sdcard and emmc, copy dtb to emmc
- reboot evk
- stop uboot
```{.sh}
editenv fdtfile
```
enter: imx93-11x11-evk-lepton.dtb

If you want to make the changes stick:
```{.sh}
saveenv
boot
```

### Other useful commands
list yocto layers:
  ```{.sh}
  bitbake-layers show-layers 
  ```

generate SDK for linux application development:
```{.sh}
bitbake acrontech-image -c populate_sdk
```
will generate a .sh script, which can be installed on a linux dev machine:
```{.sh}
build_fsl-imx-xwayland/tmp/deploy/sdk/fsl-imx-xwayland-glibc-x86_64-acrontech-image-armv8a-imx93-11x11-lpddr4x-evk-toolchain-6.6-scarthgap.sh
```


