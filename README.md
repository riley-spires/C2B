# C++ to Build(C2B)

> [!CAUTION]
> This project is in active development.
> It may not support all the features you need yet
> and it may never!


## Getting Started

Copy `c2b.h` into your project

The simplest use case is:
```c++
// c2b.cpp
#include "c2b.h"

int main() {
    c2b::Build build("main");

    build.append_src_dir("src");

    return build.build();
}
```

Then bootstrap the build with your favorite c++ compiler
```console
g++ c2b.cpp -o c2b
./c2b
```

For more advanced use cases see the [examples](./examples)

## Dependencies

### Required

- `clang` or `g++`

### Optional (Required for full functionality)

- `wget` or `curl`

## Credits

- Inspired by [nob.h](https://github.com/tsoding/nob.h).
