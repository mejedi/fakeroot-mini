# .
/set type=file uid=0 gid=0 mode=0755
.               type=dir

# ./usr
usr             type=dir

# ./usr/local
local           type=dir

# ./usr/local/bin
bin             type=dir mode=0775 ignore
# ./usr/local/bin
..

# ./usr/local
..


# ./usr/share
share           type=dir

# ./usr/share/man
man             type=dir

# ./usr/share/man/man1
man1            type=dir ignore
# ./usr/share/man/man1
..

# ./usr/share/man
..

# ./usr/share
..

# ./usr
..

..

