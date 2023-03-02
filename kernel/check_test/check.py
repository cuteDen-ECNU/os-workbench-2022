
behavior_list = []
ptr_list = []
size_list = []

def readbehavior():
    fd = open('check/repeat_addr', 'r')
    line = fd.readline()

    while line:
        str_list = line.split(" ")
        if("alloc" in str_list):
            behavior = ["a", int(str_list[5][:-1], 16), int(str_list[-1][:-1], 16)]
            # print(behavior)
        elif("free" in str_list):
            behavior = ["f", int(str_list[5][:-1], 16)]
            # print(behavior) 
        else:
            print("format is wrong")
            print(line) 
            exit(0)
        behavior_list.append(behavior)    
        line = fd.readline()

def binarySearch (arr, l, r, x): 
    if r >= l: 
        mid = int(l + (r - l)/2)       
        if arr[mid] == x: 
            return mid 
        elif arr[mid] > x: 
            return binarySearch(arr, l, mid-1, x) 
        else: 
            return binarySearch(arr, mid+1, r, x) 
    else: 
        return l


def check():
    for behavior in behavior_list:
        pos = binarySearch(ptr_list, 0, len(ptr_list) - 1, behavior[1]) 
        print(ptr_list)
        print(size_list)
        print(behavior)
        print(pos)
        
        if(len(ptr_list) == 0):
            ptr_list.append(behavior[1])
            size_list.append(behavior[2])
            continue
        if(behavior[0] == 'a'):
            
            if((pos > 0) & (behavior[1] < (ptr_list[pos - 1] + size_list[pos - 1]))):
                print("address is used.")
                exit(0)
            ptr_list.insert(pos, behavior[1])
            size_list.insert(pos, behavior[2])
        else:
            if(behavior[1] != ptr_list[pos]):
                print("ptr is not exit.")
                exit(0)
            del ptr_list[pos]
            del size_list[pos]



readbehavior()
check()