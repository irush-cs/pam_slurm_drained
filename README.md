# pam_slurm_drained

A pam module for letting users access a slurm node only when that node is
drained. E.g. for giving access for administrative works (when reservation is
not relevant).

This module won't throw people out when the node is resumed.

## Compiling

Run:
`make`
to compile, and copy `pam_slurm_drained.so` to `/lib/security/` (or somewhere
else and specify the full path).

In order to compile, slurm source (or headers) and libraries needs to be
available. If slurm was compiled manually or installed in a non standard
location, the following variables might need to be set before compiling

`C_INCLUDE_PATH=/path/to/builddir:/path/to/srcdir/src`
`LIBRARY_PATH=/path/to/slurmlibdir/`
`LD_RUN_PATH=/path/to/slurmlibdir/`

## Options

### slurm_conf

`slurm_conf=/path/to/slurm.conf`

Specify a non-default slurm.conf to use

### ignore_root

`ignore_root=0`

By default root is allowed even if not drained. Setting to 0 checks drained
also for root.


