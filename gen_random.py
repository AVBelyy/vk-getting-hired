import random

data_size = 128 * 1024 * 1024
valid_chars = ['\n'] + map(chr, range(32, 128))

with open("dict.txt", "w") as f:
    for x in xrange(data_size):
        f.write(random.choice(valid_chars))
