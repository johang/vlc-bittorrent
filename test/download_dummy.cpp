#include <gtest/gtest.h>
#include "download.h"

namespace Metadata {
  #define WRITE_EXP_GOT(EXP, GOT) std::cout << "Expected: " << (EXP) << ", got: " << (GOT) << std::endl;

  #define MAKE_METADATA_TEST_F(NAME) \
  TEST_F(NAME, Test) { \
    EXPECT_EQ(p_download_->get_name(), m_expected_name_); \
    WRITE_EXP_GOT(m_expected_name_, p_download_->get_name()) \
    EXPECT_EQ(p_download_->get_infohash(), m_expected_hash_); \
    WRITE_EXP_GOT(m_expected_hash_, p_download_->get_infohash()) \
    VecOfFiles files = p_download_->get_files(); \
    for (size_t i = 0; i < files.size(); ++i) { \
      EXPECT_EQ(m_expected_files_[i], files[i]); \
      WRITE_EXP_GOT(m_expected_files_[i].first, files[i].first) \
      WRITE_EXP_GOT(m_expected_files_[i].second, files[i].second) \
    } \
  };

  #define METADATA_FLAGS_PARAMS "flags", "18945a9300abfe4ff2442559bb08b8ddb357c16f", {{"flags/denmark.png", 476}, {"flags/norway.png", 744}, { "flags/sweden.png", 636}}
  #define METADATA_NASA1_OGV_PARAMS "nasa1.ogv", "8a32f3f6f3c9125da79e29c869122758004ee837", {{"nasa1.ogv", 58703}}
  #define METADATA_NASA_PARAMS "nasa", "6fa46c9a0bb4eecb837c25845d39c5324be66401", {{"nasa/nasa2.ogv", 104050}, {"nasa/nasa3.ogv", 88445}}
  #define METADATA_SWEDEN_PNG_PARAMS "sweden.png", "fce002e43ed1159f4612982ce8fcdb9d30e48f1e", {{"sweden.png", 636}}
  #define METADATA_BIG_BUCK_BUNNY_PARAMS "Big Buck Bunny", "dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c", { {"Big Buck Bunny/Big Buck Bunny.en.srt", 140}, {"Big Buck Bunny/Big Buck Bunny.mp4", 276134947}, {"Big Buck Bunny/poster.jpg", 310380} }

  struct Base
    : public testing::Test {
  public:
    using String      = std::string;
    using DownloadPtr = std::shared_ptr<::Download>;
    using MetadataPtr = std::shared_ptr<libtorrent::entry::preformatted_type>;
    using PairOfFile  = std::pair<String, uint64_t>;
    using VecOfFiles  = std::vector<PairOfFile>;

  protected:
    Base(String file, String name, String hash, VecOfFiles files)
      : m_torrent_file_(std::move(file)),
        m_expected_name_(std::move(name)),
        m_expected_hash_(std::move(hash)),
        m_expected_files_(std::move(files))
    { }

    void SetUp() override {
      p_metadata_ = Download::get_metadata(m_torrent_file_, ".", "/tmp");
      p_download_ = Download::get_download(p_metadata_->data(), p_metadata_->size(), ".", true);
    }

  private:
    String      m_torrent_file_;

  protected:
    DownloadPtr p_download_;
    MetadataPtr p_metadata_;

    String      m_expected_name_;
    String      m_expected_hash_;
    VecOfFiles  m_expected_files_;
  };

