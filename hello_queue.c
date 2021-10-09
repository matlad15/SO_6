#include "hello_queue.h"
#include <minix/drivers.h>
#include <minix/chardriver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include <minix/ioctl.h>
#include <sys/ioc_hello_queue.h>

static char* buffer;
static u32_t buffer_size = 0;
static u32_t queue_size = 0;

static ssize_t hello_queue_read(devminor_t minor, u64_t pos, endpoint_t ep,
	cp_grant_id_t gid, size_t size, int flags, cdev_id_t id);
static ssize_t hello_queue_write(devminor_t minor, u64_t pos, endpoint_t ep,
	cp_grant_id_t gid, size_t size, int flags, cdev_id_t id);
static int hello_queue_open(devminor_t minor, int access, endpoint_t user_endpt);
static int hello_queue_close(devminor_t minor);
static int hello_queue_ioctl(devminor_t minor, unsigned long request, endpoint_t ep,
	cp_grant_id_t gid, int flags, endpoint_t user_ep, cdev_id_t id);
static int do_init(u64_t size);
static int do_hqiocset(char *buf, u64_t size);
static int do_hqiocxch(char *buf);
static int do_hqiocdel();
static int restore_values();
static void sef_local_startup();
static int save_values(int UNUSED(state));
static int sef_cb_init(int type, sef_init_info_t *UNUSED(info));

static struct chardriver hello_queue_tab =
{
    .cdr_open	= hello_queue_open,
    .cdr_close	= hello_queue_close,
    .cdr_read	= hello_queue_read,
    .cdr_write  = hello_queue_write,
    .cdr_ioctl	= hello_queue_ioctl,
};

static int
do_init(u64_t size) {
    buffer = (char *)malloc((size + 1) * sizeof(char));
    if (buffer == NULL) {
        return -1;
    }

    for (u64_t i = 0; i < size; i++) {
        if (i % 3 == 0) {
            buffer[i] = 'x';
        }
        else if (i % 3 == 1) {
            buffer[i] = 'y';
        }
        else {
            buffer[i] = 'z';
        }
    }

    queue_size = size;
    buffer_size = size;

    return OK;
}

static int
do_hqiocset(char *buf, u64_t size) {
    while (buffer_size < size) {
        char *new_buffer = (char *)realloc(buffer, (buffer_size * 2 + 1) * sizeof(char));
        if (new_buffer == NULL) {
            free(buffer);
            return -1;
        }
        buffer = new_buffer;
        buffer_size *= 2;
    }

    if (queue_size < size) {
        for (u64_t i = 0; i < size; i++) {
            buffer[i] = buf[i];
        }
        queue_size = size;
    }
    else {
        int ind = size - 1;
        for (u64_t i = queue_size - 1; ind >= 0; i--) {
            buffer[i] = buf[ind];
            ind--;
        }
    }

    return OK;
}

static int
do_hqiocxch(char *buf) {
    for (u64_t i = 0; i < queue_size; i++) {
        if (buffer[i] == buf[0]) {
            buffer[i] = buf[1];
        }
    }

    return OK;
}

static int
do_hqiocdel() {
    char *new_buffer = (char *)malloc((buffer_size + 1) * sizeof(char));
    if (new_buffer == NULL) {
        free(buffer);
        return -1;
    }
    u64_t ind = 0;
    u64_t cnt = 0;
    for (u64_t i = 0; i < queue_size; i++) {
        if (i % 3 != 2) {
            new_buffer[ind++] = buffer[i];
        }
        else {
            cnt++;
        }
    }

    queue_size -= cnt;
    buffer = new_buffer;

    return OK;
}

static int save_values(int UNUSED(state)) {
    ds_publish_mem("buffer", buffer, buffer_size, DSF_OVERWRITE);
    ds_publish_u32("buffer_size", buffer_size, DSF_OVERWRITE);
    ds_publish_u32("queue_size", queue_size, DSF_OVERWRITE);

    if (buffer != NULL) {
        free(buffer);
    }

    return OK;
}

static int restore_values() {
    ds_retrieve_u32("buffer_size", &buffer_size);
    ds_delete_u32("buffer_size");
    buffer = malloc((buffer_size + 1) * sizeof(char));
    if (buffer == NULL) {
        return -1;
    }
    ds_retrieve_mem("buffer", buffer, &buffer_size);
    ds_delete_u32("buffer");
    ds_retrieve_u32("queue_size", &queue_size);
    ds_delete_u32("queue_size");

    return OK;

}

