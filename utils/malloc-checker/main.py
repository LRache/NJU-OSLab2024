heap = [False] * (0x8c0000 - 0x700000)

f = open("./log.txt")
allocated = {}
for i in range(8000):
    line = f.readline().split()
    t = line[2]
    if t == "alloc":
        start = int(line[3][2:], 16) - 0x700000
        end   = int(line[4][2:], 16) - 0x700000
        for j in range(start, end):
            if heap[j]:
                raise ValueError(i)
            heap[j] = True
        allocated[start] = end
    elif t == "free":
        ptr = int(line[3]) - 0x70000
        for j in range(ptr, allocated[ptr]):
            if not heap[j]:
                raise ValueError(i)
            heap[j] = False
    else:
        raise ValueError(t)
