#!/usr/bin/python3

import sys

def verifyContent(f,filename):
  linenumber=-999
  line=''
  try:
    for linenumber, line in enumerate(f):
      try:
        ascii=line.encode('ascii')
      except UnicodeEncodeError as e:
        #print(f"a: found problem {e} at line {linenumber+1} in {filename}:")
        print(f"Found problem at line {linenumber+1} in {filename}:")
        print(line.rstrip())
        for col, char in enumerate(line.encode('utf-8')):
          if char>=127:
             offender=char
             offendingcol=col
             break
        print(" "*offendingcol + "^")
        print(f"Column {offendingcol+1} contains 0x{offender:02X}")
        sys.exit(1)

  except UnicodeDecodeError as e:
    print(f"Could not open {filename} as utf-8, it can't be ascii.")
    sys.exit(1)



for filename in sys.argv[1:]:
  with open(filename,encoding='utf-8') as f:
    #print(f"file {filename} was possible to open as utf-8")
    verifyContent(f,filename)
  print("all files were found to be ascii.")