static void sef_local_startup()
{
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    sef_setcb_lu_state_save(save_values);

    sef_startup();
}

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
    int do_announce_driver = TRUE;

    switch(type) {
        case SEF_INIT_FRESH:
            do_init(DEVICE_SIZE);
        break;

        case SEF_INIT_LU:
            restore_values();
            do_announce_driver = FALSE;
        break;

        case SEF_INIT_RESTART:
            restore_values();
            do_announce_driver = FALSE;
        break;
    }

    if (do_announce_driver) {
        chardriver_announce();
    }

    return OK;
}

static int
hello_queue_ioctl(devminor_t minor, unsigned long request, endpoint_t ep,
	cp_grant_id_t gid, int UNUSED(flags), endpoint_t UNUSED(user_ep),
	cdev_id_t UNUSED(id))
{
	int r;

	switch(request) {
	case HQIOCRES:
        {
            if (buffer != NULL) {
                free(buffer);
            }
            r = do_init(DEVICE_SIZE);
            return r;
            break;
        }
	case HQIOCSET:
        {
            char buf[MSG_SIZE];
            r = sys_safecopyfrom(ep, gid, 0, (vir_bytes)buf, MSG_SIZE);
            if (r == OK) {
                r = do_hqiocset(buf, MSG_SIZE);
            }
            return r;
            break;
        }
	case HQIOCXCH:
        {
            char buf[2];
            r = sys_safecopyfrom(ep, gid, 0, (vir_bytes)buf, 2);
            if (r == OK) {
                r = do_hqiocxch(buf);
            }
            return r;
            break;
        }
	case HQIOCDEL:
		{
            r = do_hqiocdel();
            return r;
            break;
        }
	}

	return ENOTTY;
}

static int
hello_queue_open(devminor_t minor, int UNUSED(access), endpoint_t UNUSED(user_endpt))
{
    return OK;
}

static int
hello_queue_close(devminor_t minor)
{
	return OK;
}

static ssize_t hello_queue_read(devminor_t UNUSED(minor), u64_t position,
    endpoint_t endpt, cp_grant_id_t grant, size_t size, int UNUSED(flags),
    cdev_id_t UNUSED(id))
{
    int r;

    if (size > queue_size) {
        size = queue_size;
    }

    if (size == 0) {
        return 0;
    }

    if (buffer == NULL) {
        return -1;
    }

    r = sys_safecopyto(endpt, grant, 0, (vir_bytes)buffer, size);
    if (r == OK) {
        char *new_buffer = (char *)malloc((buffer_size + 1) * sizeof(char));
        if (new_buffer == NULL) {
            free(buffer);
            return -1;
        }
        u64_t ind = 0;
        for (u64_t i = size; i < queue_size; i++) {
            new_buffer[ind++] = buffer[i];
        }
        buffer = new_buffer;

        queue_size -= size;

        if (buffer_size / 4 >= queue_size) {
            buffer_size /= 2;
            char *n_buffer = (char *)realloc(buffer, (buffer_size + 1) * sizeof(char));
            if (n_buffer == NULL) {
                free(buffer);
                return -1;
            }
            buffer = n_buffer;
        }
    }
    else {
        return r;
    }

    return size;
}


static ssize_t hello_queue_write(devminor_t UNUSED(minor), u64_t position,
    endpoint_t endpt, cp_grant_id_t grant, size_t size, int UNUSED(flags),
    cdev_id_t UNUSED(id))
{
    int r;

    if (size == 0) {
        return 0;
    }

    if (buffer == NULL) {
        return -1;
    }

    char buf[size];
	r = sys_safecopyfrom(endpt, grant, 0, (vir_bytes)buf, size);
    if (r != OK) {
        return r;
    }

    while (queue_size + size > buffer_size) {
        char *new_buffer = (char *)realloc(buffer, (buffer_size * 2 + 1) * sizeof(char));
        if (new_buffer == NULL) {
            free(buffer);
            return -1;
        }
        buffer = new_buffer;
        buffer_size *= 2;
    }

    u64_t ind = 0;
    for (u64_t i = queue_size; ind < size; i++) {
        buffer[i] = buf[ind++];
    }

    queue_size += size;

    return size;

}



int main(void)
{
    sef_local_startup();

    chardriver_task(&hello_queue_tab);
    return OK;
}