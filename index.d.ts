
export interface Mmapped {
  fd: number;
  data: DataView;
  close: ()=>void;
  unmap: ()=>void;
}

export function create_shm(size: number):Mmapped;


export function send_fd(sockfd: number, data:Buffer, fd: number):number;