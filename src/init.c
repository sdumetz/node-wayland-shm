#define _GNU_SOURCE
#include <node_api.h>
#include <assert.h>

#include "shm.c"
#include "send_fd.c"

napi_value init(napi_env env, napi_value exports) {
  napi_status status;

  napi_value handle;
  status = napi_create_object(env, &handle);
  assert(status == napi_ok);


  napi_value create_fn;

  status = napi_create_function(env, "create_shm", NAPI_AUTO_LENGTH, Create, NULL, &create_fn);
  assert(status == napi_ok);

  status = napi_set_named_property(env, handle, "create_shm", create_fn);
  assert(status == napi_ok);


  napi_value send_fd;

  status = napi_create_function(env, "send_fd", NAPI_AUTO_LENGTH, Send, NULL, &send_fd);
  assert(status == napi_ok);

  status = napi_set_named_property(env, handle, "send_fd", send_fd);
  assert(status == napi_ok);


  return handle;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
