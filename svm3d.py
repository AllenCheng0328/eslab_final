import numpy as np
from sklearn.svm import SVC

ges_type = 5
files = []
files.append(open("sample_data/up.txt","r"))
files.append(open("sample_data/down.txt","r"))
files.append(open("sample_data/left.txt","r"))
files.append(open("sample_data/right.txt","r"))
files.append(open("sample_data/forward.txt"))
test = open("test_data/hybrid.txt","r")

X = []
y = []
T = []
u = []
for kind in range(ges_type):
    for sample_num in range(95):
        x = files[kind].readline().split()
        while len(x) < 600 :
            x.append(0)
        X.append(x)
        y.append(kind)

for kind in range(ges_type):
    for sample_num in range(5):
        t = files[kind].readline().split()
        while len(t) < 600 :
            t.append(0)
        T.append(t)
        u.append(kind)


X = np.array(X).astype('double')
y = np.array(y)
T = np.array(T).astype('double')
u = np.array(u)
clf = SVC(C = 10000.0, kernel = 'rbf', decision_function_shape = 'ovo', cache_size = 1024)
clf.fit(X,y)

print(y)
print(clf.score(X,y))
print(clf.predict(T))
print(clf.score(T,u))




