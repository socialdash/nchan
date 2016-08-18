#ifndef NCHAN_MEMSTORE_H
#define NCHAN_MEMSTORE_H

extern nchan_store_t  nchan_store_memory;

ngx_int_t msg_reserve(nchan_msg_t *msg, char *lbl);
ngx_int_t msg_release(nchan_msg_t *msg, char *lbl);

ngx_int_t memstore_channel_owner(ngx_str_t *id);

nchan_stub_status_t *nchan_get_stub_status_stats(void);
ngx_uint_t nchan_get_used_shmem(void);

#endif //NCHAN_MEMSTORE_H
