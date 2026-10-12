# 다중 줄 TextGump 생성·버퍼 소유·부착/입력 연결

작업일: **2026-10-13**, PC: **HJOW-Athlon / 192.168.0.94**. AGENTS.md·두 LEFT_JOBS를 읽고 마지막 디컴파일 PC가 같은 PC임을 확인했다. 이 PC의 창/게임 실행 허용을 반영했으며 정적 분석과 빌드로 진행했다. 두 Ghidra 프로젝트에서 함수 **9/11개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-13.**

## 구현과 원본 근거

`ScrollingTextGumpWidgets`는 앞 단계의 표시/스크롤 `ScrollingTextGump`, 실제 `ScrollGumpWidgets`, 기본 생성/트리/overlay/수명/입력/부모 통지를 같은 노드에 연결한다. `StyleTextBufferStore`는 실제 버퍼 생성자/소멸자와 같은 상태/handler를 보관한다. **전체 정상 생성·소멸·Attach/Handle/Broadcast 경로**를 구현했으며 생성자 예외 시 원본 EH 정리와 독립 동작 대조는 후속이다.

| 처리 | 패치 10.78 | CD 10.72 |
|---|---|---|
| 정상 생성 | 004bc060 | 004b5c30 |
| 파생 소멸 / scalar 삭제 | 004bc890 / 004bc930 | 004b5e70 / 004b7e70 |
| 가상 Attach | 004bc6b0 | 004b6bd0 |
| 가상 Handle | 004bc450 | 004b6560 |
| 프레임 Broadcast | 004ba5e0 | 004b6910 |
| overlay 등록 | 004653c0 | 00494410 |
| 버퍼 생성 / 소멸 | 004bbe40 / 004bbf20 | 004b50a0 / 004b5190 |

CD의 대기 요청/기본 소멸 보조도 내보냈다. 누락된 높이의 `_ftol` 입력·프레임 드래그·폭 비교의 x87 분기는 실제 명령으로 판독했다. 기존 가상 Draw 주소는 앞 단계의 실제 표와 동일하다.

## 생성과 버퍼 저장소

기본 `(0,0,0,1)` 등록 뒤 파생 표·보조 필드 0·signed 정렬 BYTE **46**을 설정한다. `new(0x38)`의 별도 버퍼 토큰에 실제 **128칸** 버퍼 생성자를 호출하고 포인터/+a8 소유 값 1을 저장한다. 버퍼는 Gump 등록 표에 넣지 않으므로 등록 수/serial을 증가시키지 않는다.

현재 문맥은 `{0,font,color,0,0,0,1,1}`이다. 폭/높이의 기본 필드는 float32로 저장하고 배율 계산은 저장 전 FILD 정수 값을 사용한다. 마지막 배치 폭/scroll/콜백 필드는 0, flags에는 **0x400a00**을 더하고 기존 `GumpLifetime::SetOverlay(true)`를 호출한다. 대기 요청 -1/추가 여백 0 뒤 `new(0xd8)` Slider를 현재 타입/프레임 전역으로 **9×100** 생성한다.

Slider를 숨기고 **저장한 최초 폭-현재 Slider 기본 폭**의 우측 위치에 y 0/mode 1로 가상 부착한다. 맨 아래 요청과 재그리기 후 현재 textFlags에서 bit 2/4를 지우고 **0x18**을 더하며 metadata를 0으로 쓴다. 미작성 +ac/이전 dragY와 그 밖의 기존 textFlags 비트는 보존한다.

`StyleTextBufferStore`의 레코드는 unique_ptr로 주소를 유지하며 handler가 같은 record.state를 참조한다. `Binding`이 이 상태/처리기를 기존 Core에 직접 공급한다. 소멸은 실제 완료/표시 조각 반납→호스트 토큰 free→레코드 반납 순서다. **원본의 미완료 끝 칸 삭제 누락도 유지한다.** 필요한 경우 호출자는 LF로 완료하거나 원래 소유 조각을 먼저 반납해야 한다. 현재 버퍼의 재귀 반납과 출력 중 표시 조각/줄 반납·버퍼 교체는 지원하는 수명 계약이 아니다.

## 소멸과 재부착

파생 소멸은 현재 버퍼 포인터를 저장하고 소유 값이 비0일 때만 `Destroy/free`한 뒤 현재 buffer 필드를 0으로 쓴다. **빌린 버퍼는 포인터도 유지**한다. 기본 scalar 소멸자는 기존 공통 수명 처리기로 위임하며 자식 Slider와 그 표시 자식은 기존 `Destroy/Cleanup`의 자식 우선 순서로 먼저 지운다. 버퍼는 자식 Gump가 아니다.

가상 Attach는 기본 트리 부착→현재 Slider Push→현재 부모 기본 높이/Slider 배율 높이→visible→우측 mode 1 부착 순서다. Slider는 **textFlags bit 2와 bit 4가 모두 있을 때만 표시**한다. 대기 요청이 -1일 때만 현재 scroll을 요청하고 마지막으로 폭 변화 dirty를 처리한다.

