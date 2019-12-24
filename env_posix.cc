#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <iostream>

#include "env.h"
#include "slice.h"
#include "status.h"
#include "port.h"

namespace leveldb {


namespace {

// Common flags defined for all posix open operations
#if defined(HAVE_O_CLOEXEC)
constexpr const int kOpenBaseFlags = O_CLOEXEC;
#else
constexpr const int kOpenBaseFlags = 0;
#endif  // defined(HAVE_O_CLOEXEC)

constexpr const size_t kWritableFileBufferSize = 65536;


Status PosixError(const std::string& context, int error_number) {
  if (error_number == ENOENT) {
    return Status::NotFound(context, std::strerror(error_number));
  } else {
    return Status::IOError(context, std::strerror(error_number));
  }
}

class PosixWritableFile final : public WritableFile {
 public:
  PosixWritableFile(std::string filename, int fd)
      : pos_(0),
        fd_(fd),
        is_manifest_(IsManifest(filename)),
        filename_(std::move(filename)),
        dirname_(Dirname(filename_)) {}

  ~PosixWritableFile() override {
    if (fd_ >= 0) {
      // Ignoring any potential errors
      Close();
    }
  }

  Status Append(const Slice& data) override {
    size_t write_size = data.size();
    const char* write_data = data.data();

    // Fit as much as possible into buffer.
    size_t copy_size = std::min(write_size, kWritableFileBufferSize - pos_);
    std::memcpy(buf_ + pos_, write_data, copy_size);
    write_data += copy_size;
    write_size -= copy_size;
    pos_ += copy_size;
    if (write_size == 0) {
      return Status::OK();
    }

    // Can't fit in buffer, so need to do at least one write.
    Status status = FlushBuffer();
    if (!status.ok()) {
      return status;
    }

    // Small writes go to buffer, large writes are written directly.
    if (write_size < kWritableFileBufferSize) {
      std::memcpy(buf_, write_data, write_size);
      pos_ = write_size;
      return Status::OK();
    }
    return WriteUnbuffered(write_data, write_size);
  }

  Status Close() override {
    Status status = FlushBuffer();
    const int close_result = ::close(fd_);
    if (close_result < 0 && status.ok()) {
      status = PosixError(filename_, errno);
    }
    fd_ = -1;
    return status;
  }

  Status Flush() override { return FlushBuffer(); }

  Status Sync() override {
    // Ensure new files referred to by the manifest are in the filesystem.
    //
    // This needs to happen before the manifest file is flushed to disk, to
    // avoid crashing in a state where the manifest refers to files that are not
    // yet on disk.
    Status status = SyncDirIfManifest();
    if (!status.ok()) {
      return status;
    }

    status = FlushBuffer();
    if (!status.ok()) {
      return status;
    }

    return SyncFd(fd_, filename_);
  }

 private:
  Status FlushBuffer() {
    Status status = WriteUnbuffered(buf_, pos_);
    pos_ = 0;
    return status;
  }

  Status WriteUnbuffered(const char* data, size_t size) {
    while (size > 0) {
      ssize_t write_result = ::write(fd_, data, size);
      if (write_result < 0) {
        if (errno == EINTR) {
          continue;  // Retry
        }
        return PosixError(filename_, errno);
      }
      data += write_result;
      size -= write_result;
    }
    return Status::OK();
  }

  Status SyncDirIfManifest() {
    Status status;
    if (!is_manifest_) {
      return status;
    }

    int fd = ::open(dirname_.c_str(), O_RDONLY | kOpenBaseFlags);
    if (fd < 0) {
      status = PosixError(dirname_, errno);
    } else {
      status = SyncFd(fd, dirname_);
      ::close(fd);
    }
    return status;
  }

