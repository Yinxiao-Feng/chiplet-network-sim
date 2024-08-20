import math 

## DOJO-like
print("DOJO T_locl:{:.2f} ".format(10*2*2/25))
print("DOJO T_glob:{:.2f} ".format((30*2+60)*2/450))

## Dragonfly
num_group = 545
switch_per_group = 32
num_cabinet = 2180
total_length = 0
scale_cabinet = 46
scale_group = 23
num_global_cable = num_group*(num_group-1)//2 
num_local_cable = num_group*32*31//2
num_terminal_cable = num_group*32*16
print("Switch-based Switch Number:{}".format(num_group*switch_per_group))
print("Switch-based Cable Number:{} ".format(num_global_cable+num_local_cable+num_terminal_cable))
print("Switch-less Cable Number:{} ".format(num_global_cable+num_local_cable))
## 545 groups are fully connected, count global cables
for i in range(scale_group):
    for j in range(scale_group):
        for k in range(scale_group):
            for l in range(scale_group):
                total_length += math.sqrt((i*2-k*2)**2+(j*2-l*2)**2)
## 32 routers in a group are fully connected, count local cables
total_length += num_group*32*31//2 * math.sqrt(2)
print("SW-based Cable Length:{:.2f} ".format(total_length/scale_cabinet))
total_length = 0
for i in range(scale_group):
    for j in range(scale_group):
        for k in range(scale_group):
            for l in range(scale_group):
                total_length += math.sqrt((i-k)**2+(j-l)**2)
print("SW-less Cable Length:{:.2f} ".format(total_length/scale_cabinet))

## Fat-tree
num_switch = 32*(64+32)+32*64
print("FT Switch Number:{} ".format(num_switch))
num_chip = 32*64*32
print("FT Chip Number:{} ".format(num_chip))
print("FT Cable Number:{} ".format((num_switch*64+num_chip)//2))
print("FT Cabinet Number:{} ".format(num_chip//128+32*(64+32)//32))

## Taper Fat-tree (radix-4)
num_switch = (16*(64+32)+32*64)*4
print("Taper FT Switch Number:{} ".format(num_switch))
num_chip = 32*64*48
print("Taper FT Chip Number:{} ".format(num_chip))
print("Taper FT Cable Number:{} ".format((num_switch*64+num_chip*4)//2))
print("Taper FT Cabinet Number:{} ".format(num_chip//128+16*(64+32)*4//32))

## HammingMesh (1-plane)
num_switch = 32*(64+32)+32*64
print("1-plane Hx4Mesh Switch Number:{} ".format(num_switch))
num_chip = 32*64*32 // 16 * 16
print("1-plane Hx4Mesh Chip Number:{} ".format(num_chip))
print("1-plane Hx4Mesh Cable Number:{} ".format((num_switch*64+32*64*32)//2))
print("1-plane Hx4Mesh Cabinet Number:{} ".format(num_chip//(16*16)+32*(64+32)//32))

## HammingMesh (4-plane)
num_switch = (32*(64+32)+32*64)*4
print("4-plane Hx4Mesh Switch Number:{} ".format(num_switch))
num_chip = 32*64*32 // 16 * 16
print("4-plane Hx4Mesh Chip Number:{} ".format(num_chip))
print("4-plane Hx4Mesh Cable Number:{} ".format((num_switch*64+32*64*32*4)//2))
print("4-plane Hx4Mesh Cabinet Number:{} ".format(num_chip//(16*16)+32*(64+32)*4//32))

## PolarFly
num_switch = 63*63+63+1
print("PolarFly Switch Number:{} ".format(num_switch))
num_chip = num_switch*32
print("PolarFly Chip Number:{} ".format(num_chip))
print("PolarFly Cable Number:{} ".format(num_switch*64//2))