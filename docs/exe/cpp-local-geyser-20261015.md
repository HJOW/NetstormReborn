# Early1 로컬 간헐천 생성 부모와 실제 후보 복원

작성일과 마지막 정밀 디컴파일 PC는 **2026-10-15, VM-W11-Codex**다. AGENTS와 양 LEFT를 확인했고, `reverse-engineer-anything` 지침에 따라 보호 PE와 기존 정밀 프로젝트를 읽었다. 원본 게임이나 GUI는 실행하지 않았다. 보호 원본·AGENTS·dotnetpj는 변경하지 않았다.

`RawLocalGeyser`는 PATCH10.78의 `00486360 → 00486130 → 00486030/00485890` 중 실제 로컬 호출 인자 `(0,0,1)`을 복원한다. 기존 `GameRandom`, `RawFortFinishOwner::TimeSeed`, `RawSquidFinder`, 같은 raw SID 풀·TYPE·프레임·spot·Player 기록을 사용한다. 실제 factory/Pop/삭제와 `RawConstructionConfirm` 연결은 필수 하위이며, 생성 없이 최근 목록이나 자원만 갱신하는 성공 경로는 없다. Session의 실제 초기화 사슬 연결은 후속이다.

## 원본 근거와 범위

읽기 전용 선택 export는 `tools/ghidra/local-geyser-functions.json`의11함수다. 실행 명령은 `tools/ghidra/export_functions.ps1 -Name local-geyser -Edition originals -Refined`이며 기존 정밀 프로젝트에서 재분석 없이 읽었다. 결과는 `extracted/local-geyser/originals/creation.c`와 `functions.tsv`다.

| 주소 | 확인한 계약 |
| --- | --- |
| `00486360` | 설정/지역별 수량을 읽는 Player1~8 부모 |
| `00486130` | boss/fort 관문, 생성별100회, 삭제/확정/factory/Pop, 최근5개와 자원 감소 |
| `00486030` | 후보 구조체 초기화 |
| `00485890` | 난수점/반올림/영역/spot/기존 empty/현재 isle 프레임 무늬 |
| `0041daf0` | 높이 다음 폭의 local 난수점 |
| `0043fa60` | float 입력+0.5를 double floor로 넘기는 반올림 |
| `004bca20` | signed limit의 local PRNG |
| `004e49c0`, `004e62a0` | 실제 CRT 절삭/floor |
| `0048f5c0`, `0048f550` | Player의 현재 지역/배치 사각형과 PlayerId 조회 |

`00486130`의 정밀 C에서 지역 변수처럼 보이는 `local_14/local_10`은 후보 구조체 뒤의 `+ac/+b0`와 겹친다. 실제 `004861b0..004861bf`는 ctor 뒤에 **현재 emptyGeyser 타입과1**을 쓴다. 따라서 기본 ctor의0만 보고 기존 간헐천 재사용이나 프레임 무늬 검사를 생략하면 안 된다. 초기 원본 격리 실행은 누락된 재사용 조회에서 `00485af9`의 잘못된 메모리 읽기로 멈췄고, 해당 쓰기/호출 명령을 확인한 뒤 입력과 구현을 교정했다. 그 초기 실패는 정상 반환220개에 합산하지 않는다.

프레임 문자 getter `004ad680`, CRT 비교 `004e65ed/004ef510`은 실제 PE 바이트와 명령 범위를 추가 확인했다. 기존 TimeSeed/TYPE getter는 앞선 `fortfinishowner` 선택 export를 사용한다. 이 범위들은 새11함수 export 수에 다시 더하지 않는다. CD10.72는 이번 범위가 아니다.

## Player 부모와 생성 효과 순서

`Generate`는 local seed를 `0xfed`로 시작한다. `predictableGeysers` 설정의 기본값은1이며 현재 값이0일 때만 벽시각을 기존 `RawFortFinishOwner::TimeSeed`에 넘긴다. Player1~8은 같은 local 난수 상태를 이어서 쓴다. 원본 raw 기록은9×`0xb4`이며, 각 차례에서 현재 flags BYTE`+0x50 & 0x22`와 region DWORD`+0x78 !=127`을 읽는다. 앞선 효과가 뒤 Player 기록을 바꾸면 같은 호출에서도 반영한다.

eligible Player마다 먼저 현재 지역 사각형을 얻고, 그 다음 현재 수량 전역을 읽는다. Player1~4의 기본 수량은3,5~8은4다. 부모 입구에서는 boss/fort로 조기 반환하지 않는다. 각 `Populate`의 현재 boss/fort 관문이 판단하므로 설정/사각형 조회의 원본 순서도 유지한다. 실제 `0048f5c0`의 chunk/PlayerId/배치 사각형→chunk좌표×16→양쪽4확장은 아직 필수 `playerBounds` 경계이며 임의 사각형을 실제 지역 복원으로 주장하지 않는다.

