# Player 실제 첫5초/후속60초 Handler와 현재 Template State

현재/마지막 정밀 PC는 **HJOW-Athlon, 2026-10-14**다. 단위14 자료를 변경하지 않은 신규15 준비이며, 사용자중단 지시로 현재 Source/Native/CPP초안을 동결했다. 새공유빌드/등록/다음격자는 시작하지 않는다.

## 실제 원본 몸체

Player152의 VT+5c는 **PATCH00490810/CD00407ef0**다. 원본 VT `0050ee08/CD005002a8`에서 직접 읽었다. owner가 현재battleOwner와다르거나 count0이면 payload를 그대로 반환한다. 처음 event1/count0/payload5는 기존 `RunRegular`가 현재+5/count1로재예약한다.

local/count!=0/event1은 group29에 원문`SORE LOSER TEST: `를로그한다. **그뒤** current inBattleMode를다시읽어 Battle이면`STARTING.\n`→new40→실제Regular(event2,payload60.0f,currenttime,count0)→0반환이다. 비Battle이면`NOT STARTING YET.\n`→원래payload반환이다. new40 실패에도 event1은0으로끝난다.

event2/count!=0은 Battle이면 현재Template owner의 State getter→State 첫BYTE OR4→현재Template owner를 다시읽어 Notify를호출하고0을반환한다. 비Battle도0반환이다. 불명event는PATCH41d/CD3c9줄을보고한뒤payload를반환한다. 실제0상수는PE `00500fc4/CD005002a0=00000000`이다.

| helper | 패치10.78 | CD10.72 | 현재복원선택범위 |
| --- | --- | --- | --- |
| State getter | `004bd550` | `00427920` | 현재State lookup/pos0·기존nonempty/end>=28 데이터 |
| Notify thunk/body | `004bd540/004bd360` | `00427910/004275d0` | filename/isMine/+12·현재isMine재조회·inMission/manager/+16 |
| lookup/pos0 | `0044ce60/0044ceb0` | `0044c4a0/0044c4e0` | 단일현재State 이름검색/현재stream/pos0 |
| 최소길이확인 | `0044d290` | `0044c600` | 기존end>=28의자연반환 |
| CD즉시depth복구 | 없음 | `00427a47/00429f30` | 진입+1→즉시−1, 같은현재DWORD감김 |

실제미션부모는 Template+8=1을먼저쓴다. Notify는filename이`.`로시작하지않고+12=0인 isMine1에서자연반환하므로 파일저장/inMission진단을선택하지않는다. 실제보호WarBegins State는양archive동일28BYTE `7037e7000000000000005e000000000000028d003847000000008d00`이다. 이입력의첫BYTE70은 실제event2에서74가된다.

## 독립 관찰과 경계

새 고유입력은 **658=정상566/assert68/미복원경계24**다. 각판본329=정상283/assert34/거부12이고, 두x87 정상/진단 최상위반환1268은반복이다. body/현재owner/Frame/Regular 최초시각·count·payload/Form전체raw/Kernel PID/rotation·Template180/State9pos/end/전체28BYTE를보존했다.

실제Handler/State/Notify/lookup/최소길이helper와기존152Factory/Regular2ctor/Form/Kernel는원본명령을실행한다. ASCII`State`일치비교·로그·진단·new40예약은명시외부경계다. native예비격자/14Player1680·내부재사용/두정밀도반복을추가고유수에더하지않는다.

없는State 생성, end1/27의new60힙확장, 실제파일저장 `0044d8d0/CD0044cc40`은미복원진입에서중단한다. 이24행은 `status=boundary, returnbits=-`이며정상반환/assert와합치지않는다. 원본새State의미초기stack BYTE와파일저장/OS 효과를가짜0으로초기화하지않는다. 일반DataManager/파일초기화전체완료근거가아니다.

출처는 `decomp_player_regular_oracle.py`, `recovery-player-regular-evidence.json`, `player-regular-x86.tsv`, 분리한두spec/정밀export다. **`python -X utf8 tools/analyze_player_regular_native.py`가전체SHA/658입력/세분류/반환누락을읽기감사하며통과**했다. 원본producer에는`--verify`가없다. 기본producer는재관찰/fixture생성이므로감사목적에다시실행하지않는다.

## C++ 현재 준비와 인수인계

`RawPlayerRegular`는 실제 `RawPlayerWorld`와같은 `core.ownerMode`의battle·두owner를참조하고로그/진단/예약/State/Notify를필수hooks로받는다. `RawPlayerTemplateState`는현재Template180/State9/filename9/loadingDepth/inMission을장기소유자에서직접빌린다. existingState/view 선택은명시하며unsupported경계를성공으로채우지않는다.

`PlayerRegularTests.cpp`는양판본전수2+같은실제Core/Player152/Host의첫0→5→65초소비자1을준비했다. 소비자는17.25첫실행→22.25새event2/PID2 및같은Kernelpass의count0→82.25 State70→74/Notify/두Form종료를대조한다. **미등록/미컴파일/미실행**이다. 매크로는`NETSTORM_PLAYER_REGULAR_FIXTURE`다. actual정상미션172/Player173 전체설치·AI/Guide/승패·렌더/UI/음향/캠페인완주는미완료다.

다음재개에서는최신AGENTS/양LEFT와같은PC를확인하고단위15의이3검사/actualparent130의2검사를등록·빌드·전수검증한다. root가소유한 `RawCampaignWorldRuntime`의실제173초기입력·같은Pool/Frame/HP/Pop/Post/두난수/현재기록/예약을연결하고하위미복원효과를주소별로유지한다.
