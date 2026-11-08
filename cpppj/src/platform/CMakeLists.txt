# 플랫폼 계층 (새로 쓰는 코드 — 원본에 대응하는 소스가 없다)
# 원본이 직접 부르던 Win32·DirectDraw·DirectSound·네트워크 API 를 대신해
# Windows 10/11 과 GUI 환경의 Linux 에서 창·화면·입력·소리·시계·파일을 제공한다.
# o/ 와 client/ 의 다시 만든 코드는 OS API 를 직접 부르지 않고 이 계층만 쓴다.
# 사용할 라이브러리는 아직 정하지 않았다 (../../docs/cpp-build.md 의 "정하지 않은 것").
target_sources(netstorm PRIVATE
    Console.cpp Console.h
    Bitmap.cpp Bitmap.h
    FileOutput.cpp FileOutput.h
    PortableStartup.cpp PortableStartup.h
    UserDataRoot.cpp UserDataRoot.h
    Json.cpp Json.h
    Png.cpp Png.h
    AudioLoopback.cpp AudioLoopback.h
    FrameRecorder.cpp FrameRecorder.h
    LegacyRegistryValue.cpp LegacyRegistryValue.h # Config의 실제 legacy 레지스트리 기본값 조회를 클라이언트/R이 공유한다.
    LanIpv4Name.cpp LanIpv4Name.h # 명시 LAN hostname의 첫 IPv4 조회와 개별 WSA 참조를 클라이언트/R이 공유한다.
)

target_link_libraries(netstorm PUBLIC advapi32) # RegOpenKeyA/RegQueryValueA와 성공한 키 반납의 실제 Windows 장치를 연결한다.
