# FS

```
mkdir build && cd build
cmake ..
make
```

# Cluster setup
`module load cmake/3.27.9`
`module load gcc/12.2.0`
`module load apptainer/1.1.9`
`python3 -m pip install --user --upgrade meson ninja`
`git clone https://github.com/libfuse/libfuse.git && cd `
`meson setup build --prefix=$HOME/.local`
`meson compile -C build`
`meson install -C build`
`export PKG_CONFIG_PATH="$HOME/.local/lib64/pkgconfig:$PKG_CONFIG_PATH"`
`export LD_LIBRARY_PATH="$HOME/.local/lib64:$LD_LIBRARY_PATH`

# RDMA_FS instructions
Tested on ubuntu
Clone grpc and build from source; install to a local folder and set `export CMAKE_PREFIX_PATH="$HOME/workspace/local_grpc:$CMAKE_PREFIX_PATH`
Make sure can run the examples in grpc
Install fuse3: `sudo apt install -y libfuse3-dev`
Use `src/CMakeLists.txt`
To run, do `./rdmafs backup_folder_name mount_point`

# Running our FUSE
`/home/alexhu/workspace/fs/src/rdmafs`
`df -T .`
`/home/alexhu/orcd/c7/pool`

# rdma-example
`git clone https://github.com/animeshtrivedi/rdma-example.git`
`cmake .`
`make`
`ip -o -4 addr show ib0`
get the address from above
`./bin/rdma_server`
`./bin/rdma_client -a <address> -s "yo"`

## Engage Links Reference

* https://engaging-web.mit.edu/eofe-wiki/logging_in/ssh/
* https://github.com/MIT-6-106/engaging/blob/main/tester/run.py
* https://engaging-ood.mit.edu/pun/sys/dashboard

