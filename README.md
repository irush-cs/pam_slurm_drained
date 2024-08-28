# pam_slurm_drained

A pam module for letting users access a slurm node only when that node is
drained. E.g. for giving access for administrative works (when reservation is
not relevant).

This module won't through people out when the node is resumed.

