#!/usr/bin/env python3

import os
import sys
import subprocess
import shutil
import re


def find_binary_or_die(candidates):
  found_path = None
  for candidate in candidates:
    found_path = shutil.which(candidate)
    if found_path:
      return found_path
  sys.exit('No binary found for any of %s' % str(candidates))


class PreCommit(object):
  def __init__(self):
    self.exclude_directories = [
        r'^.*\.git/.*',
        r'^\./git/.*',
        r'^\./third_party/.*',
        r'^.*build/.*'
    ]
    self.c_files = []
    self.c_header_files = []
    self.cpp_files = []
    self.cpp_header_files = []
    self.python_files = []

    self.clang_format_candidates = ['clang-format', 'clang-format-3.6']
    self.clang_format_path = None


  def find_binaries(self):
    self.clang_format_path = find_binary_or_die(self.clang_format_candidates)


  def read_files(self):
    for dirpath, subdirs, files in os.walk('.'):
      if any(re.match(excluded_dir, dirpath) for excluded_dir in self.exclude_directories):
        continue
      for f in files:
        filepath = os.path.join(dirpath, f)
        if f.endswith('.c'):
          self.c_files.append(filepath)
        elif f.endswith('.h'):
          self.c_header_files.append(filepath)
        elif f.endswith('.cpp'):
          self.cpp_files.append(filepath)
        elif f.endswith('.hpp'):
          self.cpp_header_files.append(filepath)
        elif f.endswith('.py'):
          self.python_files.append(filepath)


  def print_files(self):
    print('c_files=%s' % str(self.c_files))
    print('c_header_files=%s' % str(self.c_header_files))
    print('cpp_files=%s' % str(self.cpp_files))
    print('cpp_header_files=%s' % str(self.cpp_header_files))
    print('python_files=%s' % str(self.python_files))


  def check_formating(self):
    for files in self.c_files, self.c_header_files, self.cpp_files, self.cpp_header_files:
      if files:
        clang_format_output = subprocess.check_output([self.clang_format_path, '-output-replacements-xml'] + files)
        if '<replacement ' in clang_format_output.decode():
          sys.exit('ERROR: Some code files (e.g. %s) are not correctly formated.  Run: tools/format_code.sh' % files[0])


def main():
  pre_commit = PreCommit()
  pre_commit.find_binaries()
  pre_commit.read_files()
  #pre_commit.print_files()
  pre_commit.check_formating()


if __name__ == '__main__':
  main()
