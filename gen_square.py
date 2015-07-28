import random
from math import sqrt

data_size = 128 * 1024 * 1024
valid_chars = map(chr, range(32, 128))
total_size = 0
str_size = int(sqrt(data_size))

with open("dict.txt", "w") as f:
    while total_size < data_size:
        for x in xrange(str_size):
            f.write(random.choice(valid_chars))
        f.write('\n')
        total_size += str_size + 1
