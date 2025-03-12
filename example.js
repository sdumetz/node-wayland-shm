'use strict';
import path from "path";
import {once} from "events";
import {connect} from "net";
import timers from "timers/promises";

import {Display} from "wayland-client";

import { format_args, writeUInt } from "wayland-client/dist/args.js";

import {send_fd, create_shm} from "./index.js";



async function open_display(sPath = process.env["XDG_RUNTIME_DIR"]?path.join(process.env["XDG_RUNTIME_DIR"], process.env["WAYLAND_DISPLAY"] ?? "wayland-0"):"" ) {
  if(!sPath) throw new Error("no socket path provided and XDG_RUNTIME_DIR not set");
  
  const s = connect(sPath);
  await once(s, "connect");
  const display = new Display(s);

  await display.init();

  const original_request = display.request;
  display.request = async function interceptedRequest(srcId, opcode, def, ...args){
    const fd_idx = def.args.findIndex(arg=>arg.type === "fd");
    if(fd_idx === -1) return await original_request.apply(display, [srcId, opcode, def, ...args]);
    const b1 = Buffer.alloc(8);
    const b2 = format_args(args, def.args);

    writeUInt(b1, srcId, 0);
    //16 most significant bits are the message length. 16 next bits are the message opcode
    writeUInt(b1, (b1.length + b2.length) << 16 | opcode & 0xFFFF, 4);
    //@ts-ignore
    send_fd(s._handle.fd, Buffer.concat([b1,b2]), args[fd_idx]);
  }
  return display;
}



/**
 * 
 * @param {object} param0
 * @param {number} param0.width
 * @param {number} param0.height
 * @param {number} [param0.x = 0]
 * @param {number} [param0.y = 0]
 * @param {string} [param0.format = "xrgb8888"]
 * @param {"argb8888"|"xrgb8888"} [param0.socket]
 */
export default async function open({width, height, x=0, y=0, format="xrgb8888", socket}){
    const size = (width-x)*(height-y)*4;
    const display = await open_display(socket);
    display.on("error", (e)=> {
      console.warn("FATAL Wayland display error :", e.message);
      process.exit(1);
    });
    display.on("warning", console.warn);

    await display.load(path.join(import.meta.dirname, "protocols/stable/xdg-shell/xdg-shell.json"));
    


    /** @type {import("wayland-client/protocol/wayland.d.ts").Wl_compositor} */
    //@ts-ignore
    const compositor = await display.bind("wl_compositor");
    /** @type {import("wayland-client/protocol/wayland.d.ts").Wl_shm} */
    //@ts-ignore
    const shm = await display.bind("wl_shm");


    /** @type {Record<string,number>} */
    let fmap ={};
    shm.on("format", (value)=>{
        const fmt = shm.enums.format.find(f=>f.value == value);
        if (fmt){
          //console.log("Supported pixel format : ", fmt.name);
          fmap[fmt.name] = fmt.value;
        }
    });
    await display.sync();


   

    console.log("Binding to xdg_wm_base");
    /** @type {import("./protocols/stable/xdg-shell/xdg-shell.d.ts").Xdg_wm_base} */
    const xdg_wm = await display.bind("xdg_wm_base");
    xdg_wm.on("ping", (serial)=>{
      console.log("PING");
      xdg_wm.pong(serial);
    });


    if(typeof fmap[format] === "undefined"){
      throw new Error(`Pixel format ${format} is not supported by the wayland backend. Use one of ${Object.keys(fmap).join(",")}`);
    }

    const surface = await compositor.create_surface();

    const xdg_surface = await xdg_wm.get_xdg_surface(surface);
    xdg_surface.on("configure", async (serial)=>{
      console.log("Configure surface");
      await xdg_surface.ack_configure(serial);
      const mmap = create_shm(size);
    
      const pool = await shm.create_pool(mmap.fd, size);
      const wl_buffer = await pool.create_buffer(0, width, height, width*4, fmap[format]);
      wl_buffer.on("release", ()=>{
        console.log("releasing buffer");
        wl_buffer.destroy();
      });
      await pool.destroy();
      mmap.close(); //We don't need the file descriptor anymore : it has been transfered to the compositor
      console.log("Dataview :", mmap.data.byteLength, mmap.data.byteOffset);
      for(let dy = 0; dy < height; dy++){
        for(let dx = 0; dx < width; dx++){
          mmap.data.setUint32((dx+dy*width)*4, 0xFF000000 + (Math.floor(254*dy/height)<<8) + Math.floor(254*dx/width) , true );
        }
      }
      await surface.attach(wl_buffer, 0, 0);
      await surface.commit();
    });
    const toplevel = await xdg_surface.get_toplevel();
    console.log('Created toplevel');
    toplevel.on("configure", (...args)=>{
      console.log("toplevel configure", args);
    });
    await toplevel.set_title("Test window title");

    await surface.damage(x, y, width, height);
    await surface.commit();

    await timers.setTimeout(4000);
    await xdg_surface.destroy();
    await xdg_wm.destroy();
    display.close();
    
}

open({width: 600, height: 400});