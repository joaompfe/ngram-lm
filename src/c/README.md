## Requirements

- Linux platform
- CMake

## Build

To generate the Makefiles type the following at project root:

`cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_DEPENDS_USE_COMPILER=FALSE -B build/cmake/release .`

3 targets will be available:

- `ngram_lm` - shared library
- `ngram_lm_static` - static library
- `build` - build executable to build tries from ARPA files

Build or install the desired ones.

## Usage

### Executables

Type `build -n=X ARPA_FILE TRIE_OUT_FILE` to build an X-gram trie from the
`ARPA_FILE` file, saving the result in the file `TRIE_OUT_FILE`.

Type `build --help` for extra information.

### Library

Currently, the library main entry point can be found in `trie.h`.

To create a trie from `ARPA_FILE` file do:

```c
#include "arpa.h"
#include "trie.h"
#include "word.h"

#define ARPA_FILE "/path/to/file.arpa"

struct arpa *arpa = arpa_open(ARPA_FILE);
int order = 3;
struct trie *t = trie_new_from_arpa(order, arpa);
```

Then, to get the next word prediction given some context do:

```c
const char *context[] = { "This", "is" };
int context_length = 2;
struct word *prediction = trie_get_nwp(t, context, context_length);
printf("The next word prediction for '%s %s' is '%s'\n", context[0], context[1],
word_get_text(prediction));
```

Finally, close the arpa file and free the memory taken by the trie:

```c
arpa_close(arpa);
trie_delete(t);
```
