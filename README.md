# Provenance Tracking with Clam # 

## Description ## 

Propagate tags (numerical identifiers) from many sources to many
sinks.

### How ### 

User defines sources and sinks (memory locations) via a
configuration file (option `-add-metadata-config`):

```
read, 2, input
write, 2, output
```

This states that the second parameter of any call to `read` is an
input (i.e., _source_) and the second parameter of any call to `write`
is an output (i.e., _sink_).  Note that, in general, a program will
have _many sources_ and _many sinks_ since it can have many calls to
`read` and `write`.

The analysis then assigns a unique numerical identifier (i.e., tags)
to each source and it relies on
the [Clam static analyzer](https://github.com/seahorn/clam) to
propagate those tags across memory and function boundaries. The output
of the analysis maps each sink to a set with all possible
tags. With this, each sink is connected back to a subset of
sources. Currently, this output is encoded as metadata named
`clam-prov-tags`. For instance:

```
%res = call i64 @write(i32 %param, i8* %param1, i64 %param2), !call-site-metadata !7, !clam-prov-tags !9
...
!7 = !{!"4", !8}
!8 = !{!"2", !"output"}
!9 = !{i64 2}

```

This says that the second parameter of call to `write` (identifier
`4`) is tagged with tags `{2}`.

  

## Requirements ##

- Install [cmake](https://cmake.org/) >= 3.13 
- Install [llvm 11](https://releases.llvm.org/download.html)
- Install [gmp](https://gmplib.org/)
- Install [boost](https://www.boost.org/) >= 1.65

## Compilation and Installation ##

```
mkdir build && cd build
cmake ..
cmake --build . --target clam-seadsa && cmake ..
cmake --build . --target clam-seallvm && cmake ..
cmake --build . --target clam-src && cmake ..
cmake --build . --target crab && cmake ..
cmake --build . --target install
```

## Usage ##

```
clang -Xclang -disable-O0-optnone -c -emit-llvm test1.c -o test1.bc
clam-pp --crab-devirt test1.bc -o test1.pp.bc
clam-prov test1.pp.bc --add-metadata-config=addMetadata.config -o test1.out.pp.bc

```

