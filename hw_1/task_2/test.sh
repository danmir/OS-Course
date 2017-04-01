dd if=/dev/urandom bs=4096 count=2 of=file.hole
dd if=/dev/urandom seek=7 bs=4096 count=2 of=file.hole

dd if=/dev/zero bs=4096 count=9 of=file.nohole
