## Tests Overview ##

This directory contains tests for verifying tag propagation.

Each test directory name is of the format `test<number>`, and contains the following:

* `test.c`: The test program and the description of the test
* `AddMetadata.config`: The input to the `AddMetadata` pass to add metadata to call-sites
* `DependencyMap.output.expected`: The expected output of the `OutputDependencyMap` pass

## Execute Test ##

Each test can be executed using the following commands:
```
clang -Xclang -disable-O0-optnone -c -emit-llvm test.c -o test.bc
clam-pp --crab-devirt test.bc -o test.pp.bc
# Generate the dependency map file that should match the included file 'DependencyMap.output.expected'
clam-prov test.pp.bc --add-metadata-config=AddMetadata.config --dependency-map-file=DependencyMap.output.actual -o test.out.pp.bc
# Compare the expected and actual dependency map output 
cmp --silent DependencyMap.output.actual DependencyMap.output.expected && echo "Match!" || echo "Mismatch!"
```

