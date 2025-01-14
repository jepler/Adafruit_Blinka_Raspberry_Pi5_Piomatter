#pragma once
#include "thread_queue.h"

namespace piomatter {

struct buffer_manager {
    enum { no_buffer = -1, exit_request = -2 };

    buffer_manager() {
        free_buffers.push(0);
        free_buffers.push(1);
        free_buffers.push(2);
    }

    int get_free_buffer() { return free_buffers.pop_blocking(); }
    void put_free_buffer(int i) { free_buffers.push(i); }

    int get_filled_buffer() {
        auto r = filled_buffers.pop_nonblocking();
        return r ? r.value() : no_buffer;
    }

    void put_filled_buffer(int i) { filled_buffers.push(i); }

    void request_exit() { filled_buffers.push(exit_request); }

  private:
    thread_queue<int> free_buffers, filled_buffers;
};

} // namespace piomatter