  struct MetadataFlags
    : public Base {
    MetadataFlags()
      : Base("flags.torrent", METADATA_FLAGS_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataFlags);

  struct MetadataBigBuckBunny
    : public Base {
    MetadataBigBuckBunny()
      : Base("big-buck-bunny.torrent", METADATA_BIG_BUCK_BUNNY_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataBigBuckBunny);

  struct MetadataNasa1Ogv
    : public Base {
    MetadataNasa1Ogv()
      : Base("nasa1.ogv.torrent", METADATA_NASA1_OGV_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataNasa1Ogv);

  struct MetadataNasa
    : public Base {
    MetadataNasa()
      : Base("nasa.torrent", METADATA_NASA_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataNasa);

  struct MetadataSwedenPng
    : public Base {
    MetadataSwedenPng()
      : Base("sweden.png.torrent", METADATA_SWEDEN_PNG_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataSwedenPng);

  struct MetadataMUriFlags
    : public Base {
    MetadataMUriFlags()
      : Base("magnet:?xt=urn:btih:18945a9300abfe4ff2442559bb08b8ddb357c16f&dn=flags", METADATA_FLAGS_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataMUriFlags);

  struct MetadataMUriNasa1Ogv
    : public Base {
    MetadataMUriNasa1Ogv()
      : Base("magnet:?xt=urn:btih:8a32f3f6f3c9125da79e29c869122758004ee837&dn=nasa1.ogv", METADATA_NASA1_OGV_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataMUriNasa1Ogv);

  struct MetadataMUriNasa
    : public Base {
    MetadataMUriNasa()
      : Base("magnet:?xt=urn:btih:6fa46c9a0bb4eecb837c25845d39c5324be66401&dn=nasa", METADATA_NASA_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataMUriNasa);

  struct MetadataMUriSwedenPng
    : public Base {
    MetadataMUriSwedenPng()
      : Base("magnet:?xt=urn:btih:fce002e43ed1159f4612982ce8fcdb9d30e48f1e&dn=sweden.png", METADATA_SWEDEN_PNG_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MetadataMUriSwedenPng);
} // namespace Metadata

namespace Downloads {
  struct Base
    : public testing::Test {
  public:
    using String      = ::std::string;
    using DownloadPtr = ::std::shared_ptr<::Download>;
    using MetadataPtr = ::std::shared_ptr<libtorrent::entry::preformatted_type>;
    using PairOfFile  = ::std::pair<String, uint64_t>;
    using VecOfFiles  = ::std::vector<PairOfFile>;
    using PairOfExp   = ::std::pair<size_t, size_t>;
    using VecOfExp    = ::std::vector<PairOfExp>;

    Base(String torrent, VecOfExp files)
      : m_torrent_file_(::std::move(torrent)),
        m_expected_files_(::std::move(files))
    { }

  protected:
    void SetUp() override {
      p_metadata_ = ::Download::get_metadata(m_torrent_file_, ".", "/tmp");
      p_download_ = ::Download::get_download(p_metadata_->data(), p_metadata_->size(), ".", true);
    }

  private:
    String      m_torrent_file_;

  protected:
    DownloadPtr p_download_;
    MetadataPtr p_metadata_;

    VecOfExp    m_expected_files_;
  };

# define MAKE_DOWNLOAD_TESTS_F(KLASS) \
  TEST_F(KLASS, Test) { \
    VecOfFiles files = p_download_->get_files(); \
    int32_t file_index = 0; \
    for (Base::PairOfFile &file : files) { \
      size_t total = 0; \
      while (true) { \
        char buffer[64 * 1024] = { }; \
        ssize_t readed = p_download_->read(file_index, total, buffer, sizeof(buffer) / sizeof(*buffer)); \
        if (readed <= 0) { \
          break; \
        } \
        total += readed; \
      } \
      EXPECT_EQ(total, m_expected_files_[file_index].first); \
      EXPECT_EQ(file_index, m_expected_files_[file_index].second); \
      file_index++; \
    } \
  };

  #define DOWNLOAD_FLAGS         {{476, 0}, {744, 1}, {636, 2}}
  #define DOWNLOAD_NASA1_OGV     {{58703, 0}}
  #define DOWNLOAD_NASA          {{104050, 0}, {88445, 1}}
  #define DOWNLOAD_SWEDEN_PNG    {{636, 0}}
  #define DOWNLOAD_BIG_BUCK_BUNNY {{140, 0}, {276134947, 1}, {310380, 2}}

  struct DownloadFlags
    : public Base {
  public:
    DownloadFlags()
      : Base("flags.torrent", DOWNLOAD_FLAGS)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadFlags)

  struct DownloadNasa1Ogv
    : public Base {
  public:
    DownloadNasa1Ogv()
      : Base("nasa1.ogv.torrent", DOWNLOAD_NASA1_OGV)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadNasa1Ogv)

  struct DownloadNasa
    : public Base {
  public:
    DownloadNasa()
      : Base("nasa.torrent", DOWNLOAD_NASA)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadNasa)

  struct DownloadSwedenPng
    : public Base {
  public:
    DownloadSwedenPng()
      : Base("sweden.png.torrent", DOWNLOAD_SWEDEN_PNG)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadSwedenPng)

  struct DownloadBigBuckBunny
    : public Base {
  public:
    DownloadBigBuckBunny()
      : Base("big-buck-bunny.torrent", DOWNLOAD_BIG_BUCK_BUNNY)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadBigBuckBunny)

  struct DownloadMUriFlags
    : public Base {
  public:
    DownloadMUriFlags()
      : Base("magnet:?xt=urn:btih:18945a9300abfe4ff2442559bb08b8ddb357c16f&dn=flags", DOWNLOAD_FLAGS)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadMUriFlags)

  struct DownloadMUriNasa1Ogv
    : public Base {
  public:
    DownloadMUriNasa1Ogv()
      : Base("magnet:?xt=urn:btih:8a32f3f6f3c9125da79e29c869122758004ee837&dn=nasa1.ogv", DOWNLOAD_NASA1_OGV)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadMUriNasa1Ogv)

  struct DownloadUriNasa
    : public Base {
  public:
    DownloadUriNasa()
      : Base("magnet:?xt=urn:btih:6fa46c9a0bb4eecb837c25845d39c5324be66401&dn=nasa", DOWNLOAD_NASA)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadUriNasa)

  struct DownloadUriSwedenPng
    : public Base {
  public:
    DownloadUriSwedenPng()
      : Base("magnet:?xt=urn:btih:fce002e43ed1159f4612982ce8fcdb9d30e48f1e&dn=sweden.png", DOWNLOAD_SWEDEN_PNG)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadUriSwedenPng)

  struct DownloadMUriBigBuckBunny
    : public Base {
  public:
    DownloadMUriBigBuckBunny()
      : Base("magnet:?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big%20Buck%20Bunny&tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969&tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969&tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337&tr=udp%3A%2F%2Fexplodie.org%3A6969&tr=udp%3A%2F%2Ftracker.empire-js.us%3A1337&tr=wss%3A%2F%2Ftracker.btorrent.xyz&tr=wss%3A%2F%2Ftracker.openwebtorrent.com&tr=wss%3A%2F%2Ftracker.fastcast.nz&ws=https://webtorrent.io/torrents/", DOWNLOAD_BIG_BUCK_BUNNY)
    { }
  };

  MAKE_DOWNLOAD_TESTS_F(DownloadMUriBigBuckBunny)
} // namepsace Downloads