  // Ensures that all the caches associated with the given file descriptor's
  // data are flushed all the way to durable media, and can withstand power
  // failures.
  //
  // The path argument is only used to populate the description string in the
  // returned Status if an error occurs.
  static Status SyncFd(int fd, const std::string& fd_path) {
#if HAVE_FULLFSYNC
    // On macOS and iOS, fsync() doesn't guarantee durability past power
    // failures. fcntl(F_FULLFSYNC) is required for that purpose. Some
    // filesystems don't support fcntl(F_FULLFSYNC), and require a fallback to
    // fsync().
    if (::fcntl(fd, F_FULLFSYNC) == 0) {
      return Status::OK();
    }
#endif  // HAVE_FULLFSYNC

#if HAVE_FDATASYNC
    bool sync_success = ::fdatasync(fd) == 0;
#else
    bool sync_success = ::fsync(fd) == 0;
#endif  // HAVE_FDATASYNC

    if (sync_success) {
      return Status::OK();
    }
    return PosixError(fd_path, errno);
  }

  // Returns the directory name in a path pointing to a file.
  //
  // Returns "." if the path does not contain any directory separator.
  static std::string Dirname(const std::string& filename) {
    std::string::size_type separator_pos = filename.rfind('/');
    if (separator_pos == std::string::npos) {
      return std::string(".");
    }
    // The filename component should not contain a path separator. If it does,
    // the splitting was done incorrectly.
    assert(filename.find('/', separator_pos + 1) == std::string::npos);

    return filename.substr(0, separator_pos);
  }

  // Extracts the file name from a path pointing to a file.
  //
  // The returned Slice points to |filename|'s data buffer, so it is only valid
  // while |filename| is alive and unchanged.
  static Slice Basename(const std::string& filename) {
    std::string::size_type separator_pos = filename.rfind('/');
    if (separator_pos == std::string::npos) {
      return Slice(filename);
    }
    // The filename component should not contain a path separator. If it does,
    // the splitting was done incorrectly.
    assert(filename.find('/', separator_pos + 1) == std::string::npos);

    return Slice(filename.data() + separator_pos + 1,
                 filename.length() - separator_pos - 1);
  }

  // True if the given file is a manifest file.
  static bool IsManifest(const std::string& filename) {
    return Basename(filename).starts_with("MANIFEST");
  }

  // buf_[0, pos_ - 1] contains data to be written to fd_.
  char buf_[kWritableFileBufferSize];
  size_t pos_;
  int fd_;

  const bool is_manifest_;  // True if the file's name starts with MANIFEST.
  const std::string filename_;
  const std::string dirname_;  // The directory of filename_.
};


class PosixEnv : public Env {
 public:
  PosixEnv();
  ~PosixEnv() override {
    static char msg[] = "PosixEnv singleton destroyed. Unsupported behavior!\n";
    std::fwrite(msg, 1, sizeof(msg), stderr);
    std::abort();
  }

