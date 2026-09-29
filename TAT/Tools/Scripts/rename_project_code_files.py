# (c) 2025 OtherSide Entertainment, Inc
# SPDX-License-Identifier: MIT
# usage: rename_project_code_files.py  <root_dir>
# renames project code files from one project name to another

import P4
import os
import sys
import re

OLD_PROJECT_NAME = "TOW"
NEW_PROJECT_NAME = "TAT"

def RenameProjectFile(p4, root, file_name):
   src_path = os.path.join(root, file_name)
   new_file_name = file_name.replace(OLD_PROJECT_NAME, NEW_PROJECT_NAME)
   dst_path = os.path.join(root, new_file_name)
   p4.run('move', '-r', src_path, dst_path)

def FixupHeaders(root, file_name):
   file_path = os.path.join(root, file_name)
   
   lines = []
   with open(file_path, "r") as file:
      lines = file.readlines()

   regex = re.compile("\/(TOW[^\.h]+)")
   for idx, line in enumerate(lines):
      for old_name in regex.findall(line):
         lines[idx] = line.replace(OLD_PROJECT_NAME, NEW_PROJECT_NAME)

   regex = re.compile("#include \"TOW")
   for idx, line in enumerate(lines):
      for old_name in regex.findall(line):
         lines[idx] = line.replace(OLD_PROJECT_NAME, NEW_PROJECT_NAME)         

   with open(file_path, 'w') as file:
      file.writelines(lines)

def main():
   print('Argument List:', str(sys.argv))
   if (len(sys.argv) < 2):
      return

   root_dir = sys.argv[1]

   print('Root Dir:', root_dir)

   # move all the files
   p4 = P4.P4()
   p4.connect()

   header_list = []
   cpp_list = []
   for root, dirs, files in os.walk(root_dir):
      for file in files:
         if file.startswith(OLD_PROJECT_NAME):
            if ".h" in file:
               header_list.append([root, file])
            elif ".cpp" in file:
               cpp_list.append([root, file])
         
   for file in cpp_list:
      print(os.path.join(file[0], file[1]))
      RenameProjectFile(p4, file[0], file[1])

   for file in header_list:
      print(os.path.join(file[0], file[1]))
      RenameProjectFile(p4, file[0], file[1])

   p4.disconnect()

   # try to fix up as many header includes as we can
   code_list = []
   for root, dirs, files in os.walk(root_dir):
      for file in files:
         if ".h" in file or ".cpp" in file:
            code_list.append([root, file])
   
   for file in code_list:
      FixupHeaders(file[0], file[1])


if __name__ == "__main__":
   main()
