# pam_slurm_drained

A pam module for letting users access a slurm node only when that node is
drained. E.g. for giving access for administrative works (when reservation is
not relevant).

This module won't through people out when the node is resumed.

In order to compile, slurm source (or headers) and libraries needs to be
available. If slurm was compiled manually or installed in a non standard
location, the following variables might need to be set before compiling

`C_INCLUDE_PATH=/path/to/builddir:/path/to/srcdir/src`
`LIBRARY_PATH=/path/to/slurmlibdir/`
`LD_RUN_PATH=/path/to/slurmlibdir/`
