# CS623 NVBit assignment

This repository contains the CS623 assignment tools, test programs, inputs, and results. Large input files and trace logs are stored as gzip archives through Git LFS. The archives decompress to the original bytes and filenames.

The NVIDIA NVBit 1.8 release is downloaded from [NVIDIA's official release](https://github.com/NVlabs/NVBit/releases/tag/v1.8). The `assignment_tools/tools/` directory contains the added tools and the three modified `mem_trace` source files. Copying these files over the official release restores the assignment tool tree. The locally downloaded NVBit library and headers are ignored by Git.

## Clone and restore data

Install [Git LFS](https://git-lfs.com/) before cloning, or run `git lfs pull` in an existing clone. On Linux, restore the original input and trace paths with:

```sh
for archive in CS623_assignment1/file*.gz CS623_assignment1/results/mem_file*.log.gz; do
  gzip -dk "$archive"
done
```

The small result summaries under `CS623_assignment1/results/` are plain text. The `pc_file*.log.gz` traces are already compressed and can be read with `gzip -dc`.

## Restore NVBit tools

On Linux x86_64, download NVBit 1.8 and apply the assignment files:

```sh
curl -fL https://github.com/NVlabs/NVBit/releases/download/v1.8/nvbit-Linux-x86_64-1.8.tar.bz2 -o nvbit-Linux-x86_64-1.8.tar.bz2
echo '72a2b827f9531dcb86b6be13844f267640fb440929d92944177029da6da2b9e1  nvbit-Linux-x86_64-1.8.tar.bz2' | sha256sum -c -
tar -xjf nvbit-Linux-x86_64-1.8.tar.bz2
cp -a assignment_tools/tools/. nvbit_release_x86_64/tools/
```

The archive checksum was verified against the official NVBit 1.8 x86_64 release. NVBit tools require the Linux, CUDA, and GPU environment described in its README. The assignment's `p2.cpp`, `p3a.cpp`, and `p3b.cpp` sources are under `assignment_tools/tools/`.
