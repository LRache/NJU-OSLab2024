heap = [False] * (0x1000000 - 0x700000)

f = open("./log.txt")
allocated = {}
i = 0
for line in f:
    print(i, line.strip())
    line = line.split()
    cpu = int(line[0])
    counter = int(line[1])
    
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
        ptr = int(line[3][2:], 16) - 0x700000
        for j in range(ptr, allocated[ptr]):
            if not heap[j]:
                raise ValueError(i)
            heap[j] = False
    else:
        raise ValueError(t)
    i += 1

print("PASS")
