# 마나 볼트 주기 동작과 실제 Session — 단위129

현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 같다. AGENTS와 양 LEFT를 확인하고 깨끗한8bf7620에서 시작했다. 기존 정밀 소스를 읽었으며 새 내보내기·원본/클론 게임·GUI 실행은 없다. DirectDraw 전체화면 수정은 제외다. 로그 접두사는 `extracted/goal-mana-regular-20261017-`다.

## 원본 몸체와 연결

`RawManaBoltRegular`는 10.78의 **45a8e0**이다. 현재 물리 프레임의 signed 글자로 TYPE 구간을 읽고, event0/count0에서 현재 위치의 `influenceIcon.wav`를 요청한 후 signed count 나머지로 Frame.Set(flags0)을 호출한다. WORD12를 애니메이션 번호로 사용하지 않는다. 다른 event도 프레임 진행 후0.05를 반환한다.

event1은 같은 `RawManaBoltArrival::Target`으로 현재 후보를 다시 찾는다. 없거나 abstract가 아니면 가상 삭제(flags0) 후-1이다. 있으면 **자기 TYPE의 frame0** SHP 폭/핫스폿을 읽고 현재 대상 좌표를497220으로 투영한다. X에서 폭+핫스폿+spacing16, Y에서 unsigned WORD12+1의 간격을 뺀 뒤 Unpop0→4971e0 역투영→Pop0→현재0.05 순서다. DWORD 정수 감김과 signed SHP short를 보존한다. 대상의 그림이나 현재 애니메이션 프레임의 그림 크기로 바꾸지 않는다.

카메라 원점 두 종류를 구분했다. 투영은 현재 정수 scroll pixel59a91c/920을 읽고, 역투영은 float59a900/904를 읽는다. 원본 **497490**은 정수를 먼저 float로 좁히지 않고 FILD32×단정도 역배율 뒤 float로 저장한다. 공통 `RawCursorPosition::CameraFromPixels`로 이 몸체를 추가했고 같은 Session의 `SetParticleView`에서 정수/float 원점을 함께 갱신한다. 기존 `SoundToScreen`과 `ScreenToWorld` 몸체를 재사용한다.

Session은 도착 모듈과 같은 Target/TYPE/Frame/World/시계/Host/Kernel에서105의 실제509810 가상 표만 분배한다. 별도 모형 위치나 성공 기본값을 추가하지 않았다. 다른 파생 Regular는 기존 분배를 따른다.

## 원본 관측

새 `decomp_mana_bolt_regular.py`/`mana-bolt-regular-x86.tsv`가 실제45a8e0·4ad710·TYPE·Finder·497490·497220·4971e0을 실행한다. Frame.Set·음향·가상 공간·가상 자기 삭제만 명시 외부 효과다. 반환 float10은 caller의 FSTP float32 경계로 저장하며 ret12/ESP/x87 제어·빈 스택도 확인한다.

**196개 고유 입력 / 두 정밀도392정상 반환 / 322,906명령 / 정밀도 차이0**이다. A/B 현재 구간, event0/1/2/ffffffff, count0/1/2/7/ffffffff/80000000, WORD0/1/2/8000/ffff, 대상 부재/preview/소유권, signed SHP 헤더, 네 카메라 입력을 교차했다. 카메라에는32비트 경계 근처도 포함한다. 최초 Native1의 음수 정수 인수 인코딩 오류는 DWORD 비트로 보존해 해결한 뒤 Native2를 근거로 사용했다.

디컴파일의 역투영 호출 `unaff_EBX` 표기는 그대로 C++ 인자로 옮기지 않았다. 실제4971e0 진입 스택을 관측해 출력 포인터·X·Y를 확인했고, 예를 들어 스택의275/221이 실제 역투영 입력이었다. 원본 좌표 계산 전체와 C++ 요청/최종 비트가 일치한다. 기록된 ABI 표는 새 보고서에 보존했다.

## 실제 Session 검증

실제thewarbegins 월드의 원본 Factory/Owner/Pop으로 레인 제네레이터76을 abstract 대상 상태로 배치하고105를 생성했다. 실제 Regular0에서 첫 소리/프레임 진행을 실행한 뒤 실제39 비행 경로로 이동·도착했다. 도착이 기존46/61을 제거하고 새 event1을 만든 뒤, 현재 카메라319/127에서 같은 Frame/World의 위치 조정을 실행했다. 대상의 abstract 상태를 해제하자 다음 주기가105와 그 Regular를 실제 삭제했다.

처음 검사의 Kernel 번호 공백 기대는 잘못됐다. 새 event1이 반납된 초기 타이머 번호를 즉시 재사용하므로, 원래 event0과61의 부재 및 새 event1의 존재를 확인하도록 수정했다. 구현을 바꿔 번호를 예약하지 않았다. 독립 대조에서 외부 효과로 둔 Frame/가상 공간/삭제가 이 검사에서는 실제 몸체이며 음향 장치만 명시 관찰 경계다. 일반 건설 완료나 사용자 자연 플레이 증거로 확대하지 않는다.

## 최종 검증과 남은 범위

새 집중2개와 기존 커서 위치 검사는 통과했다. 최종 **Release 빌드4 종료0·경고0·오류0**, 전체 **1,522개 모두 통과·실패0·CTest 종료0·241.12초(전체241.15초)**다. 현대 `--verify` **23개**와 현재제품9개/보호3개 SHA·UTF-8/LF를 감사했다. 기존15보고서의 현재 HEAD 핀38개만 갱신하고 과거128핀은 보존했다. CMake 기존 접두 바이트, 기존 fixture, AGENTS, DN, 보호 원본은 보존한다.

`MakeConstructionBuilder`의 실제 접근점41db70과 move/speed/현재 nVert·Time·Prepare view 연결, 실제 서버 건설 시작과 일반 Construction Process/Frame/Visual은 남았다. Client 전체 DWORD 메뉴→실제 ContentForm6 Take/위치/수집/그리기/내려놓기, 생산·경제·AI 전략, GUI 시작/진행/승패·저장·다음 미션·사용자 자연 완주와 전체 원본 동일성도 미완료다. CD 대응47cb90은 이번 단위에서 허용하지 않았다. 전체 목표는 active다.
