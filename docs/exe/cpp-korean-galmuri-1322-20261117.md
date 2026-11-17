# cpppj 1322 — 한국어 글꼴 갈무리11 전환

2026-11-17, HJOW-Athlon. 사용자 요청에 따라 cpppj의 한국어 글꼴을 D2Coding에서 제공된 갈무리11 일반/굵은 글꼴로 바꿨다. 마지막 디컴파일 호스트와 현재 PC가 같으며 디컴파일 산출물을 수정하거나 다시 생성하지 않았다. dotnetpj·webpj의 코드와 인계는 이번 CP 작업 범위에 포함하지 않는다.

## 글꼴 선택과 실제 픽셀

`fonts/Galmuri11Bitmap-Regular-2.40.4.ttf`와 `Galmuri11Bitmap-Bold-2.40.4.ttf`의 내부 family는 `Galmuri11Bitmap`이다. 두 파일의 `glyf` 테이블은 길이0이며 실제 그림은 수평 1비트 `EBLC`/`EBDT` strike에 있다. 제공 파일의 ppem은16이고 한글 칸의 전진 폭은12, 원래 글자 그림의 폭은11이다. family만 변경한 Win32 관측에서 `AddFontResourceExW`는 두 파일 모두0을 반환했고 `CreateFontW`/`GetTextFaceW`의 실제 선택은 굴림체였다. 따라서 GDI의 대체 글꼴을 갈무리11 적용으로 간주하지 않는다.

