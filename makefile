all: ext2_mkdir ext2_checker readimage ext2_ln ext2_cp ext2_restore ext2_rm

ext2_mkdir: ext2_mkdir.o ext2_util.o
	gcc -Wall -g -o ext2_mkdir ext2_mkdir.o ext2_util.o

ext2_checker: ext2_checker.o ext2_util.o
	gcc -Wall -g -o ext2_checker ext2_checker.o ext2_util.o

ext2_ln: ext2_ln.o ext2_util.o
	gcc -Wall -g -o ext2_ln ext2_ln.o ext2_util.o

ext2_util.o: ext2_util.c ext2_util.h ext2.h
	gcc  -c ext2_util.c

ext2_mkdir.o: ext2_mkdir.c ext2.h ext2_util.h
	gcc  -c ext2_mkdir.c

ext2_checker.o: ext2_checker.c ext2.h ext2_util.h
	gcc  -c ext2_checker.c

ext2_cp: ext2_cp.o ext2_util.o
	gcc -Wall -g -o ext2_cp ext2_cp.o ext2_util.o

ext2_restore: ext2_restore.o ext2_util.o
	gcc -Wall -g -o ext2_restore ext2_restore.o ext2_util.o

ext2_rm: ext2_rm.o ext2_util.o
	gcc -Wall -g -o ext2_rm ext2_rm.o ext2_util.o

ext2_rm.o: ext2_rm.c ext2.h ext2_util.h
	gcc -g  -c ext2_rm.c

ext2_restore.o: ext2_restore.c ext2.h ext2_util.h
	gcc -g  -c ext2_restore.c

ext2_cp.o: ext2_cp.c ext2.h ext2_util.h
	gcc -g  -c ext2_cp.c

ext2_ln.o: ext2_ln.c ext2.h ext2_util.h
	gcc -g  -c ext2_ln.c

readimage: readimage.o
	gcc -Wall -g  -o readimage readimage.o

readimage.o : readimage.c ext2.h
	gcc -g  -c readimage.c

clean:
	rm *.o ext2_mkdir ext2_checker ext2_cp readimage ext2_rm ext2_restore ext2_ln
