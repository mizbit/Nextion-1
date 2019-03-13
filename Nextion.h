#ifndef NEXTION_H
#ifndef INEXTION_H

#define INEXTION_H
#define NEXTION_H

#include "SoftwareSerial.h"

#define NEXTION_SERIAL_CYCLES 255

#define  NEXTION_EVENT_RELEASE 0
#define  NEXTION_EVENT_PRESS 1

#define NEXTION_CMD_STARTUP                                     0x00 // LISTEN
#define NEXTION_CMD_SERIAL_BUFFER_OVERFLOW                      0x24 // LISTEN
#define NEXTION_CMD_TOUCH_EVENT                                 0x65 // LISTEN
#define NEXTION_CMD_CURRENT_PAGE                                0x66 // PAGE
#define NEXTION_CMD_TOUCH_COORDINATE_AWAKE                      0x67 // LISTEN
#define NEXTION_CMD_TOUCH_COORDINATE_SLEEP                      0x68 // LISTEN
#define NEXTION_CMD_STRING_DATA_ENCLOSED                        0x70 // READ
#define NEXTION_CMD_NUMERIC_DATA_ENCLOSED                       0x71 // READ
#define NEXTION_CMD_AUTO_ENTER_SLEEP                            0x86 // LISTEN
#define NEXTION_CMD_AUTO_ENTER_WAKEUP                           0x87 // LISTEN
#define NEXTION_CMD_READY                                       0x88 // LISTEN
#define NEXTION_CMD_START_MICROSD_UPDATE                        0x89 // LISTEN
#define NEXTION_CMD_TRANSPARENT_DATA_END                        0xFD // WAVE
#define NEXTION_CMD_TRANSPARENT_DATA_READY                      0xFE // WAVE

struct nextionComponent {
  int8_t page, id;
};

struct nextionTouch {
  int8_t page, id;
  bool event;
};

class INextion {
  private:
    bool wait();

  protected:
    SoftwareSerial *_serial;

  public:
    INextion(uint8_t rx, uint8_t tx);

    uint32_t begin(uint32_t speed = 0);
    String write(String instruction);
    String read();
    String wave(uint8_t id, uint8_t channel, uint8_t *data, size_t length);
};

class Nextion: public INextion {
  private:
    typedef void (*nextionPointer) (nextionTouch *);
    typedef void (*nextionTarget) (uint16_t *, uint16_t *);

    struct nextionCallback {
      nextionCallback *next;
      nextionTouch touch;
      nextionPointer pointer;
    };

    nextionCallback *_callbacks;
    nextionTarget _target;

    nextionCallback *callback(nextionTouch touch, nextionPointer pointer) {
      nextionCallback *item = new nextionCallback;
      item->next = NULL;
      item->pointer = pointer;
      item->touch = touch;
      return item;
    }

  public:
    Nextion(uint8_t rx, uint8_t tx): INextion(rx, tx) {};

    void attach() {
      _callbacks = NULL;
    }

    void attach(nextionComponent component, bool event, nextionPointer pointer) {
      attach({component.page, component.id, event}, pointer);
    }

    void attach(nextionTouch touch, nextionPointer pointer) {
      if (_callbacks) {
        nextionCallback *item = _callbacks;

        do if ((item->touch.page == touch.page) && (item->touch.id == touch.id) && (item->touch.event == touch.event)) {
            item->pointer = pointer;
            return;
          } while (item->next && (item = item->next));
        item->next = callback(touch, pointer);

      } else _callbacks = callback(touch, pointer);
    }

    String circle(uint16_t x, uint16_t y, uint16_t r, uint16_t c) {
      return write("cir " + String(x) + "," + String(y) + "," + String(r) + "," + String(c));
    }

    String clear(uint16_t c = 0xFFFFFF) {
      return write("cls " + String(c));
    }

    String click(uint8_t id, bool event) {
      return write("click " + String(id) + "," + String(event));
    }

    String crop(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t resource) {
      return write("picq " + String(x) + "," + String(y) + "," + String(w) + "," + String(h) + "," + String(resource));
    }

    String crop(uint16_t dx, uint16_t dy, uint16_t w, uint16_t h, uint16_t sx, uint16_t sy, uint8_t resource) {
      return write("xpic " + String(dx) + "," + String(dy) + "," + String(w) + "," + String(h) + "," + String(sx) + "," + String(sy) + "," + String(resource));
    }

    void detach() {
      _callbacks = NULL;
    }

    void detach(nextionComponent component, bool event) {
      detach({component.page, component.id, event});
    }

