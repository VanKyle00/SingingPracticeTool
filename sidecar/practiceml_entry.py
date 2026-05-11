"""Frozen entry point. PyInstaller targets this; it just hands off to the
package's main(). Lives outside the `practiceml/` dir so PyInstaller doesn't
treat the package's __main__.py as a top-level script (which breaks the
relative imports inside the package)."""
from practiceml.__main__ import main

if __name__ == "__main__":
    raise SystemExit(main())
