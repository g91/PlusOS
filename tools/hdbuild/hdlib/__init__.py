"""hdlib - the library behind tools/hdbuild/hdbuild.py.

Pure Python 3 (standard library only).  Modules:

    macroman   MacRoman names, the HFS catalog sort order, host-name escaping
    hfs        on-disk structures + the from-scratch HFS formatter / writer
    hfsread    HFS volume reader (catalog, extents overflow, forks)
    fsck       volume / partition-map consistency checker
    apm        Driver Descriptor Record + Apple Partition Map
    forks      MacBinary / AppleSingle / AppleDouble decoding
    manifest   the .hdbuild.json manifest (schema, defaults, dates, sizes)
    stage      lay the volume out as a host folder
    image      folder -> image, image -> folder
    cli        command line front end
"""

__version__ = "1.0"
