## Docs

### Installation
The library is available on the PyPI and can be installed with the command `pip install ngram-lm`.

### Usage
Given some ARPA file, a trie can be built like this:
```python
from ngram_lm.trie import build
lm_order = 3
arpa_path = "/path/to/arpa-file"
out_path = "/the/desired/output/path/where/trie/will/be/saved"
build(lm_order, arpa_path, out_path)
```

Then, the trie can be loaded into memory and queried for next word predictions:
```python
from ngram_lm.trie import Trie
t = Trie(out_path)
context = ["ele", "foi"]
n_predictions = 5
predictions = t.next_word_predictions(context, n_predictions)
for p in predictions:
    print(p)
```
