import os

from Cython.Build import cythonize
from setuptools import Extension, setup
from pathlib import Path

long_description = (Path(__file__).parent / "README.md").read_text()

project_dir = os.environ["NGRAM_LM_DIR"]

names = ["trie", "word"]
extension_names = ["ngram_lm." + n for n in names]
sources = [str(n + ".pyx") for n in names]

extensions = [Extension(name, [sources],
                        include_dirs=[project_dir + "/src/c"],
                        libraries=["ngram_lm"],
                        library_dirs=[
                            project_dir + "/build/cmake/release/src/c"])
              for name, sources in zip(extension_names, sources)]

ext_modules = cythonize(extensions,
                        language_level="3")
setup(name="ngram-lm",
      version="0.1.0",
      author="João Fé",
      author_email="joaofe2000@gmail.com",
      ext_modules=ext_modules,
      long_description=long_description,
      long_description_content_type='text/markdown')
