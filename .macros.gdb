define re
    monitor reset halt
end

define pprint
    printf "%s", $arg0
end

define findram
    find /w 0x20000000, +128*1024, $arg0
end

define findheap
    find /w 0x10000000, +64*1024, $arg0
end