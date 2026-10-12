# Slider 값·부착·손잡이 입력/드래그 복원

작업일: **2026-10-13**, PC: **HJOW-Athlon / 192.168.0.94**. AGENTS.md와 두 LEFT_JOBS를 확인하고 현재 PC와 마지막 정밀 디컴파일 PC가 같음을 확인했다. 두 Ghidra 프로젝트에서 **11/11개** 함수를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-13.** 사용자의 이 PC 창/게임 실행 허용을 반영했으며 이번 단계는 정적 분석과 빌드로 진행했다.

## 복원 범위

`ScrollGump.h/.cpp`에 Slider 값/범위/페이지·자식 부착·부모 사건 통지와 손잡이 선택/캡처/폴링/세로 드래그를 구현했다. 기존 생성·트리·수집·사건·입력·프레임 처리기와 **같은 기본/파생 노드**를 사용한다. `ScrollGumpNode`는 원본 +a0/+a4/+a8/+ac/+b4/+b8/+c8..d0 필드를, `ScrollGumpThumbNode`는 손잡이 사각형 +c0..cc와 이전 y/x를 표현한다. 이전 x는 패치에만 존재한다.

**전체 Slider/손잡이/화살표 생성자와 픽셀 Draw는 이번 구현에 포함되지 않는다.** 호출자는 등록/초기 크기·방향·step·세 자식 토큰을 먼저 준비하고 기존 가상 Push/Attach/Redraw와 실제 왼쪽 버튼 폴링을 공급해야 한다. 표시용 노드를 임의로 새로 만들어 현재 트리와 분리하지 않는다.

| 구현 몸체 | 10.78 패치 | 10.72 CD |
|---|---|---|
| 위치 지정 | 004a6050 | 00418700 |
| 페이지 지정 | 004a6320 | 004186d0 |
| 범위 지정 | 004a6350 | 004187a0 |
| 가상 Attach | 004a5cd0 | 00418170 |
| 세로 드래그 변환 | 004a6170 | 004184e0 |
| Slider Handle | 004a61c0 | 00418550 |
| 손잡이 Handle | 004a5ae0 | 004179c0 |
| 손잡이 Broadcast | 004a6430 | 00417ad0 |

나머지 세 함수는 손잡이 Draw/Slider 생성/손잡이 생성의 **참고 내보내기**다. 실제 PE 가상 표로 Attach/Input/Broadcast 주소를 확인했고, C 디컴파일에서 누락된 `_ftol` 입력·손잡이 좌표·DWORD 곱·x87 비교는 원본 명령으로 판독했다.

## 값·페이지·범위

- `SetPosition`은 page <= maximum-minimum의 원본 assert 계약을 검사하고, minimum 및 maximum-page로 요청을 제한한 뒤 +a8에 저장한다. 산술은 DWORD 감김을 유지한다. page가 음수여도 임의 하한을 추가하지 않는다.
- 저장 후 현재 void bit 8이 없으면 **가상 Push → 현재 x/y → 가상 Attach(x,y,0,0)**을 호출한다. void가 있으면 `Redraw(false)`를 호출한다. 값이 이전과 같아도 재부착/재그리기를 생략하지 않는다.
- `SetPage`는 signed 범위 길이보다 큰 page만 줄이고 현재 위치를 다시 적용한다.
- `SetRange`는 끝점을 정렬하고 동률이면 maximum을 minimum+1로 만든다. page 제한 뒤 **현재 위치를 두 번 적용**한다. 패치는 page 제한을 인라인하며 CD는 `SetPage`를 호출한다. 첫 가상 콜백이 position/링크/크기를 바꾸어도 두 번째 적용은 현재 값을 읽는다.

## 자식 부착과 손잡이 좌표

Slider의 가상 Attach는 먼저 기존 기본 `GumpTree::Attach`를 호출한다. 현재 방향을 고른 뒤 짧은 축을 CRT 절삭해 화살표 두께를 저장하고, 감소 화살표→증가 화살표→트랙의 **현재 토큰**을 순서대로 Push한다. 각 크기를 기본/배율 필드에 저장하고 화살표와 트랙을 mode 0으로 부착한다. child scale과 곱할 때 기본 크기의 float32 저장 전 값을 사용한다. CD의 두 번째 화살표 두께는 별도 float32 저장을 거치므로 이 차이를 유지한다.

손잡이의 길이/위치는 다음 원본 식을 CRT 절삭한다.

```text
가용 길이 = 현재 긴 축 - float32(DWORD(절삭 두께 × 2))
페이지 픽셀 = trunc(page × 가용 길이 / DWORD(maximum - minimum))
위치 픽셀 = trunc(position × 가용 길이 / DWORD(maximum - minimum))
진행 축 첫 좌표 = 위치 픽셀 + 1
진행 축 끝 좌표 = 위치 픽셀 + 페이지 픽셀 - 1
교차 축 = [1, trunc(현재 짧은 축) - 1]
```

