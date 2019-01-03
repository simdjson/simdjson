import os
import pandas as pd
import numpy as np
#import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.linear_model import Ridge
from sklearn.linear_model import Lasso
from sklearn.preprocessing import normalize
from sklearn import metrics

def displaycoefs(coef_name):
    coef_name.sort()
    coef_name.reverse()
    for c,n in coef_name:
        print("\t%0.2g cycles per %s "%(c,n))

datafile = "modeltable.txt" ## from ./scripts/statisticalmodel.sh

predictors = ["integer_count", "float_count", "string_count", "backslash_count", "nonasciibyte_count", "object_count", "array_count", "null_count", "true_count", "false_count", "byte_count", "structural_indexes_count"]
targets = ["stage1_cycle_count", "stage1_instruction_count", "stage2_cycle_count", "stage2_instruction_count"]

print("loading", datafile)
dataset = pd.read_csv(datafile, delim_whitespace=True, skip_blank_lines=True, comment="#", header=None, names = predictors + targets)



dataset.columns = predictors + targets

dataset['total_cycles']=dataset['stage1_cycle_count']+dataset['stage2_cycle_count']
dataset['ratio']=dataset['total_cycles']/dataset['byte_count']
#print(dataset[['ratio']])
print(dataset.describe())

chosenpredictors = predictors
print("chosenpredictors=",chosenpredictors)
print()
chosentargets=["stage1_cycle_count", "stage2_cycle_count","total_cycles"]
for t in chosentargets:
    print("target = ", t)
    howmany = 1 # we want at most one predictors
    if(t.startswith("stage1")):
        howmany = 2 # we allow for less
    if(t.startswith("stage2")):
        howmany = 3 # we allow for more
    if(t.startswith("total")):
        howmany = 3 # we allow for more
    A=10000000.0
    while(True):
      regressor = Lasso(max_iter=100000, alpha=A, positive = True, normalize=False,  fit_intercept=False) #LinearRegression(normalize=False,  fit_intercept=False)
      x = dataset[chosenpredictors]
      y = dataset[[t]]
      regressor.fit(x, y)
      rest = list(filter(lambda z:  z[0] != 0, zip(regressor.coef_,chosenpredictors) ))
      nonzero = len(rest)
      if(nonzero > howmany):
        A *= 1.2
      else:
       #print(rest)
       displaycoefs(rest)
       print("R2 = ", regressor.score(x,y))
       Y_pred = regressor.predict(x)
       break
    print()
