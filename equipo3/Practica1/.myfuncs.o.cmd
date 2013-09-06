cmd_/home/equipo3/Desktop/Practica1/myfuncs.o := gcc -Wp,-MD,/home/equipo3/Desktop/Practica1/.myfuncs.o.d  -nostdinc -isystem /usr/lib/gcc/i686-linux-gnu/4.6/include  -I/usr/src/linux-headers-3.8.0-29-generic/arch/x86/include -Iarch/x86/include/generated  -Iinclude -I/usr/src/linux-headers-3.8.0-29-generic/arch/x86/include/uapi -Iarch/x86/include/generated/uapi -I/usr/src/linux-headers-3.8.0-29-generic/include/uapi -Iinclude/generated/uapi -include /usr/src/linux-headers-3.8.0-29-generic/include/linux/kconfig.h -Iubuntu/include  -D__KERNEL__ -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -m32 -msoft-float -mregparm=3 -freg-struct-return -fno-pic -mpreferred-stack-boundary=2 -march=i686 -mtune=generic -maccumulate-outgoing-args -Wa,-mtune=generic32 -ffreestanding -fstack-protector -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -DCONFIG_AS_CFI_SECTIONS=1 -DCONFIG_AS_AVX=1 -DCONFIG_AS_AVX2=1 -pipe -Wno-sign-compare -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -Wframe-larger-than=1024 -Wno-unused-but-set-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -pg -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO  -DMODULE  -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(myfuncs)"  -D"KBUILD_MODNAME=KBUILD_STR(prac1)" -c -o /home/equipo3/Desktop/Practica1/.tmp_myfuncs.o /home/equipo3/Desktop/Practica1/myfuncs.c

source_/home/equipo3/Desktop/Practica1/myfuncs.o := /home/equipo3/Desktop/Practica1/myfuncs.c

deps_/home/equipo3/Desktop/Practica1/myfuncs.o := \
  /home/equipo3/Desktop/Practica1/myfuncs.h \

/home/equipo3/Desktop/Practica1/myfuncs.o: $(deps_/home/equipo3/Desktop/Practica1/myfuncs.o)

$(deps_/home/equipo3/Desktop/Practica1/myfuncs.o):
