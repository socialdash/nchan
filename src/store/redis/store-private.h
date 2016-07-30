#ifndef NCHAN_REDIS_STORE_PRIVATE_H
#define NCHAN_REDIS_STORE_PRIVATE_H

#define NCHAN_CHANHEAD_EXPIRE_SEC 1
#define NCHAN_CHANHEAD_CLUSTER_ORPHAN_EXPIRE_SEC 15

#include <nchan_module.h>
#include "uthash.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include <util/nchan_reaper.h>
#include <util/nchan_rbtree.h>
#include <util/nchan_list.h>
#include <store/spool.h>

typedef struct rdstore_data_s rdstore_data_t;
typedef struct rdstore_channel_head_s rdstore_channel_head_t;

typedef struct {
  rdstore_data_t          *node_rdt;
  unsigned                 enabled:1;
} rdstore_channel_head_cluster_data_t;


struct rdstore_channel_head_s {
  ngx_str_t                    id; //channel id
  channel_spooler_t            spooler;
  ngx_uint_t                   generation; //subscriber pool generation.
  chanhead_pubsub_status_t     status;
  ngx_uint_t                   sub_count;
  ngx_int_t                    fetching_message_count;
  ngx_uint_t                   internal_sub_count;
  ngx_event_t                  keepalive_timer;
  nchan_msg_id_t               last_msgid;
  
  void                        *redis_subscriber_privdata;
  rdstore_data_t              *rdt;
  rdstore_channel_head_cluster_data_t cluster;
  
  rdstore_channel_head_t      *gc_prev;
  rdstore_channel_head_t      *gc_next;
  
  rdstore_channel_head_t      *rd_next;
  rdstore_channel_head_t      *rd_prev;
  
  time_t                       gc_time;
  unsigned                     in_gc_queue:1;
  
  unsigned                     pubsub_subscribed:1;
  unsigned                     meta:1;
  unsigned                     shutting_down:1;
  UT_hash_handle               hh;
};

typedef struct {
  ngx_str_t     host;
  ngx_int_t     port;
  ngx_str_t     password;
  ngx_int_t     db;
} redis_connect_params_t;

typedef struct callback_chain_s callback_chain_t;
struct callback_chain_s {
  callback_pt                  cb;
  void                        *pd;
  callback_chain_t            *next;
};

typedef enum {DISCONNECTED, CONNECTING, AUTHENTICATING, LOADING, LOADING_SCRIPTS, CONNECTED} redis_connection_status_t;

typedef enum {CLUSTER_DISCONNECTED, CLUSTER_CONNECTING, CLUSTER_NOTREADY, CLUSTER_READY} redis_cluster_status_t;

typedef struct {
  redis_cluster_status_t           status;
  
  rbtree_seed_t                    hashslots; //cluster rbtree seed
  nchan_list_t                     nodes; //cluster rbtree seed
  nchan_list_t                     inactive_nodes; //cluster rbtree seed
  ngx_uint_t                       size; //number of master nodes
  ngx_uint_t                       nodes_connected; //number of master nodes
  ngx_int_t                        node_connections_pending; //number of master nodes
  uint32_t                         homebrew_id;
  ngx_http_upstream_srv_conf_t    *uscf;
  ngx_pool_t                      *pool;
  
  nchan_reaper_t                   chanhead_reaper;
  rdstore_channel_head_t          *orphan_channels_head;
} redis_cluster_t;

typedef struct {
  ngx_str_t         id;
  ngx_str_t         address;
  ngx_str_t         slots;
  redis_cluster_t  *cluster;
  unsigned          failed:1;
  unsigned          inactive:1;
} redis_cluster_node_t;

typedef struct {
  unsigned         min:16;
  unsigned         max:16;
} redis_cluster_slot_range_t;

typedef struct {
  redis_cluster_slot_range_t   range;
  rdstore_data_t              *rdata;
} redis_cluster_keyslot_range_node_t;

struct rdstore_data_s {
  ngx_str_t                       *connect_url;
  redis_connect_params_t           connect_params;
  
  redisAsyncContext               *ctx;
  redisAsyncContext               *sub_ctx;
  redisContext                    *sync_ctx;
  
  nchan_reaper_t                   chanhead_reaper;
  
  redis_connection_status_t        status;
  int                              scripts_loaded_count;
  int                              generation;
  ngx_event_t                      reconnect_timer;
  ngx_event_t                      ping_timer;
  time_t                           ping_interval;
  callback_chain_t                *on_connected;
  nchan_loc_conf_t                *lcf;
  
  //cluster stuff
  redis_cluster_node_t             node;
  rdstore_channel_head_t          *channels_head;
  
  unsigned                         shutting_down:1;
}; // rdstore_data_t


rbtree_seed_t              redis_data_tree;

redis_connection_status_t redis_connection_status(nchan_loc_conf_t *cf);
void redisCheckErrorCallback(redisAsyncContext *c, void *r, void *privdata);
ngx_int_t redis_add_connection_data(nchan_redis_conf_t *rcf, nchan_loc_conf_t *lcf, ngx_str_t *override_url);
rdstore_data_t *redis_create_rdata(ngx_str_t *url, redis_connect_params_t *rcp, nchan_redis_conf_t *rcf, nchan_loc_conf_t *lcf);
ngx_int_t redis_ensure_connected(rdstore_data_t *rdata);
ngx_int_t parse_redis_url(ngx_str_t *url, redis_connect_params_t *rcp);
ngx_int_t rdstore_initialize_chanhead_reaper(nchan_reaper_t *reaper, char *name);

ngx_int_t redis_chanhead_gc_add(rdstore_channel_head_t *head, ngx_int_t expire, const char *reason);
ngx_int_t redis_chanhead_gc_add_to_reaper(nchan_reaper_t *, rdstore_channel_head_t *head, ngx_int_t expire, const char *reason);
ngx_int_t redis_chanhead_gc_withdraw(rdstore_channel_head_t *head);
ngx_int_t redis_chanhead_gc_withdraw_from_reaper(nchan_reaper_t *, rdstore_channel_head_t *head);
#endif //NCHAN_REDIS_STORE_PRIVATE_H
