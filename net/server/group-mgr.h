/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef GROUP_MGR_H
#define GROUP_MGR_H

#include "../common/session.h"

/* #define MAX_GROUP_MEMBERS	16 */

typedef struct _CcnetGroupManager CcnetGroupManager;
typedef struct _CcnetGroupManagerPriv CcnetGroupManagerPriv;

struct _CcnetGroupManager
{
    CcnetSession	*session;
    
    CcnetGroupManagerPriv	*priv;
};

CcnetGroupManager* ccnet_group_manager_new (CcnetSession *session);

int
ccnet_group_manager_prepare (CcnetGroupManager *manager);

void ccnet_group_manager_start (CcnetGroupManager *manager);

int ccnet_group_manager_create_group (CcnetGroupManager *mgr,
                                      const char *group_name,
                                      const char *user_name,
                                      GError **error);

int ccnet_group_manager_create_org_group (CcnetGroupManager *mgr,
                                          int org_id,
                                          const char *group_name,
                                          const char *user_name,
                                          GError **error);

int ccnet_group_manager_remove_group (CcnetGroupManager *mgr,
                                      int group_id,
                                      GError **error);

int ccnet_group_manager_add_member (CcnetGroupManager *mgr,
                                    int group_id,
                                    const char *user_name,
                                    const char *member_name,
                                    GError **error);

int ccnet_group_manager_remove_member (CcnetGroupManager *mgr,
                                       int group_id,
                                       const char *user_name,
                                       const char *member_name,
                                       GError **error);

int ccnet_group_manager_set_admin (CcnetGroupManager *mgr,
                                   int group_id,
                                   const char *member_name,
                                   GError **error);

int ccnet_group_manager_unset_admin (CcnetGroupManager *mgr,
                                     int group_id,
                                     const char *member_name,
                                     GError **error);

int ccnet_group_manager_set_group_name (CcnetGroupManager *mgr,
                                        int group_id,
                                        const char *group_name,
                                        GError **error);

int ccnet_group_manager_quit_group (CcnetGroupManager *mgr,
                                    int group_id,
                                    const char *user_name,
                                    GError **error);

GList *
ccnet_group_manager_get_groupids_by_user (CcnetGroupManager *mgr,
                                          const char *user_name,
                                          GError **error);

CcnetGroup *
ccnet_group_manager_get_group (CcnetGroupManager *mgr, int group_id,
                               GError **error);

GList *
ccnet_group_manager_get_group_members (CcnetGroupManager *mgr, int group_id,
                                       GError **error);

int
ccnet_group_manager_check_group_staff (CcnetGroupManager *mgr,
                                       int group_id,
                                       const char *user_name);

int
ccnet_group_manager_remove_group_user (CcnetGroupManager *mgr,
                                       const char *user);

int
ccnet_group_manager_is_group_user (CcnetGroupManager *mgr,
                                   int group_id,
                                   const char *user);

GList*
ccnet_group_manager_get_all_groups (CcnetGroupManager *mgr,
                                    int start, int limit, GError **error);

int
ccnet_group_manager_set_group_creator (CcnetGroupManager *mgr,
                                       int group_id,
                                       const char *user_name);

#endif /* GROUP_MGR_H */