`Populate`는 다음 순서다.

1. 현재 boss가 아니거나 fort 모드면 반환한다. 이때 최근 목록도 보존한다.
2. 음수 수량은 원본 진단을 호출한 뒤0으로 한다. 그 다음 **최근 생성 수를0**으로 쓴다.0개 요청도 목록을 초기화한다.
3. ctor 뒤의 재사용 타입을 한 번 보존하고, 성공 하나마다 같은 후보를 최대100회 호출한다.100회 모두 실패하면 이 요청 전체를 끝낸다. 상위 Player 순회는 계속한다.
4. 성공 좌표에서 **현재** emptyGeyser 타입을 다시 찾아 `flags=0x400000`으로 실제 파생 삭제한다. 후보의 보존 타입과 이 현재 전역 조회를 합치지 않는다.
5. 남은 수량을 하나 줄인다. `point+float(0.99989998340606689453125)`를 절삭해 읽은 현재 spot의 bit2가 없으면 `noIsland` 타입157을 `(x-2,y-2)`에 건설 확정한다. 나머지7인자는0이다. 이 호출은 실제 `00444590`이며 Terrain의 직접 생성으로 대신하면 안 된다.
6. 현재 geyser 타입122/flags0으로 실제 factory를 호출하고, 반환한 SID에 실제 가상`+90(x,y,0)` Pop을 호출한다. 생성 실패/필수 하위 누락은 명시 오류다.
7. Pop이 변경한 현재 최근 수가 signed5 미만일 때 SID를 추가한다. 최대5개이며 count밖 stale칸은 지우지 않는다.
8. 현재 자원 pool 정수를 **float로 한 번 저장**한 다음 현재 geyser TYPE의 cost를 빼고 원본 signed64 절삭의 하위 DWORD를 취해 최솟값0으로 쓴다. 큰 정수에서 float 정밀도 손실도 유지한다.

최근 목록 `00595094/00595098`은 전체 `AssetPostPopState.geysers`와 별개다. eligible Player마다 수를0으로 하므로 전체 부모가 끝나면 마지막 처리 Player의 최대5개만 남는다. eligible Player가 전혀 없으면 기존 목록을 유지한다. `Pool()/Recent()`는 상위가 같은 풀과 전용 목록을 연결했는지 검토할 수 있게 참조를 노출한다.

## 후보의 현재 공간 계약

원본 `0041daf0`는 double 차이로 계산한 폭/높이를 signed 절삭한다. 둘 다0이 아니면 높이 난수, 폭 난수 순서로 소비한다. signed limit이0 이하인 개별 축은 seed를 소비하지 않고0을 반환한다. 둘 중 하나라도 정확히0이면 두 난수를 모두 소비하지 않고 사각형의 left/top을 쓴다. 각 좌표를 float로 저장한 뒤 `floor(double(point)+float(0.5))`로 반올림한다. 더하기를 먼저 float로 줄이지 않는다.

반올림 좌표에서2를 뺀 첫칸이 사각형의 양끝을 포함한 범위 안에 있는지 확인한다. 그 첫칸부터 point까지 **x 바깥/y 안쪽, 양끝 포함3×3** 순서다. 매 칸은 현재 spot BYTE를 먼저 보존하고, 같은 raw 풀/공간 해시를 사용하는 `RawSquidFinder`에서 재사용 타입을 찾는다. 첫 exact 타입 일치가 있으면 현재 raw x/y를 결과점으로 써서 즉시 성공한다. 이때 보존한 spot 거부나 뒤 칸 검사는 실행하지 않는다.

재사용이 없으면 로컬 `(0,0,1)` 경로에서 spot 하위6bit가 하나라도 있으면 실패한다. 상위`0x40/0x80`은 이 검사에서 무시한다. 이어 현재 hash0 WORD SID의 raw 타입이 현재 isle 타입97이면 현재 signed frame의 첫 side코드 BYTE를 모은다.9칸의 문자열이 첫NUL까지 대소문자 무시 비교로 `FBICANGDH`와 같으면 거부한다. C++는 동일한 ASCII 정규화와 NUL종료를 적용한다. 최초의 원본 입력은 해당 C locale을 명시했으며 다른 locale 전체의 CRT 문자 분류까지 검증한 것은 아니다.

후보/성공 뒤 empty 조회는 기존 `RawSquidFinder`를 직접 호출한다. TYPE flags2의`&6`에 따른 surface 생략과 첫 exact 타입 순서를 유지한다. spot은 원본의 `x+256*y` 선형 주소로 읽으며 좌표별 임의 clip은 하지 않는다. host 물리 배열/프레임/SID 범위를 넘는 입력은 명시 오류로 드러낸다. 일반 exclusion 사전 구축과 다른 allow 모드는 이번 공개 API에 없으며 빈 성공으로 처리하지 않는다.

