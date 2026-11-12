"""MacRoman names and the HFS catalog ordering."""
import unittest

from . import util  # noqa: F401  (sets sys.path)
from hdlib import macroman
from hdlib.macroman import sort_key, encode, to_host, from_host, NameError_


def mr(text):
    return text.encode("mac_roman")


class OrderTable(unittest.TestCase):
    def test_table_shape(self):
        self.assertEqual(len(macroman.ORDER), 256)
        # control characters and the high symbols map to themselves
        for b in list(range(0x20)) + list(range(0xD8, 0x100)):
            if b != 0xD8:
                self.assertEqual(macroman.ORDER[b], b)

    def test_case_insensitive(self):
        for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
            self.assertEqual(sort_key(mr(c)), sort_key(mr(c.lower())))
        for u, l in zip("ÄÅÇÉÑÖÜÀÃÕÆØŒ", "äåçéñöüàãõæøœ"):
            self.assertEqual(sort_key(mr(u)), sort_key(mr(l)), (u, l))
        self.assertEqual(macroman.compare(b"FILE.TXT", b"file.txt"), 0)

    def test_diacritics_sort_next_to_base_but_differ(self):
        self.assertLess(sort_key(mr("a")), sort_key(mr("ä")))
        self.assertLess(sort_key(mr("ä")), sort_key(mr("b")))
        self.assertNotEqual(sort_key(mr("resume")), sort_key(mr("résumé")))
        # the catalog compares byte by byte through the table (no primary/secondary
        # passes like the International Utilities): the first differing letter decides
        self.assertLess(sort_key(mr("resume")), sort_key(mr("resumf")))
        self.assertLess(sort_key(mr("resumf")), sort_key(mr("résumé")))
        self.assertLess(sort_key(mr("résumé")), sort_key(mr("rf")))

    def test_known_mac_order(self):
        # Ascending order as the Mac OS File Manager (RelString) sorts catalog names.
        # Note what ASCII would get wrong: 'a' < 'Z', '`' sorts among the A's,
        # and '[', '_', '{', '~' all sort after the letters.
        expected = [" ", "!", "0", "9", "A", "à", "Ä", "å", "æ", "`", "B", "b1", "C", "ç", "e", "é", "f",
                    "O", "ö", "õ", "ø", "œ", "P", "s", "ß", "T", "Z", "[", "_", "{", "~", "†", "∞"]
        keys = [sort_key(mr(x)) for x in expected]
        for i in range(1, len(keys)):
            self.assertLess(keys[i - 1], keys[i], (expected[i - 1], expected[i]))
        shuffled = sorted(expected, key=lambda s: s[::-1] + s)
        self.assertEqual(sorted(shuffled, key=lambda s: sort_key(mr(s))), expected)

    def test_classic_finder_examples(self):
        names = ["System Folder", "system", "Zebra", "apple", "Apple II", "_private", "[x]", "Émile",
                 "Emile2", "emile", "10", "9", "2"]
        got = sorted(names, key=lambda s: sort_key(mr(s)))
        self.assertEqual(got, ["10", "2", "9", "apple", "Apple II", "emile", "Emile2", "Émile",
                               "system", "System Folder", "Zebra", "[x]", "_private"])

    def test_prefix_and_parent_order(self):
        self.assertLess(sort_key(b"abc"), sort_key(b"abcd"))
        self.assertLess(macroman.catalog_key(2, b"zzz"), macroman.catalog_key(16, b""))
        self.assertLess(macroman.catalog_key(16, b""), macroman.catalog_key(16, b"\x01"))
        # parent IDs compare as unsigned numbers, not as bytes
        self.assertLess(macroman.catalog_key(255, b"a"), macroman.catalog_key(256, b"a"))


class Encoding(unittest.TestCase):
    def test_plain(self):
        self.assertEqual(encode("Return to PlusOS"), b"Return to PlusOS")
        self.assertEqual(encode("résumé"), b"r\x8esum\x8e")
        self.assertEqual(encode("résumé"), b"r\x8esum\x8e")      # NFD input

    def test_transliterate(self):
        warned = []
        self.assertEqual(encode("žluťoučký", warn=warned.append), b"zlutoucky")
        self.assertTrue(warned)
        self.assertEqual(encode("a:b"), b"a-b")
        self.assertEqual(encode("中.txt"), b"_.txt")

    def test_strict(self):
        for bad in ("ž", "a:b", "中"):
            with self.assertRaises(NameError_):
                encode(bad, strict=True)

    def test_length(self):
        self.assertEqual(len(encode("x" * 31)), 31)
        with self.assertRaises(NameError_):
            encode("x" * 32)
        with self.assertRaises(NameError_):
            encode("")
        with self.assertRaises(NameError_):
            encode("x" * 28, limit=macroman.MAX_VOLNAME)

    def test_host_roundtrip_every_byte(self):
        for b in range(1, 256):
            if b == 0x3A:
                continue
            for raw in (bytes([b]), b"x" + bytes([b]), bytes([b]) + b"x", b"a" + bytes([b]) + b"z"):
                host = to_host(raw)
                self.assertFalse(set(host) & set('<>:"/|?*' + chr(92)), host)
                self.assertFalse(host.endswith((".", " ")), repr(host))
                self.assertEqual(from_host(host, strict=True), raw, (raw, host))

    def test_host_special(self):
        self.assertEqual(to_host(b"a/b"), "a%2Fb")
        self.assertEqual(from_host("a%2Fb"), b"a/b")
        self.assertEqual(to_host(b"100%"), "100%25")
        self.assertEqual(to_host(b"CON"), "%43ON")
        self.assertEqual(to_host(b"nul.txt"), "%6Eul.txt")
        self.assertEqual(to_host(b"._x"), "%2E_x")
        self.assertEqual(to_host(b".rsrc", reserved=(".rsrc",)), "%2Ersrc")
        self.assertEqual(to_host(b".rsrc"), ".rsrc")
        for raw in (b"CON", b"nul.txt", b"._x", b"trailing.", b"trailing "):
            self.assertEqual(from_host(to_host(raw)), raw)


if __name__ == "__main__":
    unittest.main()
