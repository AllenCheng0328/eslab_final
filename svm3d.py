import numpy as np
from sklearn.svm import SVC

ges_type = 4
files = []
files.append(open("sample_data/up.txt","r"))
files.append(open("sample_data/down.txt","r"))
files.append(open("sample_data/left.txt","r"))
files.append(open("sample_data/right.txt","r"))
#files.append(open("sample_data/forward.txt"))
test = open("test_data/hybrid.txt","r")

X = []
y = []
T = []
for kind in range(ges_type):
    for sample_num in range(10):
        x = []
        for line in range(9):
            x = x + files[kind].readline().split()
        X.append(x)
        y.append(kind)

for sample_num in range(15):
        t = []
        for line in range(9):
            t = t + test.readline().split()
        T.append(t)

X = np.array(X).astype('double')
y = np.array(y)
T = np.array(T).astype('double')
clf = SVC(C = 10000000000.0, kernel = 'rbf', decision_function_shape = 'ovo', cache_size = 1024)
clf.fit(X,y)

print(y)
print(clf.score(X,y))
print(clf.score(T,[0,0,1,1,2,2,3,3,4,4,0,1,2,3,4]))