## 독립 실제 PE 검사와 통합 결과

`tools/decomp_local_geyser.py`는 C++ 구현/출력을 읽지 않고 보호 PE의 허용된 함수 기계어를 Unicorn에서 실행한다. 저장 레지스터, 정상 EIP/ESP, x87 제어어`027f`와 TOP0을 확인한다. 실제 유한수 floor fastpath와 CRT 대소문자 비교를 실행하며 OS 호출은0이다. 원본 게임 프로세스나 GUI 실행과는 다르다.

새 고유 입력은 **220개**, 정상 반환도220개다. 원본 실제 함수 진입은 후보621, 난수점621, local PRNG1,192, 반올림/floor각1,242, 절삭2,637, 생성 요청117, ctor87, 전체 부모31, TimeSeed15, TYPE getter206, 프레임 getter54, 문자열 비교160회다. 호출 횟수를 고유 입력 수에 더하지 않는다.

입력은5시드와5사각형, 정수/분수/0폭/음수폭, spot0/1/2/`0x40`/`0xc0`, 수량-1/0/1/3/6, boss/fort, 현재 자원-1/0/1999/2000/16,777,217/INT_MAX, 기존 empty 유무, Player 활성/지역127/1~4와5~8, deterministic 옵션 유무, 현재 대소문자 프레임 무늬를 포함한다. Pop이 현재 pool/cost/최근 count를 바꾸는 입력과 Player 사각형 효과가 다음 Player/수량을 바꾸는 입력도 포함한다. 입력 경계의 모든 조합을 전수검사한 것은 아니다.

native의 typedFind, factory/Pop/삭제, 건설 확정, Player 사각형, 설정/벽시각, 진단과 CRT thread locale은 명시 입력/관찰 경계다. 생성/확정 하위 몸체를 실행한 것으로 세지 않는다. C++ 쪽은 typedFind를 같은 실제 `RawSquidFinder`로 실행하며, 원본의 명시 Find 입력에 대응하는 raw/hash 공간을 구성한다. factory/Pop 훅을 실제 게임과 연결했다는 주장도 하지 않는다.

`LocalGeyserTests.cpp`의 새2검사는220행 각각의 후보 반환/좌표 비트, local seed, 최근 수와7칸 전체, 현재 pool/수량, 생성 수, 모든 외부 호출 인자/순서 및 필수 factory 누락 거부를 대조한다. 기대값의 난수/후보/반복/자원 계산을 C++ 구현으로 다시 만들지 않는다.

- 루트가 등록한 통합 **Release 게임/검사 빌드: 경고0·오류0**. 로그: `extracted/local-geyser-root-build.log`.
- 집중 **2개·실패 체크0**. 로그: `extracted/local-geyser-root-focused.log`.
- 최종 `python -X utf8 tools/decomp_local_geyser.py --verify`: **220행의 파일 SHA/입력 계획/정상 반환 수 감사 통과**. 로그: `extracted/local-geyser/final-verify.log`. 재실행 관찰 수에 더하지 않는다.
- 8파일의 UTF-8 읽기와 공백 오류0을 확인했다. 최초 fixture164행의 마지막 빈 trace열은 뒤의 포맷 정리에서 `-` sentinel로 저장하도록 도구/fixture/검사만 바꿨다.220행 모두26열이며 입력·실제 출력 의미는 같다. 동일220입력을 재생성/감사하고 고유 수에 다시 합산하지 않는다. 위 집중2개는 포맷 정리 전 결과이며 최종 재빌드/집중 재검사는 상위 기록을 따른다. 실제 게임 모듈 소스는 바뀌지 않았다. 전체 게임 검사/실제 미션 완주 검증은 이번 집중2개와 구별한다.

보호 PE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`다. 독립 fixture/해시 보고서는 `cpppj/tests/fixtures/local-geyser-x86.tsv`, `cpppj/local-geyser.json`에 있다.

## 상위에 남은 연결

실제 Session은 같은 Player9/TYPE188/프레임188/spot/hash/SID 풀과 **전용 최근5목록**을 제공해야 한다. `playerBounds`의 실제 chunk/region 계산, 같은 풀의 삭제/`RawConstructionConfirm`/factory/Pop을 연결해야 Early1C의 실제 생성이 완성된다. 전체 간헐천 장부와 최근 목록을 합치거나, noIsland를 Terrain의 임시 사각형으로 대신하면 안 된다.

이번 단위는 실제 Early Missions/캠페인1-1~1-2 완주, 건설/다리/수확·운반, 승패 후 진행 저장, UI/그래픽/음향 동일성의 완료 검증이 아니다. 기존 Windows10/11의 전체화면 DirectDraw 문제는 요청대로 제외한다.
