#include <node_api.h>

#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>

namespace shm{

  napi_value create(napi_env env, napi_callback_info args) {
    int size;
    napi_value fd_value;
    napi_status status;

    
    napi_get_value_int32(env, args[0], &size);

    int fd = syscall(SYS_memfd_create, "buffer", 0);
    status = napi_create_int32(env, fd, &fd_value);
    if (status != napi_ok) return nullptr;

    ftruncate(fd, size);


    return fd_value;
  }

  napi_value map(napi_env env, napi_callback_info args){
    napi_status status;
    int fd, size;

    napi_get_value_int32(env, args[0], &fd);
    napi_get_value_int32(env, args[1], &size);

    unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    status = napi_create_external_buffer(napi_env env,
                                        size,
                                        data,
                                        nullptr, //FIXME: finalize callback
                                        nullptr,
                                        result);
    if (status != napi_ok) return nullptr;
    return result;
  }


  napi_value init(napi_env env, napi_value exports) {
    napi_status status;
    napi_value create_fn;
    napi_value map_fn;

    status = napi_create_function(env, nullptr, 0, create, nullptr, &create_fn);
    if (status != napi_ok) return nullptr;

    status = napi_set_named_property(env, exports, "create", create_fn);
    if (status != napi_ok) return nullptr;

    status = napi_create_function(env, nullptr, 0, create, nullptr, &map_fn);
    if (status != napi_ok) return nullptr;

    status = napi_set_named_property(env, exports, "map", map_fn);
    if (status != napi_ok) return nullptr;

    return exports;
  }

  NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
}
