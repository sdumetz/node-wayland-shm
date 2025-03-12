
#include <node_api.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <fcntl.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>


void throw(napi_env env, int err){
  const char* code = strerrorname_np(err);
  if(code == NULL){
    code = "EINVAL";
  }
  napi_status status = napi_throw_error(env, code, strerrordesc_np(err));
  assert(status == napi_ok);
}

napi_value Close(napi_env env, napi_callback_info info){
  napi_status status;
  int fd;
  status = napi_get_cb_info(env, info, NULL, NULL, NULL, (void *) &fd);
  assert(status == napi_ok);
  int ret = close(fd);
  assert(ret == 0);
  return NULL;
}


napi_value Unmap(napi_env env, napi_callback_info info){
  napi_status status;

  napi_value thisArg;

  status = napi_get_cb_info(env, info, NULL, NULL, &thisArg, NULL);
  assert(status == napi_ok);



  napi_value buffer;
  status = napi_get_named_property(env, thisArg, "data", &buffer);
  assert(status == napi_ok);

  void* data;
  size_t data_length;
  status = napi_get_buffer_info(env, buffer, &data, &data_length);
  assert(status == napi_ok);

  int ret = munmap(data, data_length);
  if(ret < 0){
    throw(env, errno);
  }
  return NULL;
}

/* Shared memory support code */
static void
randname(char *buf)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
}

static int
create_shm_file(void)
{
    int retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

static int
allocate_shm_file(size_t size)
{
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

napi_value Create(napi_env env, napi_callback_info info) {
  napi_status status;
  int ret;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);
  

  int size;
  status = napi_get_value_int32(env, args[0], &size);
  assert(status == napi_ok);

  napi_value handle;
  status = napi_create_object(env, &handle);
  assert(status == napi_ok);
  //This is where we actually do stuff

  int fd = allocate_shm_file(size);
  if(fd == -1){
    throw(env, errno);
    return NULL;
  }
  unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
      close(fd);
      throw(env, errno);
      return NULL;
  }

  for(int i=0; i < size; i++){
    if ((i / 8 * 8) % 16 < 8)
        data[i] = 0xFF666666;
    else
        data[i] = 0xFFEEEEEE;
  }
  if(ret < 0){
    throw(env, errno);
    return NULL;
  }
  //Set up our return structure

  //The file descriptor
  napi_value fd_value;
  status = napi_create_int32(env, fd, &fd_value);
  assert(status == napi_ok);

  status = napi_set_named_property(env, handle, "fd", fd_value);
  assert(status == napi_ok);

  //the shared-memory-backed data view
  napi_value ab;
  status = napi_create_external_arraybuffer(env, data, size, NULL, NULL, &ab);
  assert(status == napi_ok);

  napi_value buffer;
  status = napi_create_dataview(env, size, ab, 0, &buffer);
  assert(status == napi_ok);

  status = napi_set_named_property(env, handle, "data", buffer);
  assert(status == napi_ok);

  //the close method
  napi_value close_fn;
  status = napi_create_function(env, "close", NAPI_AUTO_LENGTH, Close, (void *) (long)fd, &close_fn);
  assert(status == napi_ok);
  status = napi_set_named_property(env, handle, "close", close_fn);
  assert(status == napi_ok);

  napi_value unmap_fn;
  status = napi_create_function(env, "unmap", NAPI_AUTO_LENGTH, Unmap, NULL, &unmap_fn);
  assert(status == napi_ok);
  status = napi_set_named_property(env, handle, "unmap", unmap_fn);

  return handle;
}

