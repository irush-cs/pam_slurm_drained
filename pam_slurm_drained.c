/******************************************************************************
 *
 *   pam_slurm_drained.c
 *
 *   Copyright (C) 2024 Hebrew University of Jerusalem Israel, see COPYING
 *   file.
 *
 *   Author: Yair Yarom <irush@cs.huji.ac.il>
 *
 *   This program is free software: you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation, either version 3 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include <syslog.h>
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <slurm/slurm.h>

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    int rc = 0;
    int result = PAM_SUCCESS;
    node_info_msg_t *node_info = NULL;
    uint32_t state;
    char hostname[256];
    hostname[sizeof(hostname)-1] = 0;

    if (gethostname(hostname, sizeof(hostname) - 1) != 0) {
        pam_syslog(pamh, LOG_ERR, "Failed to get hostname");
        return PAM_SYSTEM_ERR;
    }

    rc = slurm_load_node_single(&node_info, hostname, SHOW_ALL);
    if (rc != SLURM_SUCCESS) {
        pam_syslog(pamh, LOG_ERR, "Failed to load node information from Slurm: %s (%i)", slurm_strerror(rc), rc);
        return PAM_SYSTEM_ERR;
    }

    if (!node_info) {
        pam_syslog(pamh, LOG_ERR, "Can't get node info");
        result = PAM_SYSTEM_ERR;
        goto cleanup;
    }

    if (node_info->record_count == 0) {
        pam_syslog(pamh, LOG_ERR, "Can't find %s in cluster", hostname);
        result = PAM_SYSTEM_ERR;
        goto cleanup;
    }

    state = node_info->node_array[0].node_state;

    if (!(state & NODE_STATE_DRAIN)) {
        pam_syslog(pamh, LOG_NOTICE, "Node %s is not draining", hostname);
        result = PAM_PERM_DENIED;
        goto cleanup;
    }

    if (((state & NODE_STATE_BASE) == NODE_STATE_ALLOCATED) ||
        ((state & NODE_STATE_BASE) == NODE_STATE_MIXED)) {
        pam_syslog(pamh, LOG_NOTICE, "Host %s is not drained yet", hostname);
        result = PAM_PERM_DENIED;
        goto cleanup;
    }

 cleanup:
    slurm_free_node_info_msg(node_info);

    return result;
}