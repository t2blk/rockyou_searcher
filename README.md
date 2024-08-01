# RockYou Searcher

RockYou Searcher was developed to search large files like rockyou.txt. The file contains an enormous password compilation. The file size is around 150GB. You can check the passwords to see if they were leaked via RockYou Searcher.

## Build

```
clang main.c -o rockyou_searcher
```

## Prepare

Insert the patterns with '\n' line terminator except the last line

```
echo -e "secret_1\nsecret_2" > patterns
```

## Usage

```
-s <SOURCE> -p <PATTERNS> -o <OUTPUT> [OPTIONS]

Options:

-t		num of threads
-e		use strcmp for patterns (default strstr)
-x		don't print patterns
-h		help
```

## The -e option

With -e option compares the line with pattern[s]
<br>
Without -e option checks the line contains pattern[s]

## Run

```
./rockyou_searcher -s rockyou2024.txt -p patterns -o result
```

## Result

The result file contains the line with the cursor position from beginning

```
[1002928]
secret_1

[77129280]
secret_2
```

## Test

- 128 bytes, 12 lines patterns file
- Intel(R) Core(TM) i7-2620M CPU @ 2.70GHz 4 core
```
hdparm -Tt /dev/sdb

/dev/sdb:
 Timing cached reads:   11808 MB in  1.99 seconds = 5918.94 MB/sec
 Timing buffered disk reads: 564 MB in  3.01 seconds = 187.47 MB/sec
```

<br>
  
```
time ./rockyou_searcher -s rockyou2024.txt -p patterns -o result

229% cpu 2:25:46.39 total
```

- Max memory usage is around 3MB

### !!! DON'T FORGET REMOVE THE PATTERNS FILE !!!

