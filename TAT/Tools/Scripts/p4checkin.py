# (c) 2025 OtherSide Entertainment, Inc
# SPDX-License-Identifier: MIT
# usage: p4checkin.py <commit_message> <root_dir>
# checks in files in chunks, because I could not get 40gb of unreal engine files submitted for add in one large changelist.
# at least if we fail here we're only failing on a much smaller commit.
# the script will throw an exception and stop and will need to be restarted -- I suppose we could fix that, but I didn't need to once I set this up.
# once this script was done I ran a p4 reconcile on the entire tree to make sure I didn't miss anything.

import P4
import os
import sys

START_AT_DIR = None # "C:\\prj\\Epic\\Release-4.25.4\\Engine\\Source\\ThirdParty\\PhysX3"
MAX_FILES_PER_CHANGELIST = 500

def CheckOutDir(root_dir):
   p4 = P4.P4()
   p4.connect()
   info = p4.run("add", "-f", "{}\\...".format(root_dir))

   file_list = []
   for obj in info:
      if isinstance(obj, dict):
         if 'depotFile' in obj:
            file_list.append(obj['depotFile'])
      else:
         print(obj)

   p4.disconnect()
   return file_list

def SubmitFiles(files, commit_message):
   p4 = P4.P4()
   p4.connect()
   change = p4.fetch_change()
   change._description = "{}".format(commit_message)
   change._files = files
   p4.run_submit( change )
   p4.disconnect()

def MakeSmallChangelists(file_list, num_files):
   for i in range(0, len(file_list), num_files):  
      yield file_list[i:i + num_files]

def main():
   print('Argument List:', str(sys.argv))
   if (len(sys.argv) < 3):
      return

   commit_message = sys.argv[1]
   root_dir = sys.argv[2]

   print('Commit Message:', commit_message)
   print('Root Dir:', root_dir)

   dir_list = []
   start_adding = START_AT_DIR is None
   for obj in os.listdir(root_dir):
      full_path = os.path.join(root_dir, obj)
      if (full_path == START_AT_DIR):
         start_adding = True
      if (not start_adding):
         continue
      if os.path.isdir(full_path):
         dir_list.append(full_path)
      # TODO: handle files in the root dir

   print("Found {} dirs ...".format(len(dir_list)))

   for local_dir in dir_list:
      print("Checking out dir {} ...".format(local_dir))
      files = CheckOutDir(local_dir)
      if (len(files) > 0):
         print("Found {} files...".format(len(files)))
         chunk_list = list(MakeSmallChangelists(files, MAX_FILES_PER_CHANGELIST))
         print("Broken down into {} chunks with no more than {} files per chunk".format(len(chunk_list), MAX_FILES_PER_CHANGELIST))
         chunkNum = 0
         for chunk_files in chunk_list:
            print("Submitting {} files w/ commit message \"{}\" in chunk {}".format(len(chunk_files), commit_message, chunkNum))
            SubmitFiles(chunk_files, commit_message)
            chunkNum += 1
      else:
         print("Skipping submitting files in dir {} because there were none!".format(local_dir))

if __name__ == "__main__":
   main()
