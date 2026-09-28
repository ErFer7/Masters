all:
	g++ -O -pthread stress_test.cpp -o stress

riscv:
	riscv64-linux-gnu-g++ -O -static -pthread stress_test.cpp -o stress

clean:
	rm stress_test

.ONESHELL:
.PHONY: run
run:
	echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us
	sudo ./stress_test
	echo 950000 | sudo tee /proc/sys/kernel/sched_rt_runtime_us

.ONESHELL:
.PHONY: install
install:
	cp ./stress /home/erfer/Code/LISHA/RTLinux/Source/busybox/_install/bin
	cd /home/erfer/Code/LISHA/RTLinux/Source/busybox/_install
	find . | cpio -o -H newc > ../../../initramfs.cpio
	cd /home/erfer/Code/LISHA/RTLinux
	mv initramfs.cpio initramfs.cpio.gz
	sudo cp ./initramfs.cpio.gz /srv/tftp
