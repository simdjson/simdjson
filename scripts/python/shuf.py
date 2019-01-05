#table1=[16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0]
#table2=[8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0]
table1=[0 for i in range(16)]
table2=[0 for i in range(16)]
spaces0=[0x0a,0x09,0x0d]
spaces2=[0x20]
struct2=[0x2c]
struct3=[0x3a]
struct57=[0x5b,0x5d,0x7b,0x7d]

for s in struct2:
    table1[s&0xF]|=1
    table2[(s>>4)&0xF]|=1
for s in struct3:
    table1[s&0xF]|=2
    table2[(s>>4)&0xF]|=2
for s in struct57:
    table1[s&0xF]|=4
    table2[(s>>4)&0xF]|=4

for s in spaces0:
    table1[s&0xF]|=8
    table2[(s>>4)&0xF]|=8
for s in spaces2:
    table1[s&0xF]|=16
    table2[(s>>4)&0xF]|=16

print(len(table1), len(table2))
print(table1)
print(table2)
for i in range(256):
    low = i & 0xF
    high = (i >> 4) & 0xF
    m = table1[low] & table2[high]
    if(m!=0):
      if(i>=0x20):
        print(hex(i), chr(i), bin(m))
      else:
        print(hex(i), bin(m))
