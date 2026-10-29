# Netstorm용 libsmacker 빌드

이 폴더의 `smacker.c`, `smacker.h`, `smk_malloc.h`, `README`, `COPYING`은 [공식 libsmacker 저장소](https://github.com/greg-kennedy/libsmacker)의 commit `76094fb9c8e98bd5fac982c504e8d9aeff3ece01`에서 바이트 변경 없이 가져왔다. 상류 버전은1.2.0이며 LGPL2.1 라이선스/저작권 고지는 COPYING·원래 README·소스에 보존했다. provenance.json에 파일별 URL과 SHA256을 기록했다.

원본32비트 Smackw32.dll 대신 Windows10/11 x64에서 팔레트 영상과 PCM을 해제할 `NetstormSmacker.dll`을 만든다. 게임은 실행 파일 옆의 DLL을 명시적으로 불러온다. 이 DLL의 역할은 디코딩이며 MovieGump의 상태/시각/입력/표시/음향 재생은 Netstorm 쪽이 담당한다.

이 폴더만으로 교체 DLL을 빌드할 때는 CMake와 Visual Studio C 도구가 있는 환경에서 다음을 실행한다.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

생성된 `build/Release/NetstormSmacker.dll`을 클론 실행 파일 옆에 둘 수 있다. 게임이 별도 DLL을 사용하므로 라이브러리를 다시 빌드하고 교체할 수 있으며 게임 코드에 라이브러리 소스를 정적으로 합치지 않는다. 이 폴더의 CMakeLists.txt와 이 설명은 프로젝트에서 추가한 빌드 자료다.
