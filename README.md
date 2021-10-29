<h1 align="center">n-gram Language Model for Next Word Prediction</h1>

<p align="center">
  <a href="https://circleci.com/gh/joaompfe/workflows/ngram-lm/tree/master">
    <img src="https://img.shields.io/circleci/build/github/joaompfe/ngram-lm/master.svg?logo=circleci&logoColor=fff&label=CircleCI" alt="CI status" />
  </a>&nbsp;
  <a href="https://pypi.org/project/ngram-lm/">
    <img alt="PyPI" src="https://img.shields.io/pypi/v/ngram-lm?logo=pypi">
  </a>&nbsp;
  <a href="https://pypi.org/project/ngram-lm/">
    <img alt="PyPI - Format" src="https://img.shields.io/pypi/format/ngram-lm">
  </a>
</p>

---
See the C's [README](src/c/README.md).

See the python binding available at https://pypi.org/project/ngram-lm.

See a binding SpaCy component available at https://github.com/joaompfe/word-prediction

See the white paper at https://joaompfe.github.io/assets/bachelor-final-project.pdf

---
Note: The backoff values are currently not being loaded, such that the 
assigned conditioned probabilities are not reliable, which makes the model 
not suitable for estimating sentence probabilities.
