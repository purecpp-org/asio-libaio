#include <aio.h>
#include <asio/io_context.hpp>
#include <asio/stream_file.hpp>
#include <fcntl.h>
#include <iostream>
#include <libaio.h>
#include <string>
#include <string_view>

using namespace asio;

void test_aio2() {
  int fd = open("/tmp/test.txt", O_RDWR);

  io_context_t ctx{};
  int r = io_setup(128, &ctx);

  char buf[4096];
  struct iocb cb {};

  struct iocb *list_of_iocb[1] = {&cb};

  ::io_prep_pread(&cb, fd, buf, 4096, 0);

  r = io_submit(ctx, 1, list_of_iocb);

  struct io_event events[1] = {{0}};
  r = io_getevents(ctx, 1, 1, events, NULL);

  std::cout << events[0].res2 << " " << events[0].res << "\n";
}

void test_aio() {
  int fd = open("/tmp/test.txt", O_DIRECT | O_RDWR | O_CREAT, 0644);

  io_context_t ctx{};
  int r = io_setup(128, &ctx);

  char buf[4096];
  void *data = buf;
  int r1 = posix_memalign(&data, 512, 4096);
  if (r1) {
    fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
    return;
  }

  struct iocb cb {};
  struct iocb cb2 {};

  struct iocb *list_of_iocb[2] = {&cb, &cb2};

  ::io_prep_pread(&cb, fd, data, 4096, 0);

  char wbuf[4096] = "test";
  void *data2 = wbuf;
  r1 = posix_memalign(&data2, 512, 4096);
  if (r1) {
    fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
    return;
  }
  ::io_prep_pwrite(&cb2, fd, data2, 512, 0);

  r = io_submit(ctx, 2, list_of_iocb);

  struct io_event events[2] = {};
  r = io_getevents(ctx, 2, 2, events, NULL);

  std::cout << events[0].res2 << " " << events[0].res << "\n";
  std::cout << events[1].res2 << " " << events[1].res << "\n";
}

void test_async_read() {
  asio::io_context ioc;
  try {
    stream_file file(ioc, "/tmp/test.txt",
                     stream_file::read_only | stream_file::direct);
    void *data = nullptr;
    int r = posix_memalign(&data, 512, 4096);
    if (r) {
      fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
      return;
    }

    file.async_read_some(buffer(data, 4096),
                         [data](asio::error_code ec, size_t size) {
                           std::cout << ec.message() << " " << size << "\n";
                           std::string_view str((char *)data, size);
                           std::cout << str << "\n";
                         });
    ioc.run();
  } catch (std::exception &e) {
    std::cout << e.what() << "\n";
  }
}

void test_async_write() {
  asio::io_context ioc;
  try {
    stream_file file(ioc, "/tmp/test.txt",
                     stream_file::write_only | stream_file::direct);
    void *data = nullptr;
    int r = posix_memalign(&data, 512, 4096);
    if (r) {
      fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
      return;
    }

    std::string str = "it is a test";
    strcpy((char *)data, str.data());

    file.async_write_some(buffer(data, 512),
                          [data](asio::error_code ec, size_t size) {
                            std::cout << ec.message() << " " << size << "\n";
                            std::string_view str((char *)data, size);
                            std::cout << str << "\n";
                          });
    ioc.run();
  } catch (std::exception &e) {
    std::cout << e.what() << "\n";
  }
}

void un_direct() {
  asio::io_context ioc;
  try {
    stream_file file(ioc, "/tmp/test.txt",
                     stream_file::write_only | stream_file::direct |
                         stream_file::create);
    void *data = nullptr;
    int r = posix_memalign(&data, 512, 4096);
    if (r) {
      fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
      return;
    }

    std::string str = "it is a test";
    strcpy((char *)data, str.data());

    file.async_write_some(buffer(data, 512),
                          [data](asio::error_code ec, size_t size) {
                            std::cout << ec.message() << " " << size << "\n";
                          });
    ioc.run();

    file.close();
    file.open("/tmp/test.txt", stream_file::write_only);

    auto size = file.write_some(buffer(str.data(), str.size()));
    std::cout << size << "\n";
  } catch (std::exception &e) {
    std::cout << e.what() << "\n";
  }
}

int main(int, char **) {
  test_aio();
  test_async_write();
  test_async_read();
  un_direct();
}