새 `client/EmbeddedBitmapFont`는 외부 TTF의 Unicode cmap4, EBLC index1/2와 EBDT image2/5를 직접 읽는다. 이 범위는 제공된 두 파일에 실제 있는 형식이며 범용 OpenType 렌더러 구현을 뜻하지 않는다. 모든 읽기에 테이블 경계를 검사하고 손상/미지원 형식은 진단 예외로 거부한다. 비트맵 테이블이 없는 기존 일본어 TTF는 기존 GDI 경로를 계속 사용한다. 형식 근거는 Microsoft의 [EBLC](https://learn.microsoft.com/en-us/typography/opentype/spec/eblc), [EBDT](https://learn.microsoft.com/en-us/typography/opentype/spec/ebdt), [cmap](https://learn.microsoft.com/en-us/typography/opentype/spec/cmap) 명세다.

`BitmapFont::ExtendedGlyph`는 이 픽셀을 기존 VFX 글리프와 팔레트 잉크100으로 만든다. 작은 슬롯12·13·14와 제목20은 원래 strike를 축소하지 않고 사용하며 슬롯48은 정수3배로 그린다. 측정과 그리기는 같은 칸 폭을 사용한다. 일반/굵은 파일을 슬롯의 실제 weight로 선택하며 기울임·밑줄·취소선과 그림자/외곽선은 기존 출력에 연결한다. 영어 `.chfnt`의 바이트와 일본어의 글꼴 선택 계약은 유지한다.

굵은 파일에는 옛 호환 자모 `U+3165..U+318E` 42자가 없고 일반 파일에는 있다. 굵은 파일의 cmap에 없는 문자는 같은 폴더의 공통 설정 일반 파일을 사용하며 시스템 글꼴/합성 굵기로 바꾸지 않는다. 현대 한글11,172자와 현대 호환 자모는 두 파일 모두 자체 글리프를 사용한다.

## 다음 글꼴 변경 위치

**`cpppj/assets/korean-font.json` 한 곳의 `family`·`regular`·`bold`·`license`를 수정하고 다시 빌드한다.** CMake가 `KoreanFontSettings.cpp`를 생성해 `kKoreanFontFamily`·일반/굵은 파일명·라이선스 상수를 제공한다. C++ 파일 탐색·Client 설정과 `package-cpppj.ps1`, `tools/verify_cpp_package.py`가 같은 JSON을 사용한다. 생성 CPP에는 이름 문자열만 들어 있으며 글꼴 파일 바이트는 들어 있지 않다.

배포판은 `Netstorm.exe` 옆 `fonts/`에 두 TTF와 `Galmuri11-LICENSE.txt`를 포함한다. D2Coding TTC 최신 버전 탐색과 별도 family 문자열 지정은 제거한다. 외부 파일 캐시는 프로세스 수명이므로 실행 중 파일 교체 대신 게임을 다시 시작한다. 다른 형식의 글꼴을 선택할 때는 해당 렌더러 지원과 작은 크기의 실제 출력도 확인한다.

## 검증

구현 전 기준선은 `netstorm_korean_text_tests` **40/40 통과**, 격리 사본의 한국어 사제 정보/F1 화면은 종료0·설치 자료 사본 불변이었다.

| 검사 | 결과 |
| --- | --- |
| Release 빌드 | `NetstormCpp`·`NetstormRoot`·`netstorm_korean_text_tests` 종료0. 최종 증분 빌드 경고0·오류0. 첫 재컴파일에는 수정하지 않은 `TextClipboard.cpp`·`DisplayTextInputTests.cpp`의 C4244가 각각2건 있었다. |
| 집중 검사 | **47/47 통과**, 신규7건. 독립 관측한 일반 '한'42픽셀/굵은 '한'66픽셀·원형 행, 두 파일의 현대 한글22,344글리프, 작은 슬롯/3배 확대·동시 밑줄/취소선·글꼴 변경 무효화·손상 거부·옛 자모42자의 실제 일반체 대체·일본어 GDI 출력·기존 영어 캐시 보존을 포함한다. 기존 한국어/입력/일본어40검사와 원본 fixture를 유지한다. |
| 첫 실패와 교정 | 신규 원형 검사 첫 실행은45건 중1건·42단언 실패였다. 실제 굵은 파일에 옛 자모가 없다는 독립 cmap 관측으로 원인을 확인했다. 파일의 실제 부재 기대를 명시하고 제품의 같은 일반체 대체와 별도 출력 회귀를 추가했다. 기존 fixture 기대값을 변경하지 않았다. |
| 실제 GUI | 한국어800×600 메뉴/옵션/캠페인 목록, 한국어1024×768의 일반 미션 사제 정보/F1·Tutorial1 브리핑, 영어 메뉴 대조, 압축 해제본 한국어 메뉴까지 **5프로세스 모두 종료0**, 오류 로그0. 메뉴·정보·브리핑의 획/줄바꿈/잘림을 화면으로 확인했다. stderr의 기존 State/인트로 자료·최초 캐시 부재 진단은 오류 로그와 구별한다. |
| ZIP·압축 해제 | **1,437항목**(원천1,430·생성4·런타임3) 전체 대조 통과. 일반/굵은 TTF와 OFL 동봉, 누락/금지 항목0·글꼴 파일 바이트의 exe 내 포함0·빌드 정보 SHA 일치. 압축 해제1,437파일/245,598,551바이트 일치, 실행 뒤 패키지 전체 바이트 불변. |
| 보호 | 원본1,392·CD425·기존 fixture988·AGENTS/README2 = **2,807파일 이름/해시 불변**. 미션 실행 사본도 불변이다. 사용자 Saved Games 대신 실행별 격리 사용자 폴더를 사용했다. |

이 검증은 글꼴 변경 범위이며 전체 솔루션/34미션/M8 최종 관문 통과로 확대하지 않는다. 제품 exe SHA-256은 `cb9d2514c47d0257bfc1ceb20a5945a00fedb9e1b6b0e66cc53cfa28886673c7`이다.

검증 ZIP: `extracted/cpp-galmuri-1322/NetstormReborn-11.01-cpppj-galmuri.zip`(205,877,500바이트), SHA-256 `a9677349ebc595b2eabc55baf774fd26e72c2925f002b978b52fe7630bc4c32e`. 커밋 전 작업본으로 생성한 검사 산출물이므로 BUILD_INFO의 이전 HEAD/dirty 표기를 최종 배포판 커밋 표기로 오인하지 않는다.

제품 화면: [한국어 메뉴](../screens/cpp-korean-galmuri-1322-20261117/menu.png) · [사제 정보](../screens/cpp-korean-galmuri-1322-20261117/priest.png) · [Tutorial1 브리핑](../screens/cpp-korean-galmuri-1322-20261117/briefing.png).

로컬 원시: `extracted/cpp-galmuri-1322/`의 빌드 로그·집중 검사·배포/보호 지문과 메뉴 관찰 자료, `extracted/cpp-help-1315/before-galmuri-1322/`의 변경 전 사제 정보/F1이다. 실제 자연 완주·희생/포획·특정 적 공격·MP·물리 IME/다른 DPI/Windows11·ARM은 이번 검증 범위에 포함하지 않는다.
