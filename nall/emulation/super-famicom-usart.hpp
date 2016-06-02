#pragma once

#include <nall/nall.hpp>
#include <nall/serial.hpp>
using namespace nall;

#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>

static function<auto () -> bool> usart_quit;
static function<auto (uint microseconds) -> void> usart_usleep;
static function<auto () -> bool> usart_readable;
static function<auto () -> uint8> usart_read;
static function<auto () -> bool> usart_writable;
static function<auto (uint8 data) -> void> usart_write;

extern "C" auto usart_init(
  function<auto () -> bool> quit,
  function<auto (uint microseconds) -> void> usleep,
  function<auto () -> bool> readable,
  function<auto () -> uint8> read,
  function<auto () -> bool> writable,
  function<auto (uint8 data) -> void> write
) -> void {
  usart_quit = quit;
  usart_usleep = usleep;
  usart_readable = readable;
  usart_read = read;
  usart_writable = writable;
  usart_write = write;
}

extern "C" auto usart_main(nall::lstring) -> void;

//

static serial usart;
static bool usart_is_virtual = true;
static bool usart_sigint = false;

static auto usart_virtual() -> bool {
  return usart_is_virtual;
}

//

static auto usarthw_quit() -> bool {
  return usart_sigint;
}

static auto usarthw_usleep(uint microseconds) -> void {
  usleep(microseconds);
}

static auto usarthw_readable() -> bool {
  return usart.readable();
}

static auto usarthw_read() -> uint8 {
  while(true) {
    uint8 buffer[1];
    int length = usart.read((uint8_t*)&buffer, 1);
    if(length > 0) return buffer[0];
  }
}

static auto usarthw_writable() -> bool {
  return usart.writable();
}

static auto usarthw_write(uint8 data) -> void {
  uint8 buffer[1] = {data};
  usart.write((uint8*)&buffer, 1);
}

static auto sigint(int) -> void {
  signal(SIGINT, SIG_DFL);
  usart_sigint = true;
}

#include <nall/main.hpp>
auto nall::main(lstring args) -> void {
  setpriority(PRIO_PROCESS, 0, -20);  //requires superuser privileges; otherwise priority = +0
  signal(SIGINT, sigint);

  if(!usart.open("/dev/ttyACM0", 57600, true)) {
    return print("error: unable to open USART hardware device\n");
  }

  usart_is_virtual = false;
  usart_init(usarthw_quit, usarthw_usleep, usarthw_readable, usarthw_read, usarthw_writable, usarthw_write);
  usart_main(args);
  usart.close();
}