이는 **양 끝 포함** 좌표다. 원본은 위치 분자에서 minimum을 빼지 않는다. 빈/뒤집힌 손잡이도 임의 최소 길이로 넓히지 않는다. 범위나 실수 입력이 무효이면 원본 `_ftol`의 하위 DWORD 0 결과를 사용한다. 일반적인 double 중간값 복원이며 극단 x87 80비트 값과 CD Pentium FDIV 보정 경로는 아직 독립 대조하지 않았다.

## 화살표와 손잡이 입력

Slider는 사건의 하위 `0x03ffffff` 분류를 사용한다. 실제 click/repeat ID와 자식 serial을 대조하고 감소/증가 화살표이면 부모에게 step 인자 **2/1**을 보낸다. 부모가 0을 반환할 때만 현재 위치에서 step을 빼거나 더한다. 현재 위치가 바뀌었으면 change 사건/현재 Slider serial/값 0을 다시 통지한다. 손잡이의 step 사건은 인자를 그대로 부모로 전달하고 같은 변경 통지를 수행한다. Slider Handle 자체의 반환은 항상 0이어서 더 위의 부모 전달을 허용한다.

손잡이는 왼쪽 누름에서 현재 Bounds 기준 지역 좌표와 포함 사각형을 비교한다. 첫 좌표보다 작으면 step **4**, 끝 좌표보다 크면 **3**을 부모에 통지한다. 포함되면 이전 좌표를 저장하고 프레임 통지 bit 0x100을 세워 기존 `GumpInput::Capture`를 호출한다. release는 세로로 판정한 경우에만 통지 비트를 지운다. 두 판본의 x87 C0/C3 분기는 NaN 폭/높이도 세로로 처리한다.

Broadcast는 event/x와 폴링 반환 좌표를 읽지 않는다. 실제 왼쪽 버튼 폴링에서 release면 통지 비트만 지운다. 그 밖에는 전달된 y와 이전 y의 DWORD 차이가 비0일 때 이전 y를 먼저 쓰고 부모 Slider에 세로 변환을 요청한 뒤 change를 통지한다. 변환식은 **DWORD((maximum-minimum) × (delta+현재 손잡이 top)) / (baseHeight-2×baseWidth)**의 CRT 절삭이다. minimum을 더하지 않는다.

**패치는 가로 누름에서 캡처와 이전 x 저장을 수행하지만 Broadcast/변환은 y와 세로 함수만 사용한다. CD는 가로 누름을 1 반환으로 끝낸다.** 가로 위치 갱신을 새로 만들어 넣지 않았으며 두 판본의 제한을 유지한다.

## TextGump와 기존 가상 분배 연결

`BindScrollGump`는 Slider Attach/Handle, 손잡이 Handle/Broadcast를 기존 트리·이벤트 재귀/부모 Bubble·직접 캡처/overlay·프레임 분배에 연결한다. 다른 버튼/메뉴/기본 노드의 이전 처리기를 보존한다. 기본 부모 Attach는 직접 기존 트리 몸체를 호출해 자기 가상 분배 재귀를 피한다.

`BindScrollingTextSlider`는 기존 `ScrollingTextGumpHooks`의 range/page/position/현재 값 조회를 **실제 `ScrollGump`와 같은 노드**에 연결한다. TextGump가 매 호출 직전 현재 slider 토큰을 다시 읽는 순서는 기존 구현을 유지한다. 이로써 앞 단계의 네 Slider 값 경계에 실제 처리기를 공급할 수 있다. 전체 생성/Draw와 raw Client 자동 조립은 후속이다.

## 빌드·자료·후속

게임 라이브러리/`NetstormCpp` **Release 최종 빌드 경고 0·오류 0**. SOURCE_MAP 생성기로 ScrollGump 존재 상태를 갱신했다. **새 테스트 추가/실행·전체 CTest·독립 원본/C++ 동작/픽셀 대조·게임/GUI 실행은 미수행**이다. 원본 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 앞 단계 기록이며 이번 구현의 검증 결과가 아니다.

자료: `tools/ghidra/scrollgump-functions.json`, `tools/analyze_scrollgump.py`, `cpppj/recovery-scrollgump-evidence.json`, `extracted/scrollgump/`, `extracted/scrollgump-build.log`. 보고서는 실제 함수 범위/명령/가상 슬롯과 구현·연결 파일 해시를 기록하고 8개 구현 몸체/3개 참고 함수를 구분한다.

후속은 Slider/손잡이/화살표 전체 생성·FrameGump/화살표/손잡이 실제 Draw·다중 줄 TextGump 전체 생성/소멸·가상 Attach/Input·W 명령/전체 스타일 파서·메뉴/Form/자동 UI와 독립 PE/C++ 대조다. **Client/UberGump는 기존 메뉴 어댑터이며 raw 자동 GUI와 건설·미션 완주는 미완료다.**

원본 두 PE SHA 불변. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시 없음.
