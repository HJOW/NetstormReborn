# webpj 경고 메시지 공장 WarningMessages (WB04-E454, 2026-11-14)

기준선 `1f00a25a`의 `WarningMessages.cpp/.h`를 `www/ui/warning-messages.js`로 이식했다. `RawMenuTree`는 현재 create/life/collection/tree/lifetime과 token/node/changed를 빌리는 계약이며, menus의 textBackend/styleBackend/setPalette/setExternalGumps를 같은 실제 장치에 연결한다. 별도 표시 목록이나 글자/측정 저장소를 만들지 않는다.

## 구현과 기준선 뒤 판정

같은 MessageBoard 20칸과 MessageGump를 사용한다. 생성→현재 기본/파생 노드→Style 파싱/측정→현재 root 부착, 영구 Pause·0.1초 시간·10초 Warning, 벽 시각 Idle→Kill→Cleanup/실제 Text scalar 소멸→파생 저장소 반납 순서다. Text 생성 전 실패는 미등록 파생만 반납하고, 생성 후 실패는 실제 GumpLifetime.destroy와 TextGump의 head 정리/등록 해제를 거친다. root 교체에서 serial 표를 초기화하지 않으며 조회는 현재 root의 비dead 직계 자식만 읽는다. Draw 2번 공급자도 같은 TextGump를 호출한다.

기준선 뒤 `a60d547f`·`37dcdfc2`·`53df9e61`·`8d45566d`를 읽고 **② 공통 상태 안내 공장/메서드**를 반영했다(base `8d45566d`). 원본 C++ 공장은 종류 6/8/10을 아직 연결하지 않았으므로 MessageGump 지원 여부와 별개로 같은 명시 오류를 유지한다. 실제 온라인/메타 발생원이나 전송을 새로 연결하지 않는다.

## 검증과 발견

신규 **13건 통과**. 실제 GumpCreate/Tree/Lifetime/Collection·TextGump·StyleText·MessageBoard·MessageGump와 GameClock을 공유한 조립 검사다. 20개 한도·정확한 10초 만료·Pause 영구성·게임 시계 정지 중 벽 시각 만료·현재 팔레트/크기·혼합 형제·생성 중 다른 Gump의 재진입·생성 실패의 실제 Style 반납·중복 시간 안내·NULL/빈 본문·직계 링크 순환·미연결 종류와 공통 Draw 공급자를 확인했다. 글자 측정 장치는 독립 폭/높이 입력이며 실제 원본 글꼴 픽셀 비교는 아니다.

보고서는 C++ `defaultfloat/setprecision(17)`을 따른다. JavaScript `toPrecision(17)`은 `1.00000762939453125`를 `1.0000076293945313`으로, C++는 `1.0000076293945312`로 출력한다. IEEE 비트의 정수 유리수에서 짝수 동률 반올림과 지수/부호0/비정상값을 구현했고 **명시18+고정LCG1000=1,018개**를 별도 C++ 출력과 직접 대조했다. fixture는 `tests/fixtures/warning-report-numbers.tsv`, 해시는 manifest의 `warning-report-numbers`다. **원본 10.78 PE 관측 자료가 아니라 현행 C++ 표준 스트림 출력**이다.

최초 조립 검사는 검사 어댑터가 존재하지 않는 `lifetime.state`를 빌려 실패했다. 기존 공통 Text 검사 장면에서 실제 `GumpLifetimeState`를 노출해 같은 객체를 연결했다. 숫자 프로브의 최초 컴파일 호출은 cmd 중첩 인용 오류로 컴파일 전 실패했고, 분리한 배치 호출로 수정해 MSVC 14.51.36231 x64에서 생성했다. 원본/기존 fixture와 cpppj 파일은 바꾸지 않았다.

전체 회귀는 Node v26.13.0·v22.23.3에서 각각 **4,691건 통과, 실패 0**이며 대응표 984행 오류 0이다. Node v24는 이 PC에 없어 실행하지 못했다. 실제 RawMenuControls/RawButtonControls 허브·원본 글꼴 픽셀/클립·실제 메뉴 가림/페이지 생성·Tell 명령 연결은 해당 소비자 조립에서 대조한다. 게임/브라우저 실행은 없으며 실제 미션 플레이는 아직 불가다.

## 숫자 fixture 재생성

`tools/warning-report-number-probe.cpp`는 선택적인 정적 검증 도구다. 웹 런타임/Node 검사에 C++ 빌드나 이 실행 파일이 필요하지 않다. C++20 MSVC 개발자 셸에서 `cl /nologo /utf-8 /EHsc /std:c++20 warning-report-number-probe.cpp /Fe:warning-report-number-probe.exe`로 빌드한 뒤 실행하면 인자 없이 1,018행 TSV를 stdout으로 출력한다. 실행 파일/obj는 세션 임시 경로에 두고 커밋하지 않는다. fixture를 갱신할 때 CRLF를 LF로 정규화하고 manifest 해시를 갱신한다.
