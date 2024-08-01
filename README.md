# RockYou Searcher

RockYou Searcher was developed to search large files like rockyou.txt. The file contains an enormous password compilation. The file size is around 150GB. You can check the passwords to see if they were leaked via RockYou Searcher.

## Download

<a href="https://github.com/exploit-development/RockYou2024" target="_blank">https://github.com/exploit-development/RockYou2024</a>

## Build

```
clang main.c -o rockyou_searcher
```

## Prepare

Insert the patterns with '\n' line terminator except the last line

```
echo -e "secret_1\nsecret_2" > patterns
```

## Run

```
./rockyou_searcher -s rockyou2024.txt -p patterns -o result
```

## The -e option

With -e option compares the line with pattern[s]
<br>
Without -e option checks the line contains pattern[s]

## Result

The result file contains the line with the cursor position from beginning

```
[1002928]
secret_1

[77129280]
secret_2
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