폭 비교는 패치의 FUCOM/C2·C3 parity와 CD의 C3 검사가 다르다. 패치는 **NaN도 변경**으로 처리해 dirty/저장하고, CD는 NaN 비교에서 두 갱신을 생략한다. ±0은 두 판본 모두 동률이다. 이 동작을 현재 필드 재조회와 함께 유지한다.

## Slider·태그·마우스 입력

Handle은 modifier/release를 보관하고 사건 분류에 `0x03ffffff`을 적용한다. 사건 전역은 원본 문자열 초기화 뒤의 실제 ID를 공급해야 한다. Slider 변화/step은 기존 Slider 처리기와 동일한 ID를 사용한다.

- Slider change의 발신 serial이 현재 Slider와 다르면 원본 assert 계약으로 거부한다. 일치하면 현재 position을 스크롤 요청에 저장하고 재그리기를 통지한다. 반환값은 0이라 더 위 부모 전달은 가능하다.
- Slider step 1/2는 **+5/-5**, 3/4는 **+기본 높이/-기본 높이의 CRT 절삭**을 상대 요청으로 전달하고 1을 반환한다. 모르는 step 값은 소비하지 않는다.
- refresh 사건은 현재 textFlags dirty bit 1을 세우고 `Redraw(false)`를 호출한다.
- 왼쪽 사건은 bit 8이 있을 때만 기존 HitTest를 호출한다. 선택 조각의 T 태그가 비0이면 누름에서 부모에 leftTag/현재 serial/현재 태그를 보낸다. 빈 영역/태그 0이면 누름에서 dragY·프레임 통지 bit 0x100을 설정해 기존 Capture를 호출한다. 이 분기의 release는 통지 비트를 지운다. 원본처럼 태그 release 분기에는 별도 해제를 추가하지 않는다.
- 오른쪽 사건은 bit 16이 있을 때 누름에서 선택 조각이 존재하면 부모에 rightTag를 보낸다. **태그 0도 보낸다.** 성공은 1, release/빈 영역은 0이다.
- cursor 사건은 링크 태그면 **16**, 그 밖에 bit 4가 있고 현재 Slider page < maximum-minimum이면 **15**, 나머지는 **1**을 선택한다. 실제 커서 선택은 공급 경계다.

현재 HitTest는 Layout/대기 스크롤을 자동 적용하지 않으며 엄격한 조각 위 경계와 half-open 화면 범위를 유지한다. 조각의 metadata는 문자열/그림 구분 없이 같은 공통 표에서 읽는다.

## 프레임 드래그와 분배 연결

Broadcast는 **폴링하지 않으며 event/x도 읽지 않는다.** bit 4가 있고 전달 y-이전 y의 DWORD 차이가 비0일 때 이전 y를 먼저 갱신하고 반대 방향 상대 스크롤을 요청한다. 현재 대기 요청에 누적하지 않고 원본처럼 현재 scroll을 기준으로 계산한다. 프레임 통지 bit의 필터링은 기존 `GumpFrame`이 맡는다.

`BindScrollingTextBuffers`는 같은 Store를 기존 표시/Append/Layout에 공급한다. `BindScrollingTextGumpWidgets`는 Attach·이벤트 재귀/부모 Bubble·직접 캡처/overlay·프레임·scalar를 실제 Widget에 분배하고 기존 버튼/Slider/메뉴 처리를 보존한다. 앞선 `BindTextGumpDraw`는 같은 Core Draw를 계속 사용한다. 분배 콜백은 처리기 생성 전에 구성하고, resolve의 현재 처리기 포인터를 생성 뒤 연결할 수 있다. 버퍼/조각/기본 노드/Slider는 모두 같은 저장소를 사용해야 한다.

## 빌드·자료·후속

라이브러리/`NetstormCpp` **Release 최종 빌드 경고 0·오류 0**. SOURCE_MAP 생성/내용 차이 없음. **새 테스트 추가/실행·전체 CTest·독립 PE/C++ 생성/소유/부착/태그/커서/드래그/픽셀 대조·게임/GUI 실행은 미수행**이다. 원본 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 앞 단계 기록이며 이번 변경의 검증 수치가 아니다.

자료: `tools/ghidra/scrolltext-functions.json`, `tools/analyze_scrolltext.py`, `cpppj/recovery-scrolltext-evidence.json`, `extracted/scrolltext/`, `extracted/scrolltext-build.log`. 실제 함수/명령/가상 슬롯/정렬/사건 전역 주소와 현재 생성·소유·표시·입력 연결 해시를 저장한다.

후속은 W 명령/전체 다중 줄 스타일 파서·버퍼 교체/추가 설정·메뉴/Form/자동 raw UI와 이번 생성/소유/태그/드래그·앞선 텍스트/버퍼/Slider/픽셀/파서의 독립 원본/C++ 대조다. 실제 ID/타입 전역·allocator 토큰·커서 공급은 상위 조립 계층 책임이다. 생성자 EH 정리·극단 x87 80비트 값의 전체 대조도 남았다. **Client/UberGump는 기존 메뉴 어댑터이며 raw 자동 GUI와 건설·미션 완주는 미완료다.**

두 보호 PE SHA 불변. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시 없음.
