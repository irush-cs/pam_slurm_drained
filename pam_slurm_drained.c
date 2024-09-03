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
#include <string.h>
#include <src/common/read_config.h>

static struct {
    char *slurm_conf;
    int ignore_root;
} opts;

static int _converse(pam_handle_t *pamh, char *message) {
    struct pam_conv *conv_info;
    int rc = PAM_SUCCESS;
    struct pam_message *msg[1], rmsg[1];
    struct pam_response *resp;

    rc = pam_get_item(pamh, PAM_CONV, (void*) &conv_info);
    if (rc != PAM_SUCCESS) {
	pam_syslog(pamh, LOG_ERR, "unable to get the PAM conversation struct");
        return rc;
    }

    msg[0] = &rmsg[0];
    rmsg[0].msg_style = PAM_ERROR_MSG;
    rmsg[0].msg = message;

    rc = conv_info->conv(1,
                         (const struct pam_message**)msg,
                         &resp,
                         conv_info->appdata_ptr
                         );

    return rc;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    int rc = 0;
    int slurm_conf_inited = 0;
    int result = PAM_SUCCESS;
    node_info_msg_t *node_info = NULL;
    uint32_t state;
    char hostname[256];
    char *user_name = NULL;
    char buffer[1024];
    int i;

    buffer[sizeof(buffer)-1] = 0;

    opts.slurm_conf = NULL;
    opts.ignore_root = 1;

    for (i = 0; i < argc; i++) {
        if (strncasecmp(argv[i], "slurm_conf=", 11) == 0) {
            opts.slurm_conf = strdup((argv[i] + 11));
        } else if (strncasecmp(argv[i], "ignore_root=", 12) == 0 &&
                   (argv[i][12] == '0') &&
                   argv[i][13] == 0) {
            opts.ignore_root = 0;
        } else {
            pam_syslog(pamh, LOG_WARNING, "Warning: Unknown option: %s", argv[i]);
        }
    }

    rc = pam_get_item(pamh, PAM_USER, (void*)&user_name);
    if (user_name == NULL || rc != PAM_SUCCESS)  {
        pam_syslog(pamh, LOG_WARNING, "Can't get username");
        user_name = NULL;
    }
    if (user_name && strcmp("root", user_name) == 0 && opts.ignore_root) {
        pam_syslog(pamh, LOG_INFO, "Ignoring root user");
        result = PAM_IGNORE;
        goto cleanup;
    }

    rc = slurm_conf_init(opts.slurm_conf);
    if (rc == SLURM_SUCCESS) {
        slurm_conf_inited = 1;
    } else {
        pam_syslog(pamh, LOG_WARNING, "Warning: slurm.conf already loaded, might be from different file");
    }

    hostname[sizeof(hostname)-1] = 0;

    if (gethostname(hostname, sizeof(hostname) - 1) != 0) {
        pam_syslog(pamh, LOG_ERR, "Failed to get hostname");
        result = PAM_SYSTEM_ERR;
        goto cleanup;
    }

    rc = slurm_load_node_single(&node_info, hostname, SHOW_ALL);
    if (rc != SLURM_SUCCESS) {
        pam_syslog(pamh, LOG_ERR, "Failed to load node information from Slurm: %s (%i)", slurm_strerror(rc), rc);
        result = PAM_SYSTEM_ERR;
        goto cleanup;
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
        snprintf(buffer, sizeof(buffer) - 1, "%s is not draining", hostname);
        pam_syslog(pamh, LOG_NOTICE, buffer);
        if (!(flags & PAM_SILENT)) {
            snprintf(buffer, sizeof(buffer) - 1, "Access denied by pam_slurm_drained: %s is not draining", hostname);
            _converse(pamh, buffer);
        }
        result = PAM_PERM_DENIED;
        goto cleanup;
    }

    if (((state & NODE_STATE_BASE) == NODE_STATE_ALLOCATED) ||
        ((state & NODE_STATE_BASE) == NODE_STATE_MIXED)) {
        snprintf(buffer, sizeof(buffer) - 1, "%s is not drained yet", hostname);
        pam_syslog(pamh, LOG_NOTICE, buffer);
        if (!(flags & PAM_SILENT)) {
            snprintf(buffer, sizeof(buffer) - 1, "Access denied by pam_slurm_drained: %s is not drained yet", hostname);
            _converse(pamh, buffer);
        }
        result = PAM_PERM_DENIED;
        goto cleanup;
    }

    pam_syslog(pamh, LOG_INFO, "Node drained, allowing access");

 cleanup:
    if (opts.slurm_conf) {
        free(opts.slurm_conf);
    }
    if (node_info) {
        slurm_free_node_info_msg(node_info);
    }
    if (slurm_conf_inited) {
        slurm_conf_destroy();
    }

    return result;
}
