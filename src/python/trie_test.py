from ngram_lm.trie import Trie


def test_trie():
    t = Trie.from_arpa(3, "data/tmp.arpa")
    assert t.get_word_id("amarrar") == 1
    assert "que" == str(t.nwp(["PAra", "é"])[0])
    assert "os" == str(t.nwp(["que"])[0])
    assert "que" == str(t.nwp(["havia", "é"])[0])
    predictions = t.nwp(["é", "que"], 3)
    assert "os" == str(predictions[0])
    assert "levaram" == str(predictions[1])
    assert "já" == str(predictions[2])