    void detach(nextionTouch touch) {
      if (_callbacks) {
        nextionCallback *item = _callbacks, *preview;
        do if ((item->touch.page == touch.page) && (item->touch.id == touch.id) && (item->touch.event == touch.event)) {
            if (item == _callbacks) _callbacks = _callbacks->next;
            else preview->next = ((item->next) ? item->next : NULL);
            return;
          } while (item->next && (preview = item) && (item = item->next));
      }
    }

    String disable(uint8_t id) {
      return write("tsw " + String(id) + ",0");
    }

    String enable(uint8_t id) {
      return write("tsw " + String(id) + ",1");
    }

    String erase(uint8_t id) {
      return write("cle " + String(id) + ",255");
    }

    String erase(uint8_t id, uint8_t channel) {
      return write("cle " + String(id) + "," + String(channel));
    }

    String fillCircle(uint16_t x, uint16_t y, uint16_t r, uint16_t c) {
      return write("cirs " + String(x) + "," + String(y) + "," + String(r) + "," + String(c));
    }

    String fillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
      return write("fill " + String(x) + "," + String(y) + "," + String(w) + "," + String(h) + "," + String(c));
    }

    String get(String attribute) {
      return write("get " + attribute);
    }

    String hide(uint8_t id) {
      return write("vis " + String(id) + ",0");
    }

    String line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t c) {
      return write("line " + String(x1) + "," + String(y1) + "," + String(x2) + "," + String(y2) + "," + String(c));
    }

    uint8_t listen() {
      if (_serial->available() > 3) {
        String string = read();

        switch ((uint8_t)string[0]) {

          case NEXTION_CMD_STARTUP:
            if ((string[1] == 0x00) && (string[2] == 0x00)) break;

          case NEXTION_CMD_TOUCH_EVENT:
            nextionCallback *item = _callbacks;
            while (item) {
              if ((item->touch.page == (uint8_t)string[1]) && (item->touch.id == (uint8_t)string[2]) && (item->touch.event == (uint8_t)string[3]) && (item->pointer)) {
                nextionTouch touch = item->touch;
                item->pointer(&touch);
                break;
              }
              item = item->next;
            }
            break;

          case NEXTION_CMD_TOUCH_COORDINATE_AWAKE: 
          case NEXTION_CMD_TOUCH_COORDINATE_SLEEP:
            if (_target) {
              uint16_t x = (uint16_t)string[1] << 8 | (uint8_t)string[2], y = (uint16_t)string[3] << 8 | (uint8_t)string[4];
              _target(&x, &y);
            }
            break;

          case NEXTION_CMD_SERIAL_BUFFER_OVERFLOW: 
          case NEXTION_CMD_AUTO_ENTER_SLEEP: 
          case NEXTION_CMD_AUTO_ENTER_WAKEUP: 
          case NEXTION_CMD_READY: 
          case NEXTION_CMD_START_MICROSD_UPDATE:
            break;
        }
        return (uint8_t)string[0];
      }
      return 0;
    }

    String page() {
      return write("sendme");
    }

    String page(uint8_t page) {
      return write("page " + String(page));
    }

    String picture(uint16_t x, uint16_t y, uint8_t resource) {
      return write("pic " + String(x) + "," + String(y) + "," + String(resource));
    }

    String reboot() {
      return write("rest");
    }

    String rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t c) {
      return write("draw " + String(x1) + "," + String(y1) + "," + String(x2) + "," + String(y2) + "," + String(c));
    }

    String reply(bool state) {
      return write("thup=" + String(state));
    }

    String show(uint8_t id) {
      return write("vis " + String(id) + ",1");
    }

    String sleep() {
      return write("sleep=1");
    }

    void target(nextionTarget pointer) {
      _target = pointer;
    }

    String text(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t font, uint16_t foreground, uint16_t background, uint8_t alignX, uint8_t alignY, uint8_t fill, String text) {
      return write("xstr " + String(x) + "," + String(y) + "," + String(w) + "," + String(h) + "," +
                   String(font) + "," + String(foreground) + "," + String(background) + "," + String(alignX) + "," + String(alignY) + "," + String(fill) + "," + text );
    }

    String wakeup() {
      return write("sleep=0");
    }

    String wave(uint8_t id, uint8_t channel, uint8_t data) {
      return write("add " + String(id) + "," + String(channel) + "," + String(data));
    }

    String wave(uint8_t id, uint8_t channel, uint8_t *data, size_t length) {
      return INextion::wave(id, channel, *data, length);
    }
};

#endif
#endif