  Status NewWritableFile(const std::string& filename,
                         WritableFile** result) override {
    int fd = ::open(filename.c_str(),
                    O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
    if (fd < 0) {
      *result = nullptr;
      return PosixError(filename, errno);
    }

    *result = new PosixWritableFile(filename, fd);
    return Status::OK();
  }

  Status DeleteFile(const std::string& filename) override {
    if (::unlink(filename.c_str()) != 0) {
      return PosixError(filename, errno);
    }
    return Status::OK();
  }

  void Schedule(void (*background_work_function)(void* background_work_arg),
                void* background_work_arg) override;

  void StartThread(void (*thread_main)(void* thread_main_arg),
                   void* thread_main_arg) override {
    std::thread new_thread(thread_main, thread_main_arg);
    new_thread.detach();
  }

  uint64_t NowMicros() override {
    static constexpr uint64_t kUsecondsPerSecond = 1000000;
    struct ::timeval tv;
    ::gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * kUsecondsPerSecond + tv.tv_usec;
  }

 private:
  void BackgroundThreadMain();

  static void BackgroundThreadEntryPoint(PosixEnv* env) {
    env->BackgroundThreadMain();
  }

  struct BackgroundWorkItem {
    explicit BackgroundWorkItem(void (*function)(void* arg), void* arg)
        : function(function), arg(arg) {}

    void (*const function)(void*);
    void* const arg;
  };

  port::Mutex background_work_mutex_;
  port::CondVar background_work_cv_;
  bool started_background_thread_;

  std::queue<BackgroundWorkItem> background_work_queue_;

  //PosixLockTable locks_;  // Thread-safe.
  //Limiter mmap_limiter_;  // Thread-safe.
  //Limiter fd_limiter_;    // Thread-safe.

};

} //namespace 
PosixEnv::PosixEnv()    
: background_work_cv_(&background_work_mutex_),
      started_background_thread_(false){}

void PosixEnv::Schedule(
    void (*background_work_function)(void* background_work_arg),
    void* background_work_arg) {
  background_work_mutex_.Lock();

  // Start the background thread, if we haven't done so already.
  if (!started_background_thread_) {
    started_background_thread_ = true;
    std::thread background_thread(PosixEnv::BackgroundThreadEntryPoint, this);
    background_thread.detach();
  }

  // If the queue is empty, the background thread may be waiting for work.
  if (background_work_queue_.empty()) {
    background_work_cv_.Signal();
  }

  background_work_queue_.emplace(background_work_function, background_work_arg);
  background_work_mutex_.Unlock();
}

void PosixEnv::BackgroundThreadMain() {
  while (true) {
    background_work_mutex_.Lock();

    // Wait until there is work to be done.
    while (background_work_queue_.empty()) {
      background_work_cv_.Wait();
    }

    assert(!background_work_queue_.empty());
    auto background_work_function = background_work_queue_.front().function;
    void* background_work_arg = background_work_queue_.front().arg;
    background_work_queue_.pop();

    background_work_mutex_.Unlock();
    background_work_function(background_work_arg);
  }
}

namespace {

// Wraps an Env instance whose destructor is never created.
//
// Intended usage:
//   using PlatformSingletonEnv = SingletonEnv<PlatformEnv>;
//   void ConfigurePosixEnv(int param) {
//     PlatformSingletonEnv::AssertEnvNotInitialized();
//     // set global configuration flags.
//   }
//   Env* Env::Default() {
//     static PlatformSingletonEnv default_env;
//     return default_env.env();
//   }
template <typename EnvType>
class SingletonEnv {
 public:
  SingletonEnv() {
#if !defined(NDEBUG)
    env_initialized_.store(true, std::memory_order::memory_order_relaxed);
#endif  // !defined(NDEBUG)
    static_assert(sizeof(env_storage_) >= sizeof(EnvType),
                  "env_storage_ will not fit the Env");
    static_assert(alignof(decltype(env_storage_)) >= alignof(EnvType),
                  "env_storage_ does not meet the Env's alignment needs");
    new (&env_storage_) EnvType();
  }
  ~SingletonEnv() = default;

  SingletonEnv(const SingletonEnv&) = delete;
  SingletonEnv& operator=(const SingletonEnv&) = delete;

  Env* env() { return reinterpret_cast<Env*>(&env_storage_); }

  static void AssertEnvNotInitialized() {
#if !defined(NDEBUG)
    assert(!env_initialized_.load(std::memory_order::memory_order_relaxed));
#endif  // !defined(NDEBUG)
  }

 private:
  typename std::aligned_storage<sizeof(EnvType), alignof(EnvType)>::type
      env_storage_;
#if !defined(NDEBUG)
  static std::atomic<bool> env_initialized_;
#endif  // !defined(NDEBUG)
};

#if !defined(NDEBUG)
template <typename EnvType>
std::atomic<bool> SingletonEnv<EnvType>::env_initialized_;
#endif  // !defined(NDEBUG)

using PosixDefaultEnv = SingletonEnv<PosixEnv>;

}  // namespace


Env* Env::Default() {
  static PosixDefaultEnv env_container;
  return env_container.env();
}

}
