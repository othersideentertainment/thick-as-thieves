# (c) 2025 OtherSide Entertainment, Inc
# SPDX-License-Identifier: MIT
# usage: rename_project_code_classes.py  <root_dir>
# renames actors, components, structs, and interfaces from one project to another
# generates a list of core redirectors
# NOTE: structs may also require ActiveStructRedirects too, as of UE5, to properly deserialize into containers (eg TArray).
#       this file does not generate these redirects, they can be duplicated out from StructRedirects

import P4
import os
import sys
import re

FILE_TYPES = ["cpp", "h"]
OLD_PROJECT_NAME = "TOW"
NEW_PROJECT_NAME = "TAT"

def FixupByRegex(lines, regex): # lines is a list, so mutable, and modifying here will modify the calling func
   struct_regex = re.compile(regex)
   out_renames = []
   for idx, line in enumerate(lines):
      for old_name in struct_regex.findall(line):
         #print("{} {} {}".format(file_path, idx,match))
         new_name = old_name.replace(OLD_PROJECT_NAME, NEW_PROJECT_NAME)
         out_renames.append((old_name[1:], new_name[1:]))
         lines[idx] = line.replace(old_name, new_name)
   return out_renames

def FixupFile(file_path):
   struct_renames = []
   actor_renames = []
   component_renames = []
   interface_renames = []
   enum_renames = []

   lines = []
   with open(file_path, "r") as file:
      lines = file.readlines()

   struct_renames = FixupByRegex(lines, "(F{}[^\s(&;\>,*\":)]+)".format(OLD_PROJECT_NAME))
   actor_renames = FixupByRegex(lines, "(A{}[^\s(&;\>,*\":)]+)".format(OLD_PROJECT_NAME))
   component_renames = FixupByRegex(lines, "(U{}[^\s(&;\>,*\":)]+)".format(OLD_PROJECT_NAME))
   interface_renames = FixupByRegex(lines, "(I{}[^\s(&;\>,*\":)]+)".format(OLD_PROJECT_NAME))
   enum_renames = FixupByRegex(lines, "(E{}[^\s(&;\>,*\":)]+)".format(OLD_PROJECT_NAME))

   with open(file_path, 'w') as file:
      file.writelines(lines)
   
   return struct_renames, actor_renames, component_renames, interface_renames, enum_renames

def main():
   print('Argument List:', str(sys.argv))
   if (len(sys.argv) < 2):
      return

   root_dir = sys.argv[1]

   print('Root Dir:', root_dir)

   files_list = []
   for root, dirs, files in os.walk(root_dir):
      for file in files:
         for file_type in FILE_TYPES:
            if file_type in file:
               files_list.append(os.path.join(root, file))
               break

   #for file in files_list:
      #print(file)

   print("Found {} files ...".format(len(files_list)))

   struct_renames = []
   actor_renames = []
   component_renames = []
   interface_renames = []
   enum_renames = []

   for i in range(20): # HACK because it's not picking up multiple classes on one line in the first pass, and this is faster than fixing my script :D
      for file in files_list:
         struct_rename, actor_rename, component_rename, interface_rename, enum_rename = FixupFile(file)
         for pair in struct_rename:
            if pair not in struct_renames:
               struct_renames.append(pair)
         for pair in actor_rename:
            if pair not in actor_renames:
               actor_renames.append(pair)
         for pair in component_rename:
            if pair not in component_renames:
               component_renames.append(pair)
         for pair in interface_rename:
            if pair not in interface_renames:
               interface_renames.append(pair)
         for pair in enum_rename:
            if pair not in enum_renames:
               enum_renames.append(pair)

   print("Found {} struct renames ...".format(len(struct_renames)))
   print("Found {} actor renames ...".format(len(actor_renames)))
   print("Found {} component renames ...".format(len(component_renames)))
   print("Found {} interface renames ...".format(len(component_renames)))
   print("Found {} interface renames ...".format(len(enum_rename)))

   for struct_rename_pair in struct_renames:
         print("+StructRedirects=(OldName=\"/Script/{new_project_name}.{old_class_name}\",NewName=\"/Script/{new_project_name}.{new_class_name}\")"
            .format(new_project_name=NEW_PROJECT_NAME, old_class_name=struct_rename_pair[0], new_class_name=struct_rename_pair[1]))

   for actor_rename_pair in actor_renames:
      print("+ClassRedirects=(OldName=\"/Script/{new_project_name}.{old_class_name}\",NewName=\"/Script/{new_project_name}.{new_class_name}\")"
         .format(new_project_name=NEW_PROJECT_NAME, old_class_name=actor_rename_pair[0], new_class_name=actor_rename_pair[1]))

   for component_rename_pair in component_renames:
      print("+ClassRedirects=(OldName=\"/Script/{new_project_name}.{old_class_name}\",NewName=\"/Script/{new_project_name}.{new_class_name}\")"
         .format(new_project_name=NEW_PROJECT_NAME, old_class_name=component_rename_pair[0], new_class_name=component_rename_pair[1]))
   
   for enum_rename_pair in enum_renames:
      print("+EnumRedirects=(OldName=\"/Script/{new_project_name}.{old_class_name}\",NewName=\"/Script/{new_project_name}.{new_class_name}\")"
         .format(new_project_name=NEW_PROJECT_NAME, old_class_name=enum_rename_pair[0], new_class_name=enum_rename_pair[1]))

if __name__ == "__main__":
   main()
