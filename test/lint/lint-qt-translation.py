#!/usr/bin/env python3
#
import subprocess
import sys


def main():
    tr_strings = subprocess.run(['git', 'grep', '-e', 'tr("[[:space:]]', '--', 'src/qt'], stdout=subprocess.PIPE, text=True).stdout

    if tr_strings.strip():
        print("Avoid leading whitespaces in:")
        print(tr_strings)
        sys.exit(1)


if __name__ == "__main__":
    main()
