
pam_slurm_drained.so: pam_slurm_drained.c Makefile helper.c helper.h
	gcc -fPIC -shared -Wall -o pam_slurm_drained.so pam_slurm_drained.c helper.c -lslurm -lpam

clean:
	rm pam_slurm_drained.so

