import os
import csv

try: import pandas as pd
except ImportError:
    import pip
    pip.main(['install', '--user', 'pandas'])
    import pandas as pd

def getdata(filename):
    df = pd.read_csv(filename, delim_whitespace=True)
    return (df["gb_per_s"].tolist())

ourdir=os.path.dirname(os.path.realpath(__file__))
answer = []
for file in os.listdir(ourdir):
    if file.startswith("all") and file.endswith(".table"):
        fullpath = os.path.join(ourdir, file)
        answer.append([file[3:-11]]+getdata(fullpath))
print(" \t&\t simdjson \t&\t RapidJSON \t&\t RapidJSONinsitu \t&\t sajsondyn \t&\t sajson \t&\t dropbox-json11 \t&\t fastjson \t&\t gason \t&\t ultrajson \t&\t jsmn\t&\t cJSON \\\\")
answer.sort()
for l in answer:
    print("\t&\t".join(map(lambda x : (('{:.2f}'.format(x) if x < 1 else '{:.1f}'.format(x) ) if (type(x) is float) else x),l))+"\\\\")
