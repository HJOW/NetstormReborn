# 원본10.78 커서 미리보기 생성·재사용 복원

단위119 교정: 이 단위의 생성→Query/Prepare32행에는 실제 Activate 전이가 포함되지 않았다. 후속 두 번 연속 이동 사슬에서55a2ac/55a2e0/55a2dc의 공유 전역 연결과 기본 GUI 수신 이벤트를 교정했다. 기존522행은 보존했고 검사 장면만 공용 헤더로 옮겼다. [후속 근거](cpp-cursor-position-20261017.md)를 함께 참고한다.

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 일치한다. 단위118은 깨끗한 `a61a8a4`에서 시작했다. 이전 단위117의 실제 위치 판정573행/전체1,474검사와 커밋은 완료된 진행이다. 이번 작업에서 원본/클론 게임을 별도로 실행하거나 GUI 플레이를 검증하지 않았고, 새 Ghidra 내보내기도 없다. 보호 원본·추출 소스·AGENTS.md·dotnetpj는 수정하지 않았다.

## 생성·재사용 몸체

`RawCursorPreviews`는 **PATCH00445b30** 전체를 복원한다. CD10.72의 대응 함수는 **0043cfa0**이며 후속 분석을 위해 기록한다. 현재 생성자/연결기는10.78만 허용한다.

첫445440 위치 판정 뒤 **실제 첫 미리보기 SID가0인지**를 한 번 읽어 생성/재사용을 고른다. 현재 개수와 별개의 조건이다. Canon 생성 인자는 그 뒤의 현재 타입/인자/방향/위치다. 각 유효 칸은 실제 생성 또는 ordinal 슬롯을 사용해 buried8·프레임·하위WORD 인자·가상 Owner·조건부 공통 HP0·위치를 순서대로 쓴다. 외부 효과 뒤 현재 타입/인자/소유자/품질을 원본과 같은 시점에 읽는다.

원본 C 출력에서 생략된 화면 캐시는 실제 명령으로 보완했다. raw+14/18은 월드 float 위치, +22/24는 **`_ftol(x*16+0.5)`/`_ftol(y*11+0.5)` 하위WORD 화면 좌표**다. 지도 칸 정수 좌표가 아니다. 음수·WORD 감김·비유한 절삭의 indefinite 하위0도 실제 연산을 따른다.

다리 genus4는 원래 Form의 품질1/3을20/40 플래그로 바꾸고 현재 bridge 프레임 코드의 signed flags와 OR한 뒤 실제 FindFlags를 실행한다. variant는 unsigned BYTE이며 side/flags의 원래 바이트 해석을 보존한다. 프레임을 쓴 뒤 실제 raw 위치를 캡처하고 `floor(float+0.5)`로 임시 맞춘다. 맞춘 위치의 연결 객체를 만든 뒤 각 연결 객체에서 단정도로 저장한 delta를 빼고 화면 캐시를 다시 쓴다. 주 미리보기는 원래 위치로 복원한다.

연결 개수6/전체 개수30 진단과 출력 배열 범위는 명시적으로 확인한다. 잘못된 배열/슬롯을 호스트 메모리 산술로 처리하지 않는다. flags1의400은 같은 현재 위치에 frame8인 island/stalag 둘을 만든다. 재사용 슬롯의 ordinal에는 연결 객체 수를 더하지 않는다. 공간 Pop은 이 몸체에서 부르지 않는다.

## 같은 월드에 연결

Drop/Lifecycle와 기존 `BridgeConnectState`의 bridge/island/stalag 타입 전역을 그대로 공유하며 별도 추정 목록이나 좌표를 만들지 않는다.

- `BindCursorPreviews`는 수명의 실제 createPreviews 경계를 연결한다.
- `BindCursorPreviewPlacement`는 같은 풀의 실제 Query/Prepare 및 같은 Canon 공급자를 연결한다.
- `BindCursorPreviewWorld`는 같은 Factory/HP/RawBridgeConnect/프레임 표에 연결한다. 하위WORD 쓰기 뒤 실제 타입이 bridge일 때만 표면 진단을 호출한다. 파생 가상 Owner는 상위 월드가 반드시 공급해야 한다.

`RawBridgeConnect`에는 읽기 전용 Pool 조회와 **ConnectPreview의 즉시 SID 소비자**를 추가했다. 원본4210f0은 연결 객체마다 출력 배열에 쓴 뒤 소유권 전파/다음 이웃으로 진행한다. 벡터에 모아 반환 뒤 복사하면 두 번째 Factory 재진입에서 앞선 SID가 보이지 않는다. 기존 벡터/새 소비자 API는 같은 LinkImpl/ConnectImpl을 사용하고, 새 소비자는 원래 append 시점에 실제 커서 배열을 쓴다.

기존 세 판본의 Connect 미리보기696행은 새 API에 그대로 재생한다. 기존 fixture를 재생성하지 않았다. 별도 다중 연결 검사는 각 Factory 호출에서 이미 기록된 앞선 SID 개수와 원문의 최종 순서를 확인한다.

## 독립 원문 검증

