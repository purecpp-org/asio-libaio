#include "asio/error_code.hpp"
#include <aio.h>
#include <asio/io_context.hpp>
#include <asio/random_access_file.hpp>
#include <asio/stream_file.hpp>
#include <cstdlib>
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

  void *data;
  int r1 = posix_memalign(&data, 512, 4096);
  if (r1) {
    fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
    return;
  }

  struct iocb cb {};
  struct iocb cb2 {};

  struct iocb *list_of_iocb[2] = {&cb, &cb2};

  ::io_prep_pread(&cb, fd, data, 4096, 0);

  void *data2;
  size_t buf_size = 4096;
  r1 = posix_memalign(&data2, 512, buf_size);
  if (r1) {
    fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
    return;
  }
  ::io_prep_pwrite(&cb2, fd, data2, buf_size, 0);

  r = io_submit(ctx, 2, list_of_iocb);

  struct io_event events[2] = {};
  r = io_getevents(ctx, 2, 2, events, NULL);

  std::cout << events[0].res2 << " " << events[0].res << "\n";
  std::cout << events[1].res2 << " " << events[1].res << "\n";

  free(data);
  free(data2);
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
    free(data);
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
    size_t buf_size = 4096;
    int r = posix_memalign(&data, 512, buf_size);
    if (r) {
      fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
      return;
    }

    std::string str = "it is a test";
    strcpy((char *)data, str.data());

    file.async_write_some(buffer(data, buf_size),
                          [data](asio::error_code ec, size_t size) {
                            std::cout << ec.message() << " " << size << "\n";
                            std::string_view str((char *)data, size);
                            std::cout << str << "\n";
                          });
    std::cout << "test\n";
    ioc.run();
    free(data);
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
    free(data);

    file.close();
    file.open("/tmp/test.txt", stream_file::write_only);

    auto size = file.write_some(buffer(str.data(), str.size()));
    std::cout << size << "\n";
  } catch (std::exception &e) {
    std::cout << e.what() << "\n";
  }
}

void offset_visit() {
  asio::io_context ioc;
  try {
    stream_file ss_file(ioc, "/tmp/test.txt",
                        stream_file::write_only | stream_file::create);
    std::string str = "it is a test for random";
    ss_file.write_some(buffer(str.data(), str.size()));
    ss_file.close();

    random_access_file file(ioc.get_executor(), "/tmp/test.txt",
                            stream_file::read_write | stream_file::create);

    char buf[6];
    file.async_read_some_at(
        5, buffer(buf, 6), [&file](asio::error_code ec, size_t size) {
          std::cout << "read " << size << "\n";
          char buf[6]{};
          file.async_read_some_at(5, buffer(buf),
                                  [buf](error_code ec, size_t size) {
                                    std::cout << "write " << size << "\n";
                                  });
        });

    ioc.run();
  } catch (std::exception &e) {
    std::cout << e.what() << "\n";
  }
}

int main(int, char **) {
  test_aio();
  test_async_write();
  test_async_read();
  un_direct();
  offset_visit();
}
