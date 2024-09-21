#include <gtest/gtest.h>
#include "download.h"

namespace MetadataTest {
  #define MAKE_METADATA_TEST_F(NAME)                          \
  TEST_F(NAME, Name) {                                        \
    EXPECT_EQ(p_download_->get_name(), m_expected_name_);     \
  };                                                          \
  TEST_F(NAME, Hash) {                                        \
    EXPECT_EQ(p_download_->get_infohash(), m_expected_hash_); \
  };                                                          \
  TEST_F(NAME, Files) {                                       \
    VecOfFiles files = p_download_->get_files();              \
    for (size_t i = 0; i < files.size(); ++i) {               \
      EXPECT_EQ(m_expected_files_[i], files[i]);              \
    }                                                         \
  };

#define FLAGS_METADATA_PARAMS     "flags",      "18945a9300abfe4ff2442559bb08b8ddb357c16f", {{"flags/denmark.png", 476}, {"flags/norway.png", 744}, { "flags/sweden.png", 636}}
#define NASA1_OGV_METADATA_PARAMS "nasa1.ogv",  "8a32f3f6f3c9125da79e29c869122758004ee837", {{"nasa1.ogv", 58703}}
#define NASA_METADATA_PARAMS      "nasa",       "6fa46c9a0bb4eecb837c25845d39c5324be66401", {{"nasa/nasa2.ogv", 104050}, {"nasa/nasa3.ogv", 88445}}
#define SWEDEN_PNG_METADATA       "sweden.png", "fce002e43ed1159f4612982ce8fcdb9d30e48f1e", {{"sweden.png", 636}}

  struct MetadataBase
    : public testing::Test {
  public:
    using String      = std::string;
    using DownloadPtr = std::shared_ptr<::Download>;
    using MetadataPtr = std::shared_ptr<libtorrent::entry::preformatted_type>;
    using PairOfFile  = std::pair<String, uint64_t>;
    using VecOfFiles  = std::vector<PairOfFile>;

  protected:
    MetadataBase(String file, String name, String hash, VecOfFiles files)
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

  struct FlagsMetadata
    : public MetadataBase {
    FlagsMetadata()
      : MetadataBase("flags.torrent", FLAGS_METADATA_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(FlagsMetadata);

  struct Nasa1OgvMetadata
    : public MetadataBase {
    Nasa1OgvMetadata()
      : MetadataBase("nasa1.ogv.torrent", NASA1_OGV_METADATA_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(Nasa1OgvMetadata);

  struct NasaMetadata
    : public MetadataBase {
    NasaMetadata()
      : MetadataBase("nasa.torrent", NASA_METADATA_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(NasaMetadata);
  
  struct SwedenPngMetadata
    : public MetadataBase {
    SwedenPngMetadata()
      : MetadataBase("sweden.png.torrent", SWEDEN_PNG_METADATA)
    { }
  };

  MAKE_METADATA_TEST_F(SwedenPngMetadata);

  struct MagnetUriFlags
    : public MetadataBase {
    MagnetUriFlags()
      : MetadataBase("magnet:?xt=urn:btih:18945a9300abfe4ff2442559bb08b8ddb357c16f&dn=flags", FLAGS_METADATA_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MagnetUriFlags);

  struct MagnetUriNasa1Ogv
    : public MetadataBase {
    MagnetUriNasa1Ogv()
      : MetadataBase("magnet:?xt=urn:btih:8a32f3f6f3c9125da79e29c869122758004ee837&dn=nasa1.ogv", NASA1_OGV_METADATA_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MagnetUriNasa1Ogv);

  struct MagnetUriNasa
    : public MetadataBase {
    MagnetUriNasa()
      : MetadataBase("magnet:?xt=urn:btih:6fa46c9a0bb4eecb837c25845d39c5324be66401&dn=nasa", NASA_METADATA_PARAMS)
    { }
  };

  MAKE_METADATA_TEST_F(MagnetUriNasa);

  struct MagnetUriSwedenPng
    : public MetadataBase {
    MagnetUriSwedenPng()
      : MetadataBase("magnet:?xt=urn:btih:fce002e43ed1159f4612982ce8fcdb9d30e48f1e&dn=sweden.png", SWEDEN_PNG_METADATA)
    { }
  };

  MAKE_METADATA_TEST_F(MagnetUriSwedenPng);
} // namespace MetadataTest

namespace DownloadTest {

} // namepsace DownloadTest

