# Provenance Tracking with Clam # 

## Description ## 

Propagate tags (numerical identifiers) from many sources to many
sinks.

### How ### 

User defines sources and sinks (memory locations) via a
configuration file (option `-add-metadata-config`):

```
read, 2, clam-prov-type:input
read, 2, clam-prov-size:3
write, 2, clam-prov-type:output
write, 2, clam-prov-size:3
```

This states that the `clam-prov` type of the second parameter of any 
call to `read` is an input (i.e., _source_) and the `clam-prov` type of
the second parameter of any call to `write` is an output (i.e., _sink_).
Also, it states that the size of the second parameter of any call to `read`
is specified by the third parameter and the size of the second parameter
of any call to `write` is specified by the third parameter. Note that, in 
general, a program will have _many sources_ and _many sinks_ since it can 
have many calls to `read` and `write`.

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
!8 = !{!"2", !"clam-prov-type:output", !"clam-prov-size:3"}
!9 = !{i64 2}

```

This says that the second parameter of call to `write` (identifier
`4`) is tagged with tags `{2}`.

  

## Requirements ##

- Install [cmake](https://cmake.org/) >= 3.13 
- Install [llvm 11](https://releases.llvm.org/download.html)
- Install [gmp](https://gmplib.org/)
- Install [boost](https://www.boost.org/) >= 1.65

### To run tests ###

- `lit`: `sudo pip install lit`
- `cmp` utility

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

## Checking installation ## 

To run some regression tests:

     cmake --build . --target test-all

## Usage ##

     clam-prov.py  test.c -o test.prov.bc
     
By default, `clam-prov.py` uses the file `config/sources-sinks.config`
from the install directory to know which are the sources and sinks.
The output bitcode file `test.prov.bc` contains `call-site-metadata`
metadata indicating the tags assigned to both sources (e.g., read
calls) and sinks (e.g., write calls).

If we want to use a different set of sources and sinks then use the
command:

     clam-prov.py  test.c --add-metadata-config=addMetadata.config -o test.prov.bc
     

## Output Propagated Tags ##

Alternatively, the same information provided by LLVM metadata
`call-site-metadata` can be printed to a file in [DOT](https://graphviz.org/doc/info/lang.html) format. The tags
propagated to sinks from sources can be outputted using the argument
`dependency-map-file` as follows:

     clam-prov.py test.c --add-metadata-config=addMetadata.config --dependency-map-file=dependency_map.dot

Following is an example output file `dependency_map.dot`:

```
digraph clam_prov_dependency_map{
"0" [label="function name:read\ncall site:0"];
"1" [label="function name:read\ncall site:1"];
"2" [label="function name:write\ncall site:2"];
"2" -> "0" [label="WasDependentOn"];
"2" -> "1" [label="WasDependentOn"];
}
```

The output above says the following:
* First call-site `read` has the call site tag `0`
* Second call-site `read` has the call site tag `1`
* Third call-site `write` has the call site tag `2`
* The third call-site (`write`) has the propagated call site tags `0`, and `1`.

## Log call-sites (Linux) ##

Logging can be added to a Linux program to emit call-sites using the argument `add-logging-config` as follows:

     clang -Xclang -disable-O0-optnone -c -emit-llvm test.c -o test.bc
     clam-pp --crab-devirt test.bc -o test.pp.bc
     clam-prov test.pp.bc --add-metadata-config=addMetadata.config --add-logging-config=call-site-logging.config -o test.out.pp.bc

The above specifies the file `call-site-logging.config` to configure how to log the call-sites when program is executed. The configurations must have the keys:
* `output_mode` - Whether to write to a file (at `~/.clam-prov/audit.log`) or to a pipe (at `~/.clam-prov/audit.pipe`). Specify `0` to write to the file, or specify `1` to write to the pipe
* `max_records` - The maximum call-site records to buffer before writing to the file or the pipe

The output is written in binary format as the `clam-prov-record` [struct](https://github.com/SRI-CSL/clam-prov/blob/master/src/Logging/clam-prov-logger.h#L32).

To be able to generate an executable to log call-sites from `test.out.pp.bc` (above), the shared library must be linked as follows:

```
llc -relocation-model=pic test.out.pp.bc -o test.out.pp.s 
gcc -L./install/lib test.out.pp.s -o test.out.pp.native -lclamprovlogger
```

Finally, the generated executable can be executed as:

```
LD_LIBRARY_PATH=./install/lib ./test.out.pp.native
```
