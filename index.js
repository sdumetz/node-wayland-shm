'use strict';
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

const shm = require('./build/Release/shm');

console.log("shm", shm);