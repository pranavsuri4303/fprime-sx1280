#!/Users/pranav/Developer/fprime-sx1280/lora-fprime-demo/fprime-venv/bin/python3.11
import sys
from clang_format.clang_format_diff import main
if __name__ == '__main__':
    sys.argv[0] = sys.argv[0].removesuffix('.exe')
    sys.exit(main())
