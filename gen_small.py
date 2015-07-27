import random

data_size = 128 * 1024 * 1024
valid_chars = map(chr, range(32, 128))
total_size = 0
str_sizes = range(3, 16)

with open("dict.txt", "w") as f:
    while total_size < data_size:
        cur_size = random.choice(str_sizes)
        for x in xrange(cur_size):
            f.write(random.choice(valid_chars))
        f.write('\n')
        total_size += cur_size + 1
