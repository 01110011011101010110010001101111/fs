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

## Engage Links Reference

* https://engaging-web.mit.edu/eofe-wiki/logging_in/ssh/
* https://github.com/MIT-6-106/engaging/blob/main/tester/run.py
* https://engaging-ood.mit.edu/pun/sys/dashboard