`tools/decomp_cursor_previews.py`는 실제445b30/FindFlags/CRT 절삭·floor·FPU helper를 허용된 명령 범위에서 실행한다.32개 사슬 입력은445440/445340/4458e0도 실제 실행한다. decoder/Factory/WORD/가상 Owner/HP/연결 객체와 사슬의 설명/공간/GUI/투영/dirty 효과는 명시 경계다. 실제 Factory/연결 객체 내부 전체를 이번522행의 증거로 합산하지 않는다.

**522고유 입력/두 정밀도1,076정상 반환/사슬32·차이0/명령 및 명시 경계 진입 관찰870,296회**다. 두 x87 제어값은027f/037f다. 생성/재사용·빈 Canon·품질/flags400·HP 유무·signed 프레임 코드·최대6연결·음수/절삭 경계/화면WORD 감김·현재 전역 변경·연속 호출을 포함한다. 비유한 입력은 다리 snap/공간 조회를 하지 않는 경로에서만 관찰한다.

목록52DWORD·접근한 모든 raw 슬롯50바이트·모든 외부 효과를 비교한다. 두 호출 입력은 첫 호출 결과를 그대로 두 번째 호출에 사용한다. 관찰 초기의 Query 재사용 전환 입력은 유효한 후속 SID를 공급하지 않아 native vtable0에서 차단되었다. 해당 외부 입력을 완전한 재사용 목록으로 교정한 최종 원문만 사용하며, 이 실패를 제품 오류나 정상 반환 수로 합산하지 않는다.

로그 접두사는 `extracted/goal-cursor-previews-20261017-`다. `native3.log`는 최종 관찰이며 새 보고서는 `cpppj/cursor-previews.json`, 새 fixture는 `cpppj/tests/fixtures/cursor-previews-x86.tsv`다. 최초 직렬 빌드는 공통 헤더의 넓은 영향으로 오래 걸려, 소유한 CMake PID/명령/부모를 검증해 종료했다. 세션95886 종료1과 자식 종료를 확인한 뒤 검사 도우미 이름 Read를Get으로 교정했다. 이 첫 빌드는 완료/컴파일 실패 증거로 사용하지 않는다. 설치된 VS18의 Microsoft.Cpp.Common.props에서 UseMultiToolTask 기본false를 확인했고, 후속 빌드2는 UseMultiToolTask=true/EnforceProcessCountAcrossBuilds=true/CL_MPCount=3으로 실행한다. 실제 cl3개 동시 동작을 확인했다. 최종 원문3은522행의 바이트가 원문2와 동일하다. 그 뒤 실행 계산을 바꾸지 않는 한국어 상수 주석 한 줄만 보완하고 보고서에 이전/현재 도구 SHA를 기록했다. 최종 Release 빌드2는 **종료0·경고0·오류0**이다. 새 집중2개가522행의 전체 상태/접근 슬롯/효과와 일치했고, 기존 세 판본 미리보기696행 및 즉시 SID 가시성/실제 중립 섬 소유권 전파를 포함한 다리 집중7개도 통과했다. 전체 CTest 내부 **1,477개 모두 통과·실패0·235.50초·종료0**이다. 영구 내부 출력은 `extracted/goal-cursor-previews-20261017-ctest-output.log`다. 기본 시스템 커서는 통과했지만 앞선 간헐 실패 원인은 미확정이다.

## 남은 실제 플레이 작업

다음 연결인00446b50은 실제 명령446b55/446b7a에서 **int 화면 x/y**를 읽는다. C 출력의 float 매개변수 캐스트를 옮기지 않는다.0055a2d8이 켜졌을 때0041d770은 실제 `_ftol` signed64 결과의 **하위signed DWORD를 다시 float로 변환**한다. floor/반올림이 아니다.004971e0은 int 감김 뒤 x에16/y에11을 더하고 원본 float 역배율00503b54/00504194와 현재 카메라0059a900/904를 x87 정밀도로 합한다.스냅0055a2d8의 PE 초기값은0이며, Cancel의 teleporter 진단 전역은005e4794로 서로 다르다. 이번 단위에서는 이 입력 갱신 함수를 구현하거나 활성화하지 않았다.

실제 Client Session/현재 전역/메뉴 명령/Tell/GumpForm, 원본 커서 위치 갱신/표시와 내려놓기 위치426a70, 일반 건설 확정/배치와 생산·다리·경제·AI 전략이 남아 있다. Construct는 아직 활성화하지 않았다. GUI 미션 시작/단계·승패/저장/다음 미션·사용자 자연 완주와 전체 원본 동일성도 미확정이다.522행 또는400프레임을 완주 증거로 사용하지 않는다. 전체화면 DirectDraw 오류 수정은 이번 단계에서 제외한다.

최종 스테이지 감사에서 수정한 기존 BridgeConnectTests.cpp의 작업본CRLF/스테이지LF 차이를 확인했다. 줄바꿈만 정규화한 결과가 기존 스테이지와 바이트 동일함을 확인했고, LF 속성 및 보고서의 이전/현재 SHA 이력을 추가했다. 검사 코드의 실행 의미와 모든 원문 fixture는 유지했다.
