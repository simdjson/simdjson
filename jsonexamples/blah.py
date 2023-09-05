import sys
import re

# replace utf8
# file = open(sys.argv[1], 'rb')
# for line in file.readlines():
#     sys.stdout.write(bytes([(b if b < 128 else ord('_')) for b in line]).decode('ascii'))

# replace non-key strings with empty strings
file = open(sys.argv[1])
for line in file.readlines():
    sys.stdout.write(re.sub(r'\"([^\"]*)\"', lambda s: (' ' * len(s.group(1))) + '""', line))
