// 실제 SMK 전체를 창/오디오 장치 없이 해제해 독립 디코더와 비교할 프레임·PCM 해시를 남긴다.
#include "client/SmackerDecoder.h"
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace { // 제품 디코더 결과의 검증용 해시/직렬화만 담당한다.
class Digest { // Windows SHA256의 현재 상태를 단일 소유하며 프레임별/전체 스트림에 사용한다.
public:
    // 시스템 SHA256 provider와 그 provider가 요구한 크기의 현재 해시를 만든다.
    Digest() {
        Check(BCryptOpenAlgorithmProvider(&algorithm_,BCRYPT_SHA256_ALGORITHM,nullptr,0));DWORD size{},received{};
        Check(BCryptGetProperty(algorithm_,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&received,0));object_.resize(size);Check(BCryptCreateHash(algorithm_,&hash_,object_.data(),size,nullptr,0,0));
    }
    ~Digest() { if (hash_) BCryptDestroyHash(hash_);if (algorithm_) BCryptCloseAlgorithmProvider(algorithm_,0); } // 해시 뒤 provider 순서로 반납한다.
    void Feed(std::span<const std::uint8_t> bytes) { if (!bytes.empty()) Check(BCryptHashData(hash_,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)); } // 현재 바이트만 이어 넣는다.
    std::string Finish() { std::array<std::uint8_t,32> result{};Check(BCryptFinishHash(hash_,result.data(),static_cast<ULONG>(result.size()),0));constexpr std::string_view digits="0123456789abcdef";std::string text;
        for (const auto byte:result) { text+=digits[byte>>4];text+=digits[byte&15]; }return text; // SHA256의 원래32BYTE를 소문자 hex로 보존한다.
    }
    static std::string Of(std::span<const std::uint8_t> bytes) { Digest digest;digest.Feed(bytes);return digest.Finish(); } // 독립 프레임 하나의 바이트 해시다.
private:
    static void Check(NTSTATUS status) { if (status<0) throw std::runtime_error("Smacker 검증 SHA256 장치 실패"); } // 실패를 빈 해시로 바꾸지 않는다.
    BCRYPT_ALG_HANDLE algorithm_{};BCRYPT_HASH_HANDLE hash_{};std::vector<std::uint8_t> object_; // provider/현재 해시/필요한 실제 작업 메모리다.
};
}

// 유니코드 파일 경로를 유지하며 원시 출력은 새 전용 폴더에만 생성한다.
int wmain(int argc,wchar_t** argv) {
    try {
        if (argc!=3 && argc!=4) { std::cerr<<"사용법: netstorm_smacker_dump <SMK 파일> <새 출력 폴더> [교체 디코더 DLL]\n";return 2; }
        const std::filesystem::path source(argv[1]),output(argv[2]);if (std::filesystem::exists(output)) throw std::runtime_error("Smacker 최초 출력 폴더 덮어쓰기 금지");
        auto decoder=netstorm::client::SmackerDecoder::OpenFile(source,argc==4?std::filesystem::path(argv[3]):std::filesystem::path{});if (!decoder) throw std::runtime_error("Smacker 실제 파일/디코더 열기 실패");const auto info=decoder->Info();std::filesystem::create_directories(output);
        std::ofstream frames(output/L"frames.tsv",std::ios::binary);frames<<"# frame\trgb_sha256\tindex_sha256\tpalette_sha256\n";Digest video;std::array<Digest,7> audio;std::array<std::uint64_t,7> audioBytes{};std::uint32_t count{};
        for (;;) { // 실제 마지막 프레임까지 한 순회만 해제하며 게임 시계/상태/창은 실행하지 않는다.
            const auto status=decoder->Next();if (status==netstorm::client::SmackerFrameStatus::Done) break;if (status==netstorm::client::SmackerFrameStatus::Error) throw std::runtime_error("Smacker 실제 프레임 해제 실패");
            const auto pixels=decoder->Pixels();const auto palette=decoder->Palette();std::vector<std::uint8_t> rgb(pixels.size()*3);
            for (std::size_t pixel=0;pixel<pixels.size();++pixel) for (std::size_t channel=0;channel<3;++channel) rgb[pixel*3+channel]=palette[std::size_t(pixels[pixel])*3+channel]; // 팔레트의 원래 RGB만 펼치고 색 변환/보정은 하지 않는다.
            frames<<decoder->FrameIndex()<<'\t'<<Digest::Of(rgb)<<'\t'<<Digest::Of(pixels)<<'\t'<<Digest::Of(palette)<<'\n';video.Feed(rgb);++count;
            for (std::uint32_t track=0;track<7;++track) { const auto pcm=decoder->Audio(track);audio[track].Feed(pcm);audioBytes[track]+=pcm.size(); } // 모든 트랙의 실제 PCM 순서를 유지한다.
            if (status==netstorm::client::SmackerFrameStatus::Last) break;
        }
        if (decoder->Next()!=netstorm::client::SmackerFrameStatus::Done) throw std::runtime_error("Smacker 마지막 뒤 EOF 불일치");
        std::ofstream meta(output/L"metadata.json",std::ios::binary);meta<<std::setprecision(17)<<"{\n\"width\":"<<info.width<<",\"height\":"<<info.height<<",\"headerFrames\":"<<info.frameCount<<",\"decodedFrames\":"<<count<<",\"microsecondsPerFrame\":"<<info.microsecondsPerFrame<<",\"yScaleMode\":"<<std::uint32_t(info.yScaleMode)<<",\"videoSha256\":\""<<video.Finish()<<"\",\"audio\":[\n";
        for (std::size_t track=0;track<7;++track) { const auto& format=info.audio[track];if (track) meta<<",\n";meta<<"{\"track\":"<<track<<",\"exists\":"<<(format.exists?"true":"false")<<",\"channels\":"<<std::uint32_t(format.channels)<<",\"bits\":"<<std::uint32_t(format.bitsPerSample)<<",\"rate\":"<<format.sampleRate<<",\"bytes\":"<<audioBytes[track]<<",\"sha256\":\""<<audio[track].Finish()<<"\"}"; } // 트랙별 형식과 전체 PCM 해시를 분리해 남긴다.
        meta<<"]\n}\n";std::cout<<"decoded "<<count<<" frames\n";return 0;
    }catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
