'use strict';
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

//@ts-ignore
const addon = require('./build/Release/shm');

export const create_shm = addon.create_shm;

export const send_fd = addon.send_fd;