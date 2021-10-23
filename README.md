# *n*-gram Language Model for Next Word Prediction

See the C's [README](src/c/README.md).

See the python binding available at https://pypi.org/project/ngram-lm.

See a binding SpaCy component available at https://github.com/joaompfe/word-prediction

See the white paper at https://joaompfe.github.io/assets/bachelor-final-project.pdf

---
Note: The backoff values are currently not being loaded, such that the 
assigned conditioned probabilities are not reliable, which makes the model 
not suitable for estimating sentence probabilities.
