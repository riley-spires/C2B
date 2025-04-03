# Quoopie Build System (QBS)

> [!CAUTION]
> This project is in active development.
> It may not support all the features you need yet
> and it may never!


## Getting Started

Copy `qbs.h` into your project

The simplest use case is:
```c++
#include "qbs.h"

int main() {
    qbs::Build build("main");

    build.append_src_dir("src");

    return build.build();
}
```

Then bootstrap the build with your favorite c++ compiler
```console
g++ qbs.cpp -o qbs
./qbs
```

For more advanced use cases see the [examples](./examples)

## Dependencies

### Required

- `clang` or `g++`

### Optional (Required for full functionality)

- `wget` or `curl`

## Credits

- Inspired by [nob.h](https://github.com/tsoding/nob.h).
