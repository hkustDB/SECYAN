import os

g = os.walk("3MB")

for path, dir_list, file_list in g:
    for file_name in file_list:
        file_path = os.path.join(path, file_name)
        if not file_path.endswith(".csv"):
            continue
        f = open(file_path,"r")
        s = f.readlines()
        f.close()
        numc = s[1].count('|') + 1
        t = []
        t.append(str(numc))
        for i in s:
            i=i.strip()
            if i:
                t.append(i+'|')
        t[1] = t[1].lower()
        file_path = file_path.replace('_table_','')
        file_path = file_path.replace('csv','tbl')
        f = open(file_path,"w")
        f.write('\n'.join(t))
        f.close()
