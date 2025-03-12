#include <node_api.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>


napi_value Send(napi_env env, napi_callback_info info) {
  napi_status status;

  size_t argc = 3;
  napi_value args[3];
  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);
  
  int sfd;
  status = napi_get_value_int32(env, args[0], &sfd);
  assert(status == napi_ok);

  void* data;
  size_t data_length;
  status = napi_get_buffer_info(env, args[1], &data, &data_length);
  assert(status == napi_ok);


  int fd;
  status = napi_get_value_int32(env, args[2], &fd);
  assert(status == napi_ok);

  //This is where we actually do stuff

  //Vector IO to send data. Could be an array if needed but data length is generally short enough
  struct iovec iov;
  iov.iov_base = data;
  iov.iov_len = data_length;

  //Message Control (ancillary data)
  union {
    char   buf[CMSG_SPACE(sizeof(int))]; /* large enough to hold the file descriptor */
    struct cmsghdr align;
  } cmsg;
  //If we ever want to use CMSG_NXTHDR(), we need to memset buf to 0

  struct msghdr msgh = {
    .msg_name = NULL,       /* optional address */
    .msg_namelen = 0,       /* size of address */
    .msg_iov = &iov,          /* scatter/gather array */
    .msg_iovlen = 1,        /* # elements in msg_iov */
    .msg_control =cmsg.buf, /* ancillary data, see below */
    .msg_controllen = sizeof(cmsg.buf), /* ancillary data buffer len */
    .msg_flags = 0,         /* flags on received message */
  };

  //set control data with somewhat convoluted macrosœ
  struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
  cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
  cmsgp->cmsg_level = SOL_SOCKET;
  cmsgp->cmsg_type = SCM_RIGHTS;
  memcpy(CMSG_DATA(cmsgp), &fd, sizeof(int));


  ssize_t char_len = sendmsg(sfd, &msgh, 0);
  if(char_len < 0){
    napi_throw_error(env, strerrorname_np(errno), strerror(errno));
    assert(status == napi_ok);
  }

  napi_value result;
  status = napi_create_uint32(env, char_len, &result);
  assert(status == napi_ok);

  return result;
}
