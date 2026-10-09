# cpppj — NetStorm C++ 빌드

기존 게임(NetStorm: Islands at War)을 디컴파일한 소스를 토대로 C++ 소스를 다시 만드는 프로젝트다.
C# + MonoGame 빌드(`../dotnetpj/`)는 cpppj 완성 후 이를 분석하여 개발한다.

**1차 복원 기준(2026-10-07 사용자 결정): `originals/Netstorm.exe`의 10.78이다.** 추가 10.37·10.62 패치·10.82 본체와 DevLog는 비교 자료로 사용하며 최신판 규칙/기능을 섞지 않는다. [판본 비교·재현](../docs/exe/cpp-reference-versions.md), [정적 기록](recovery-reference-versions.json).

**방향(AGENTS.md):** 기존 게임을 디컴파일하여 C++ 코드로 **최대한 복원하는 것이 1차 목표**다. 이후 Windows 10/11에서 실행 가능한 수준으로 만들고, 요구사항 반영과 MCP 추가로 이어 간다. 기능 변경을 최소로 하고 원본과 같은 방식으로 만든다. `options.cfg`는 원본과 같은 시작·변경 저장 시점에 `<게임 폴더>/d/options.cfg`로 쓴다(Windows-1252·XOR). 그래서 창·화면·입력은 원본처럼 **Win32 API를 직접 부른다**(외부 라이브러리 없음). **Windows 전용**이다.

**현재 우선순위(2026-10-05 사용자 요청): 기존 게임이 온전히 동작하고 실제 게임 플레이가 가능하도록 복원한다.** 먼저 메인 메뉴→캠페인 선택→브리핑→선택·이동·건설·경제·전투→승패·결과·재시작을 완성하고, 나머지 원본 기능·캠페인까지 복원한다. 화면비 확장·한국어·Linux·60/120프레임·MCP·추가 기능은 후순위다. 작업 순서와 완료 기준은 [실제 플레이 복원 계획](../docs/cpp-playable-plan.md)을 따른다.

**원본 메뉴→캠페인→브리핑→실제 지형/객체·선택·사제 이동→메뉴 복귀를 연결했다.** 1-1과 TEST01의 시작 SP/동맹·지형·점유를 적용하고, 몸통 좌클릭 선택→땅 좌클릭 이동, 우클릭 메뉴, 화면 이동·일시정지·재진입을 검사했다. **건설·채집·전투·AI·승패·사운드 출력·DirectDraw 전체화면은 후속이며 미션 완주는 아직 불가능하다.** [월드/조작의 범위와 제한](../docs/exe/cpp-world-reconstruction.md), [메뉴 연결](../docs/exe/cpp-menu-reconstruction.md).

[복원 근거·함수 대응·검증 범위](../docs/exe/cpp-reconstruction.md), [검토 목록](recovery-manifest.json), [기계어 검증 기록](recovery-evidence.json).

2026-10-09 최신(`HJOW-Athlon`): **[일반 배치 후보 권한·방향 소유 관계](../docs/exe/cpp-canon-permission-reconstruction.md)**를 복원했다. `RawCanonPlacementPermission`이 그래프 준비/요청 소유자 조기 반환·현재 raw 소유자/편집기/방향 동맹·중립/다른 소유자 허용을 검사하고 Player 조회의 DWORD 반환을 보존한다. signed BYTE 소유자 어댑터를 실제 3×3 decoder/미리보기/finder/지형에 연결했다. 새 독립 x86 **16,890개**, 누적 **328,123개**, Release 경고/오류 **0**, CTest 내부 **401개·실패 0**(101.73초), 감사 **58종 모두 통과**. **Player 작업장·그래프 조회 몸체는 필수 경계이며 전체 MayPlace는 미완료**다. 다음은 이 Player 조회와 실제 주변 권한 finder·최종 표면 소유 관계/특수 지역/거부 조건, 이어 사제 Pop·보호막/회복 예약·Carrier 검사와 raw GUI·건설·경제·전투·승패다. 최신 인계는 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[일반 타입 지형·지역 누적과 권한 경계](../docs/exe/cpp-canon-terrain-reconstruction.md)**를 복원했다. `RawCanonPlacementTerrain`이 그룹/genus/bridge 초기 권한·후보 다리/섬·실제 지역 getter/배열·모양 종료·최종 관계 진입 전 지면 누적을 처리한다. 실제 3×3 받침/미리보기/finder/지역 지도를 두 판본에서 연결했다. 새 독립 x86 **11,220개**, 누적 **311,233개**, Release 경고/오류 **0**, CTest 내부 **395개·실패 0**(106.75초)이며 감사 **57종 모두 통과**했다. 후보 권한·주변 관계·최종 표면 소유 관계는 필수 외부 경계이며 **전체 MayPlace 완료는 아니다**. 다음은 이 권한/관계 몸체·특수 지역·최종 거부 조건, 이어 사제 Pop·보호막/회복 예약·Carrier 검사와 raw GUI·건설·경제·전투·승패다. 최신 감사/인계는 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[일반 타입 배치의 후보 충돌/실제 패턴 연결](../docs/exe/cpp-canon-collision-reconstruction.md)**을 완료했다. `RawCanonPlacementCollision`이 사제 genus 제한 없이 무시·발자국 표시·extra/mode 거부와 실제 finder를 사용하고 기존 사제 후보도 같은 본문에 위임한다. 실제 3×3 받침/미리보기/모양/finder·두 번째 모양 거부·현재 전역/저장된 next를 두 판본에서 검사했다. 새 독립 x86 **22,680개**, 누적 **300,013개**다. 원본 후보 중간 구간 관찰과 C++ 패턴 합성을 구별한다. Release 경고/오류 **0**, CTest 내부 **389개·실패 0**(103.33초), 감사 **56종 모두 통과**. **다음은 일반 후보 지형·지역 효과/모양 종료·최종 관계 판정과 사제 Pop·보호막/회복 예약·Carrier 검사**이며 raw GUI·건설·경제·전투·승패는 남았다. 최신 감사/인계는 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[일반 타입·패턴의 로컬 배치 미리보기](../docs/exe/cpp-canon-preview-reconstruction.md)**를 완료했다. `RawCanonPlacementPreview`가 타입/별도 argument·로컬 배열·고정 표면/현재 genus·방향 소유 관계를 공유하고 기존 사제 경로도 위임한다. 실제 decoder 범위 연결·3×3 받침/비로컬 배열 보존을 두 판본에서 검사했다. 새 독립 x86 **4,860개**, 누적 **277,333개**다. 모양/decoder 진입과 후반 충돌 구간 대체를 구별한다. Release 경고/오류 **0**, CTest 내부 **382개·실패 0**(104.87초), 감사 **55종 모두 통과**. **다음은 일반 충돌 정책·지형/지역/관계와 사제 Pop·보호막/회복 예약·Carrier 검사**이며 raw GUI·건설·경제·전투·승패는 남았다. 최신 감사/인계는 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[일반 타입·패턴 배치 접두와 실제 픽셀 getter 연결](../docs/exe/cpp-canon-placement-reconstruction.md)**을 완료했다. `RawCanonPlacement`가 타입 번호/별도 argument·진입 초기화·즉시 허용·캡처한 부호 BYTE 소유자·판본별 여백/지도 경계를 공유하고 기존 사제 접두도 위임한다. 새 독립 x86 **3,360개**, 누적 **272,473개**다. 실제 10.78 **116개 자산·512개 조합·4,096개 관찰**, CD **101개 자산·497개 조합·3,976개 관찰**이 원본 명령과 일치한다. getter와 접두는 별도 원본 실행이고 모양 진입/후반 정책은 명시 대체다. Release 경고/오류 **0**, CTest 내부 **377개·실패 0**(108.26초), 감사 **54종 모두 통과**. **다음은 일반 타입 미리보기/충돌 정책·지형/관계와 사제 Pop·보호막/회복 예약·Carrier 검사**이며 비표면 raw GUI·건설·경제·전투·승패는 남았다. 최신 감사/인계는 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[일반 자산/패턴 배치의 모양 순회·finder 사각형](../docs/exe/cpp-canon-geometry-reconstruction.md)**을 복원했다. `RawCanonPlacementGeometry`가 타입 번호/패턴 번호를 구별하고 모든 유효 칸의 현재 발자국·원래/절삭 좌표와 정수 탐색 범위를 공급한다. 사제 geometry도 기존 종류 계약을 유지한 채 같은 순회에 연결했다. 후보/지형 거부 뒤의 후처리 생략과 0 발자국 일반 자산 `dude`도 보존한다. 새 독립 x86 **5,967개**, 누적 **269,113개**다. 실제 10.78 **116개 자산·512개 조합·2,132개 칸**, CD **101개 자산·497개 조합·2,117개 칸**이 원본 계산과 일치한다. Release 경고/오류 **0**, CTest 내부 **372개·실패 0**(114.35초), 감사 **53종 모두 통과**. **전체 MayPlace/일반 타입의 배치 접두·미리보기/충돌 정책·지형/관계 판정과 일반 사제 Pop·보호막/회복 예약·Carrier 검사는 후속**이며 비표면 raw GUI·건설·경제·전투·승패로 이어 간다. 최신 감사/인계는 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[일반 자산과 특수 패턴의 픽셀 범위](../docs/exe/cpp-canon-shape-reconstruction.md)**를 완료했다. `RawCanonPixelShape`가 일반 타입과 영역·다리·섬·받침·공유 전투 패턴의 모든 유효 칸/SHP 범위를 합치며, 사제 배치는 기존 종류 계약을 유지한 채 공통 계산기로 위임한다. 판본별 float32 저장·동률·부호 있는 0·기준점 DWORD 차·빈 범위를 보존한다. 새 독립 x86 **5,950개**, 누적 **263,146개**다. 실제 10.78 자산 **116개·512개 조합**, CD **101개·497개 조합**이 원본 전체 getter와 일치한다. Release 경고/오류 **0**, CTest 내부 **366개·실패 0**(101.77초), 감사 **52종 모두 통과**. **다음은 일반 타입의 전체 MayPlace/지형·관계 연결과 일반 사제 Pop·보호막/회복 예약·Carrier 상태 검사**, 이어 비표면 raw GUI·건설·경제·전투·승패다. 최신 인계는 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[CanonDecoder 타입별 패턴 선택/전체 순회](../docs/exe/cpp-canon-type-reconstruction.md)**를 복원했다. 기존 영역/다리와 누락 섬 2개·받침 1개·공유 전투 1개를 현재 타입 전역 순서로 선택하며, 패턴 인자/비패턴 명시 프레임·CD 홀수 방향 진단·별칭 우선순위를 보존한다. 새 독립 x86 **3,927개**, 누적 **257,196개**다. 두 판본 실제 자산 각각 **408개 조합·2,028개 유효 칸**의 전체 순회/범위/끝 상태가 원본과 일치한다. 기존 사제 배치도 같은 decoder를 사용하며 사제/비패턴 종류 계약을 유지한다. **다음은 패턴의 일반 자산 픽셀/지형·관계 연결과 일반 사제 Pop·보호막/회복 예약·Carrier 상태 검사**, 이어 비표면 raw GUI·건설·경제·전투·승패다. 최신 검증은 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[타입 기준점과 실제 사제 배치 자산 연결](../docs/exe/cpp-priest-assets-reconstruction.md)**을 완료했다. `hotFootRatioX/Y`의 float32→고정 16/11→x87 곱셈→0쪽 정수 절삭과 초기값/순서/중복을 복원했다. `PriestPlacementAssets`가 실제 프레임 코드/defaultFrame·타입 기준점·SHP 물리 헤더·패턴 전역 번호를 함께 공급하며 표면 월드도 같은 자료를 사용한다. 새 독립 x86 **1,404개**, 누적 **253,269개**다. 실제 자산 CLI/두 판본 대조 및 최종 검증은 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다. 전체 MayPlace/일반 사제 Pop/GUI 플레이는 후속이다. **다음은 특수 패턴/타입 조합·일반 사제 Pop/보호막·회복 예약/Carrier 상태 검사**, 이어 비표면 raw 월드/GUI·건설·경제·전투·승패다.

2026-10-09 앞 단계(`HJOW-Athlon`): **[일반 사제 배치의 다리·섬·지역 효과](../docs/exe/cpp-priest-terrain-reconstruction.md)**를 복원했다. 비패턴 decoder/SHP/미리보기/finder/후보를 통과한 뒤 실제 지역 번호를 쓰고 모양 종료와 최종 사제 우회를 계산하며, 기존 나선 생성/실제 생성자 통합에도 연결했다. 새 독립 x86 **6,720개**, 누적 **251,865개**다. 전체 MayPlace를 기계어 실행한 근거는 아니며 중간 후보/모양 종료/최종 구간과 실제 지역 getter의 대조다. **다음은 타입 기준점/프레임 메타 자산 연결·특수 패턴/타입 조합·일반 사제 Pop/보호막·회복 예약/Carrier 검사**, 이어 비표면 raw 월드/GUI·건설/경제/전투/승패다. 최신 검증은 [LEFT_JOBS.md](../LEFT_JOBS.md)를 따른다.

2026-10-08 이전 단계(`HJOW-Athlon`): **[Damageable 삭제 준비의 효과·소리 접두와 실제 종속 순회](../docs/exe/cpp-damageable-predestroy-reconstruction.md)**를 복원했다. ordinary 진입→붕괴/폭발 효과·현재 좌표의 소리→좌표+0.9999/x87 절삭의 spot→contained 사제마다 해방 소리→현재 권한/flags의 해방 요청→같은 flags의 공통 pre 순서다. 공용 contained 커서는 다음 SID를 먼저 저장하며 현재 타입/kind 전역을 다시 읽는다. 실제 사제 보호막/Carrier·회복 form/Kernel·공통 장부/깊이·SID 반납 합성에도 접두를 연결했다. 새 독립 x86 **4,896개**, 누적 **207,577개**, Release 경고/오류 0·CTest 내부 **311개·실패 0**(91.03초), 감사 **41종 모두 통과**. **파편/소리 출력·종속 해방의 생성/배치·Carrier 전역 후처리의 내부는 외부 경계**다. 기계어 대조도 해방 중간 코드 구간을 명시 대체하며 바깥 조건/좌표/순회/반환만 실제 실행한다. 다음은 해방 좌표/타입 조건·생성/배치 요청·보호막 생성/해제·사제 낙하·전체 공간 수명이다. 일반 사제 Pop/비표면 raw GUI·실제 플레이는 후속이다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`HJOW-Athlon`): **[Carrier 삭제 준비와 실제 contained 조회](../docs/exe/cpp-carrier-predestroy-reconstruction.md)**를 복원했다. Damageable 호출 뒤 현재 권한→타입 DWORD/두 WORD 조건의 종속 조회 `(1,0,0)`→0이 아닌 SID일 때 인자 없는 전역 후처리 순서를 보존한다. 직접 조회는 대체 없이 실행하고 Carrier에서는 Damageable/전역 후처리만 외부 경계다. 사제 보호막 조회·회복 form/Kernel·공통 장부/깊이·SID 정리의 기존 두 판본 server/client 합성 검사에도 실제 Carrier를 연결했다. 새 독립 x86 **2,880개**, 누적 **202,681개**, Release 경고/오류 0·CTest 내부 **307개·실패 0**(101.70초), 감사 **40종 모두 통과**. **다음은 Damageable pre/post의 파편·소리·공간 효과와 전역 후처리·보호막 생성/해제·사제 낙하·전체 postPop/공간 효과**다. 일반 사제 Pop/비표면 raw GUI·실제 플레이는 후속이다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`HJOW-Athlon`): **[사제 보호막 실제 조회와 삭제 준비 연결](../docs/exe/cpp-priest-forcefield-reconstruction.md)**을 완료했다. 원본 좌표 절삭→flags 0의 일반 finder→DWORD 타입/owner BYTE의 첫 일치 SID를 복원했다. 조회에는 외부 대체가 없으며 preDestroy는 보호막 가상 삭제·Carrier 몸체만 경계로 남긴다. 두 판본 server/client 삭제 연결 검사도 수동 SID 반환에서 실제 조회로 바꿔 회복 form/Kernel·공통 장부·SID 정리를 검사했다(공간 등록은 합성 void 입력). 새 독립 x86 **8,928개**, 누적 **199,801개**, Release 경고/오류 0·CTest 내부 **303개·실패 0**(93.06초), 감사 **39종 모두 통과**. **다음은 Carrier/Damageable 삭제 효과·보호막 생성/해제·사제 낙하·전체 postPop/공간 효과**이며 일반 Pop/비표면 raw GUI·보행 진행/종료·실제 플레이는 후속이다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`HJOW-Athlon`): **[사제 삭제 준비와 회복 form 정리](../docs/exe/cpp-priest-destroy-reconstruction.md)**를 복원했다. 사제 목록의 모든 중복 제거→보호막 조회/삭제 요청(flags 0)→Carrier 호출 순서를 유지하며 추상/매장·dead/void·비활성 꼬리를 대조했다. 보호막 공간 조회/가상 삭제의 파생 몸체와 Carrier/Damageable 효과는 외부 훅이다. 두 판본 server/client의 실제 사제 생성자·회복 prefix→ProcessForm/Kernel→삭제·장부·SID 반납을 void 자산에서 합성했다. 새 독립 x86 **6,552개**, 누적 **190,873개**, Release 경고/오류 0·CTest 내부 **299개·실패 0**(100.12초), 감사 **38종 모두 통과**. **다음은 보호막 실제 조회·Carrier/Damageable 삭제 효과·사제 보호막/낙하·전체 postPop/공간 효과**이며 일반 Pop/비표면 raw GUI·보행 진행/종료·실제 플레이는 후속이다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`HJOW-Athlon`): **[사제 회복 이벤트 0x25a와 Kernel 연결](../docs/exe/cpp-priest-regen-reconstruction.md)**을 완료했다. 실제 HP getter/setter·최대 HP/6·중립 조건·패치 난수/추적 필드 쓰기를 복원했다. 공간/표시·damageable·중립 동작·패치 검증/측정 하위 효과는 외부 훅이다. 새 독립 x86 **4,539개**는 세 PE의 회복 분기 전체 정상 반환/스택과 C++ 반환/사건/추적·난수/raw 슬롯을 대조한다. 두 판본 server/client 생성자→회복 예약→Kernel→HP 80→113→146→179→200·재예약/정리를 검사했다. Release 경고/오류 0·CTest 내부 **294개·실패 0**(90.47초), 감사 **37종 모두 통과**, 누적 독립 x86 **184,321개**다. **다음은 사제 preDestroy·보호막/낙하·전체 postPop/공간 효과**이며 일반 Pop/비표면 raw GUI·보행 진행/종료·실제 플레이 복원은 후속이다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`HJOW-Athlon`): **[사제 HP/지면 상태 조회와 HP setter](../docs/exe/cpp-priest-state-reconstruction.md)**를 복원했다. 패치 signed DWORD/CD signed WORD·절반 HP 경계·권한/소유자 허용·Unpop→좌표 보정→Repop 순서를 보존한다. 지면 조회의 **0.9999f**와 HP 전환의 **0.99999f**를 구별한다. 새 독립 x86 **20,256개**는 조회 대체 없이, setter 공간 효과 두 개만 기록 대체하여 전체 몸체 정상 반환과 C++ 반환/사건/raw 슬롯을 대조했다. 최종 Release 경고/오류 0·CTest 내부 **289개·실패 0**(92.95초), 감사 **36종 모두 통과**, 누적 독립 x86 **179,782개**다. **다음은 실제 회복 이벤트 0x25a·사제 공간 효과/보호막/낙하·전체 postPop/preDestroy**이며 비표면 raw GUI·보행 진행/종료와 실제 게임 플레이 복원은 남았다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[Carrier/Damageable→공통 postPop 직접 호출](../docs/exe/cpp-carrier-postpop-reconstruction.md)**을 복원했다. 비최초·비권한 가상 +0xcc→로딩 중 지면 소유자 갱신 조건→같은 flags의 공통 장부 순서를 보존한다. 사제 생성자·소유자·회복 prefix/ProcessForm·Kernel과 실제 공통 비용/통계/깊이를 두 판본 server/client에서 합성했다. 새 독립 x86 **7,344개**는 세 PE의 carrier/damageable 전체 몸체 정상 반환/스택을 확인하며 **+0xcc·지면 갱신·공통 후처리 세 호출만 대체**한다. C++ 사건/좌표 비트/외부 효과 뒤 슬롯 전체가 일치한다. 일반 Pop/Activate의 사제 거부와 공통 raw/Graph/AI 보호를 유지했다. Release 경고/오류 0·CTest 내부 **284개·실패 0**(58.19초), 관련 감사 **6종 통과**, 누적 독립 x86 **159,526개**다. **다음은 사제 HP/지면 상태 조회·실제 회복 이벤트·보호막/낙하·preDestroy**이며 가상 +0xcc와 지면 공간 순회·보행 진행/종료·비표면 raw GUI는 남았다. 사용자 지시의 30분 내 범위로 진행했고 장시간 검사는 인계한다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[사제 postPop의 목록·회복 예약 prefix](../docs/exe/cpp-priest-postpop-reconstruction.md)**를 복원했다. carrier 호출 전에 중복 없는 사제 목록 등록→기존 회복 Regular(0x25a) 검색→확보 성공 때 최대 HP로 payload 계산/예약→원래·현재 소유자가 같은 1~8의 상태 칸 초기화를 실행한다. 기존 최대 HP 조회와 실제 ProcessHost/ProcessForm·Kernel을 연결했다. 세 PE/두 x87 정밀도의 새 **7,176개** 관찰과 C++ 목록·소유자 칸·사건·payload 비트·raw 슬롯 전체가 일치한다. 독립 원본 실행은 **carrier 진입에서 중단**, Regular 검색/할당/생성만 대체한다. Release 경고/오류 0·CTest 내부 **278개·실패 0**(57.75초), 관련 감사 **5종 통과**, 누적 독립 x86 **152,182개**다. **다음은 carrier/damageable postPop·사제 회복 이벤트 처리·낙하/경로·preDestroy**이며 보행 진행/종료·비표면 raw 월드는 남았다. 전체 사제 postPop·실제 HP 회복·GUI raw 연결은 아직 미완성이다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[사제 소유자 재정의](../docs/exe/cpp-priest-owner-reconstruction.md)**를 복원했다. 현재 소유자 BYTE와 원래 소유자 WORD의 low 7비트/로컬 표식을 구별하고 로딩·전투 모드의 요청 처리/알림 순서를 보존한다. 세 PE의 새 제한 x86 **7,416개**는 대체 함수 없이 사제/공통 지정/알림 전체를 실행했으며 C++ 사건/슬롯 전체와 일치한다. 실제 두 판본 사제 생성자·공통 소유자와 순차 모드 전환, 기존 섬/종유석 분배기 합성을 콘솔에서 검사했다. Release 경고/오류 0·CTest 내부 **272개·실패 0**(62.21초), 관련 감사 **4종 통과**, 누적 독립 x86 **145,006개**다. **다음은 사제 postPop/preDestroy·회복 Regular·낙하/경로 처리**이며 보행 진행/종료·비표면 raw 월드는 남았다. 이번 작업은 GUI 연결이나 포획 플레이 완성이 아니다. 원본 게임/복사본·클론 창 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[Squid 방향 조회·PathProcess 도착 프레임 접두](../docs/exe/cpp-path-animation-reconstruction.md)**를 복원했다. 원본의 signed side와 방향 계산, **Unpop(0) → 방향별 첫 물리 프레임 +1 쓰기 → Repop(0x40)**를 보존한다. 세 PE의 새 제한 x86 **6,048개**와 C++ 사건/슬롯 전체가 일치하며 누적 **137,590개**다. 실제 두 판본 raw Unpop/Pop·해시 재등록을 콘솔에서 검사하고, GUI 보행 종료 프레임 계산에 연결했다. Release 경고/오류 0·CTest 내부 **266개·실패 0**(60.46초), 관련 감사 **7종**, world/surface 창 회귀가 통과했다. **건물/walker는 기존 어댑터이며 보행 중 12Hz 임시 타이머·경로 종료는 미복원**이다. 다음은 사제 전용 소유자/postPop·PathProcess 프레임 진행·비표면 raw 전환, 이후 낙하·파편/소리·배치·건설·경제·전투·승패다. 원본 게임/복사본 실행·보호 파일 변경·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[raw 표면 월드·GUI·게임 시각 연결](../docs/exe/cpp-raw-world-integration.md)**을 완료했다. 본섬 지면·noIsland·다리·받침·종유석·연결 조각은 실제 raw SID/Graph 상태에서 표시하고, 미션 내부 Kernel에 클라이언트의 고정 게임 시각/프레임을 공급한다. 건물·walker는 기존 어댑터이며 전역 Kernel 전체 복원은 아니다. Release 경고/오류 0·CTest 내부 **261개·실패 0**, 관련 감사 **15종 통과**. 기존 창 회귀 4종과 새 raw 표면 창 검사도 통과했다. 정지 중 섬 삭제 예약 유지→재개 뒤 끝 칸 교체/연결 정리→실제 픽셀 변화→미션 재진입 초기화를 확인했다. 실제 10.78/CD **5개 미션 raw 로드**를 확인했고 새 독립 x86 입력 없이 누적 **131,542개 유지**다. **다음은 비표면 객체의 raw 전환·애니메이션 프로세스·walker 낙하·파편/소리**이며 배치·건설·경제·전투·승패는 남았다. 클론 창만 실행했고 원본 파일·AGENTS.md·dotnetpj를 변경하거나 커밋/푸시하지 않았다.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[Graph 활성 섬 생성·다리 가상 삭제·끝 칸 재등록](../docs/exe/cpp-surface-graph-integration.md)**을 완료했다. `RawSquidDestroyDispatch`로 원본 가상 +0x10의 hard/debugKeep 삭제 거부를 연결하고, Graph 활성 상태에서 받침/종유석 생성→지연 낙하→끝 칸 생성/삭제/Pop→연결/Regular 정리를 실제 SquidFrame/Display·공통 장부·공간과 함께 검사했다. Release 경고/오류 0·CTest 내부 **257개·실패 0**(62.12초), 관련 감사 **10종 모두 통과**. 실제 10.78/CD 타입·SHP를 사용하는 **두 판본 × 네 흐름 = 8개 모두 통과**다. 새 독립 x86 입력은 없고 누적 **131,542개 유지**. **다음은 raw GameWorld/GUI·Kernel 프레임/게임 시각 연결**이다. 실제 애니메이션·낙하/파편/소리·건설/경제/전투/승패는 남았다. 원본 게임/복사본·클론 창 실행·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[SquidFrame 표시 갱신·실제 SHP 크기 공급](../docs/exe/cpp-frame-display-integration.md)**을 `MakeSquidFrameHooks`로 실제 SquidDisplay/Unpop/Pop·Renderer에 연결했다. Release 경고/오류 0·CTest 내부 **251개·실패 0**(57.52초), 관련 감사 **4종 모두 통과**다. 실제 10.78/CD 자산의 물리 프레임 **6,950개** 메타데이터와 범위 안 크기 공급 **6,946개**를 확인했고 두 판본 모두 부분 Draw/Present가 통과했다. 새 독립 x86 입력은 없고 누적 **131,542개 유지**다. **다음은 Graph 활성 상태의 끝 칸 생성/삭제/Pop·받침 생성 통합**이다. GUI raw GameWorld·Kernel↔게임 루프·애니메이션 프로세스·실제 낙하/파편/소리·건설/경제/전투/승패는 남았다. 원본 게임/복사본·클론 창 실행·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[섬 삭제→지연 낙하→끝 칸 변환→연결 객체 정리](../docs/exe/cpp-surface-lifecycle-integration.md)**를 `RawSurfaceLifecycle`과 실제 raw 모듈로 연결했다. 10.78/CD에서 정상 교체·고립 삭제·hard 유지·`0x1000` 보존·표면 삭제의 walker 분배를 확인했다. Release 경고/오류 0·CTest 내부 **246개·실패 0**(60.27초), 관련 근거 감사 **7종 모두 통과**다. 새 기계어 관찰은 없고 누적 제한 x86 **131,542개**는 그대로다. **다음은 SquidFrame→SquidDisplay 표시 갱신과 실제 SHP 크기 공급이다.** Graph 활성 통합·GUI raw GameWorld·Kernel↔게임 루프·낙하/파편/소리·건설/경제/전투/승패는 남았다. 이번 단계 원본 게임/복사본·클론 창 실행·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[noIsland 최초 등록·3×3 받침 소유자 전파](../docs/exe/cpp-islandpostpop-reconstruction.md)**를 복원해 `SquidPostPop`에 연결했다. F 칸에서 받침·종유석을 만들고 H 칸에서 9칸/받침/종유석의 소유자를 맞추는 실제 raw 모듈 통합이 두 판본에서 통과한다. 새 제한 x86 **1,656개**·Release 경고/오류 0·CTest 내부 **240개·실패 0**(100.04초), 누적 **131,542개**·감사 **31개 모두 통과**다. `regiongraph`는 재생성 후 기존 1,536개 fixture가 바이트 단위로 같고 감사도 통과한다. 클론 창 회귀 4종도 통과했다. **GUI raw GameWorld·Graph 활성 통합·Kernel↔게임 루프·낙하/파편/소리·건설/경제/전투/승패는 남았다.** 원본 게임/복사본 실행 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): 앞 단계에서 콜백 경계로 남았던 **[다리/섬 연결·연결 객체 생성·소유자 전파](../docs/exe/cpp-bridgeconnect-reconstruction.md)**(`004213b0`·`004210f0`·`00421240`)와 섬 받침·종유석의 소유자 지정/postPop 재정의, **[Squid 프레임 지정](../docs/exe/cpp-setframe-reconstruction.md)**(`004acee0`), **[섬 받침 preDestroy·noIsland postDestroy](../docs/exe/cpp-islandlifecycle-reconstruction.md)**(`00442240`·`00442640`)를 복원했다. 다리가 섬 표면에 닿으면 연결 조각이 생기고 주인 없는 섬이 다리 주인의 것이 되는 과정을 실제 생성/소유자 지정/Pop/프레임 지정 모듈로 이은 콘솔 통합 검사가 두 판본에서 통과한다. 새 제한 x86 **18,132개**(10,800 + 5,772 + 1,560)·Release 경고/오류 0·CTest 내부 **234개·실패 0**, 누적 **129,886개**·감사 30개 중 29개 통과(`regiongraph`는 [재현 준비만 끝남](../docs/exe/ghidra-exports.md)). 앞 단계 이웃 필터의 spot 칸 보정(+0.9999)을 원본 관찰로 확인해 정정했다. **이 PC에서는 창 검사가 허용돼 클론 창 회귀 4종(window·renderer·menu·world)을 실행했고 모두 통과했다.** 원본 게임/복사본 실행 없음. **noIsland postPop(`004423b0`)·GUI raw GameWorld·Kernel↔게임 루프·Graph 활성 통합·낙하/파편/소리·건설/경제/전투/승패는 남았다. 변이 확인 일부와 `regiongraph` 재생성은 다음 작업으로 넘겼다([LEFT_JOBS.md](../LEFT_JOBS.md)).**

2026-10-08 후속: [flag 0 첫 연결 이웃·다리 postPop 접두·끝 칸 raw 통합](../docs/exe/cpp-neighbor-reconstruction.md)을 완료했다. 새 제한 x86 **1,968개**(이웃/이벤트 1,728 + 접두 240)·Release 경고/오류 0·CTest 내부 **218개·실패 0**(CTest 전체 86.80초), 누적 **111,754개**·감사 **27개 모두 통과**다. 실제 생성/소유자/삭제/Pop과 실행 중 Regular 정리를 콘솔에서 연결했다. **다리 연결 객체/소유자 전파 함수는 콜백 경계이며 GUI raw GameWorld·Kernel↔게임 루프·Graph 활성 통합·실제 낙하/파편/소리·건설/경제/전투/승패는 남았다.** 게임/복사본·클론 창 실행 없음.

[2026-10-08 소유자 지정(vtable +0x74)·다리 이벤트 처리기 분배](../docs/exe/cpp-owner-reconstruction.md), [독립 x86 3,000개](recovery-owner-evidence.json) — 객체의 소유자 바이트를 바꾸고 요새/전투 모드에서 vortex·factory 타입이면 이전 소유자의 작업장 목록에서 빼고 새 소유자의 목록에 넣는 base 가상 함수를 복원했다(`SquidOwner`, 대체 함수 없이 원본 명령과 대조). 다리 이벤트 처리기는 `MakeBridgeRegularHandler`가 객체의 가상 표 기록값으로 분배해 Regular 프로세스에 잇는다. 실제 `bridge.type`의 끝 프레임(L 47·M 50·N 53·O 56)과 글자 표를 만드는 시점(타입 후처리 `0049b0d0`)도 확인했다. Release 경고/오류 0·콘솔 CTest 내부 **211개·실패 0**(103.26초), 누적 제한 x86 **109,786개**. 기계어 대조 입력의 [Ghidra 내보내기 재현 절차](../docs/exe/ghidra-exports.md)를 정리해 **감사 25개가 모두 통과**한다. **호스트 `HJOW-Athlon`에서 게임/원본 실행과 클론 창 실행 없음.**

[2026-10-07 다리 이벤트 처리기와 끝 칸 변환 복원](../docs/exe/cpp-bridgeevent-reconstruction.md), [독립 x86 4,653개](recovery-bridgeevent-evidence.json) — 다리 객체의 이벤트 처리기(vtable +0x5c: `0x2691` 자기 destroy, `0x2692` 지연 낙하 payload를 풀어 끝 칸 L·M·N·O로 변환)와 방향 글자별 프레임 구간 표를 복원했다(`RawBridgeEvents`, `RiftTypeFrames::Run`). 이웃 탐색기·표면 알림·새 객체 생성·가상 destroy/소유자/Pop은 호출자가 연결하는 훅이고 호출 순서·인자만 원본대로 지킨다. Release 경고/오류 0·콘솔 CTest 내부 **205개·실패 0**(104.23초), 누적 제한 x86 **109,786개**. **호스트 `HJOW-Athlon`에서 게임/원본 실행과 클론 창 실행 없음.**

[2026-10-07 객체 부착 프로세스(ProcessForm)·Kernel·Regular 지연/주기 이벤트 복원](../docs/exe/cpp-process-reconstruction.md), [독립 x86 시나리오 360개·연산 5,893개(보강 재생성)](recovery-process-evidence.json) — 프로세스마다 Kernel 슬롯과 부모 객체 안의 form SID를 갖는 BaseProcess 생성자, 양방향 종속 체인, form의 공통 destroy(프로세스 소멸), 부모 삭제 때의 종속 정리, 정해진 시각에 부모의 이벤트 처리기를 부르고 반환값으로 재예약/종료하는 RegularProcess, 이벤트 조회를 복원했다(`SquidProcess`). 다리 지연 낙하(`0x2692`)의 예약/조회를 이 위에 올렸다. Release 경고/오류 0·콘솔 CTest 내부 **200개·실패 0**(97.58초), 누적 제한 x86 **109,786개**. 부모의 가상 pre/post/이벤트 처리기·new/free·로그·전파만 대체했다. 보강한 대조 입력 재생성과 변이 확인은 `HJOW-Athlon`에서 마쳤다. 다리 이벤트 처리기 몸체는 바로 위 단계에서 복원했고 다른 파생 프로세스·raw GameWorld는 후속이다. **호스트 DESKTOP-HJOW에서 게임/업데이터/설치 도구·클론 창 실행 없음**, 원본 여섯 디렉터리 2,782개 파일 SHA/목록 동일.

[2026-10-07 삭제 보상(SP)·SP 저장소·샘 풀 복원](../docs/exe/cpp-reward-reconstruction.md), [독립 x86 7,200개](recovery-reward-evidence.json) — 삭제 flags의 수신자에게 비용×비율(25%)을 지급하고 해체는 남은 HP 비례로 환불하며, 지급하지 않은 가치를 샘 풀에 쌓는 `0044c2f0`/CD `004626d0`를 판본별 x87 순서로 복원했다. 패치판 SP는 32비트를 32개의 9칸 DWORD에 흩뿌리고 난수 잡음을 섞는 저장소이며 **SP가 바뀔 때마다 전역 난수가 160번 전진**한다(`SpStore`). 로컬/소유자 SP·AI 지갑(0x4a6 하한)·UI 갱신 훅을 연결하고 삭제 장부(`SquidDestroyLifecycle`)의 보상 사건을 이 계산으로 대신할 수 있게 했다. Release 경고/오류 0·콘솔 CTest 내부 **191개·실패 0**(61.41초), 누적 제한 x86 **96,240개**. UI 갱신·시각 재시드·힙 할당·assert 보고·파생 vtable +0x80 개수만 대체했다. AI 부착 통지·종속 파생 메서드·파편/소리/낙하·이벤트·raw GameWorld는 후속이다. **호스트 DESKTOP-HJOW에서 게임/업데이터/설치 도구·클론 창 실행 없음**, 원본 여섯 디렉터리 2,782개 파일 SHA/목록 동일.

[2026-10-07 공통 삭제 Graph 분할·Free·주변 표면 Add](../docs/exe/cpp-destroygraph-reconstruction.md), [독립 x86 1,728개](recovery-destroygraph-evidence.json) — 공통 preDestroy의 Graph 분할/직접 Free와 postDestroy의 발자국→일반 finder→표면 Add를 실제 destroy/Unpop/SID 반납에 연결했다. Release 경고/오류 0·콘솔 CTest 내부 **182개·실패 0**(87.53초), 누적 제한 x86 **89,040개**(당시 수치).

[2026-10-07 공통 pre/postDestroy 장부 복원](../docs/exe/cpp-destroylifecycle-reconstruction.md), [독립 x86 1,728개](recovery-destroylifecycle-evidence.json) — 선택/abstract 배치 복구·공급/작업장/소유자 목록 제거·현재 타입 수 감소·누적 생산 수 유지·unitLost 사건/좌표·판본별 비용 차감을 공통 destroy의 실제 Unpop/SID 반납에 연결했다. Release 경고/오류 0·콘솔 CTest 내부 **175개·실패 0**(60.85초), 누적 제한 x86 **87,312개**. Graph 비활성·AI null·보상/SP·실제 소리/UI 대체 범위이며 공통 Graph 분할/Free·주변 표면 Add, 실제 보상/SP·AI 통지·종속 파생 메서드·파편/소리/낙하·이벤트·raw GameWorld는 후속이다. `0044c2f0`/CD `004626d0`는 이동 경로가 아니라 삭제 보상 계산으로 정정했다. **호스트 DESKTOP-HJOW에서 게임/업데이터/설치 도구·클론 창 실행 없음**, 원본 여섯 디렉터리 2,782개 파일 SHA/목록 동일. 아래 기록의 수치/창 허용/대체 범위는 당시 이력이다.

[2026-10-07 공통 Squid 삭제·실제 Unpop/SID 반납·다리 훅의 중첩 삭제](../docs/exe/cpp-destroy-reconstruction.md), [독립 x86 1,992개](recovery-destroy-evidence.json) — dead 선표시·종속 release 전 next 저장·전파/선택·실제 공간 해제/반납 순서를 복원했다. 일반 finder와 다리 훅 내부의 링크 삭제를 같은 풀에서 대조했다. Release 경고/오류 0·콘솔 CTest 내부 **169개·실패 0**, 누적 제한 x86 **85,584개**. 공통 pre/post의 Graph/목록/통계·종속 파생 메서드·파편/소리/낙하·이벤트·raw GameWorld는 남았다. `00460600`은 파편 생성으로 정정했으며 기존 사건 이름 NotifyRemoval은 유지했다. 호스트 `DESKTOP-HJOW`에서 게임/업데이터/설치 도구·클론 창을 실행하지 않았다. 아래 창 허용과 과거 수치는 각 당시 작업 기록이다.

[2026-10-07 일반 공간 탐색·다리 삭제 훅 연결](../docs/exe/cpp-rawfinder-reconstruction.md), [독립 x86 1,776개](recovery-rawfinder-evidence.json) — 네 단계 버킷/next·소수 좌표 발자국·기본 필터·동적 필드 조회와 커서 전체를 복원했다. 일반 Begin/Next를 대체하지 않고 삭제 훅 연결까지 대조했다. 당시 Release 경고/오류 0·CTest 내부 **163개·실패 0**, 누적 제한 x86 **83,592개**. 호스트 `DESKTOP-HJOW`에서 게임/업데이터/설치 도구·클론 창을 실행하지 않았다. 공통 삭제 일부는 위 후속에서 연결했고 낙하/소리·이벤트 실행·파생 탐색 필터·raw GameWorld는 남았다.

[2026-10-07 다리 삭제 전후 훅·낙하 이벤트 좌표](../docs/exe/cpp-bridgeeffects-reconstruction.md), [독립 x86 6,170개](recovery-bridgeeffects-evidence.json) — raw 참조 검사·주변 탐색 좌표 보정·제거 통지/소리/walker/공통 삭제의 호출 순서·signed 좌표 포장을 복원했다. Release 경고/오류 0·CTest 내부 **157개·실패 0**, 누적 제한 x86 **81,816개**. 이번 작업의 명시적 허용으로 `DESKTOP-HJOW`에서 클론 창의 캠페인/TEST01 회귀 20개 상태를 확인했다. 일반 탐색과 실제 삭제/낙하/소리·이벤트 실행·raw GameWorld 연결은 남았다. 원본 게임·업데이터를 실행하지 않았고 원본 2,782개 파일 SHA/목록은 유지했다.

[2026-10-07 다리 붕괴 스캔·한 칸 처리·수명 감소 전체](../docs/exe/cpp-bridgedecay-reconstruction.md), [독립 x86 14,524개](recovery-bridgedecay-evidence.json) — 스캔 커서(`00422bc0`)의 프레임당 `trunc(delta / 10 × 범위)`와 주기 끝 일괄 처리, 한 칸 처리(`004227e0`), 금 간/보통 프레임 전환, 다리 destroy 재정의를 세 실제 PE로 대조했다. `00540bc0`은 전투 모드가 아니라 서버 플래그로 정정했다. Release 경고/오류 0·콘솔 CTest 내부 **152개·실패 0**, 누적 제한 x86 **75,646개**. 실제 삭제(Unpop·그래프 분할·반납)·낙하·소리는 사건으로만 돌려주며 raw GameWorld 연결은 남았다. `DESKTOP-HJOW`에서 게임/업데이터·클론 창을 실행하지 않았다.

[2026-10-07 Add·Detach·Pop/postPop의 자동 Graph 소진 복구](../docs/exe/cpp-graphrecovery-reconstruction.md), [독립 x86 192회](recovery-graphrecovery-evidence.json) — 전체 자산 풀 계약 `GraphRecovery::FullPool`을 선택하면 상위 호출 내부에서 재구성→할당→flood/감소를 원본 순서대로 처리한다. 다른 void 표면/내부 타입의 손상을 Pop 전에 거부한다. Release 경고/오류 0·콘솔 CTest 내부 **145개·실패 0**, 누적 제한 x86 **61,122개**. SID 소진·form/process를 포함한 실제 전체 월드 계약·건물 부착·특수 조회·파생 삭제/참조 수명·raw GameWorld는 남았다. `DESKTOP-HJOW`에서 게임/설치/업데이터·클론 창을 실행하지 않았다.

[2026-10-07 전역 Graph 재구성·소진 할당](../docs/exe/cpp-graphrebuild-reconstruction.md), [독립 x86 288회](recovery-graphrebuild-evidence.json) — 전체 raw 풀의 무효 수집/전체 초기화·SID 순서·임시 그래프·flood·reserved/스택 보존을 복원했다. Release 경고/오류 0·콘솔 CTest 내부 **143개·실패 0**, 누적 제한 입력 **60,930개**다. Add/Detach/Pop의 자동 전역 재구성 연결·SID 소진 복구·특수 조회·건물 부착·파생 삭제/참조 수명·raw GameWorld는 남았다. **`DESKTOP-HJOW`에서 게임/업데이터/설치 도구·클론 창은 실행하지 않았고 GUI 검사는 다른 PC에 인계한다.**

[2026-10-07 같은 위치의 다른 SID 그래프 조회](../docs/exe/cpp-reference-versions.md#cpppj-후속-같은-위치의-다른-sid-조회), [독립 x86 384회](recovery-graphlookup-evidence.json) — 삭제 원천의 타입/프레임과 위치 머리의 그래프 번호를 분리했다. Release·콘솔 CTest 내부 **139개·실패 0**, 누적 제한 입력 **60,642개**다. 전역 소진 복구·특수 조회·건물 부착·파생 삭제/참조 수명·raw GameWorld는 남았다. 이번에도 원본/복사본·업데이터·클론 창을 실행하지 않았으며 GUI 검사는 다른 PC에 인계한다.

[2026-10-07 삭제 준비 분할·일반 다리/섬 Unpop](../docs/exe/cpp-graphremove-reconstruction.md), [새 독립 기록](recovery-graphremove-evidence.json) — 원본 일반 탐색의 이웃 순서·경계 접촉·0단계 위치 조회와 정상/rebuild 분할 정책을 복원했다. 새 x86 **1,158회**, x64 Release·CTest 내부 **137개·실패 0**, 누적 제한 입력 **60,258개**다. 전역 소진 복구·건물 부착·파생 삭제/참조 수명·raw GameWorld는 남았다. **최신 사용자 지시: 이 PC에서는 원본/복사본 게임과 모든 창 검사를 금지하며 다른 PC에 인계한다.**

[2026-10-07 영역 그래프·일반 다리/섬 비전투 Pop](../docs/exe/cpp-regiongraph-reconstruction.md), [기계어 기록](recovery-regiongraph-evidence.json) — 네 단계 해시의 교차 발자국/내부 spot 표면을 무효화한 뒤 Add한다. 최종 상태의 사전 검사와 체인 순환/소진 보호를 유지한다. 세 실제 PE의 새 x86 1,536회, Release 빌드·내부 검사 **134개·실패 0**. 일반 다리/섬 Unpop·삭제 분할은 위 후속에서 추가했고 건물 부착·전역 복구·raw GameWorld는 남았다.

[10.37 일본 유통 CD(실제 영문판 가능성) 디컴파일](../docs/exe/original1037jp-comparison.md), [근거](recovery-original1037jp-evidence.json) — 3,711개 함수·실패 0. 실행 파일이 `originalCD`와 바이트 단위로 같다. 파일은 `HJOW-Athlon`·`HJOW-X3D`에만 있고 Git에는 없다.

[추가 10.37 전체 디컴파일·CD 비교](../docs/exe/original1037-comparison.md), [근거](recovery-original1037-evidence.json) — 3,711개 함수·실패 0. CD와 실행 파일 전체 차이는 InsertCD 안내창 분기 한 바이트이며 두 `netstorm.ver`는 모두 10.37이다. 같은 그래프 배치를 새 알고리즘으로 세지 않고 패치 10.78을 복원 기준으로 유지한다.

[raw SID 그래프·공통 postPop 연결](../docs/exe/cpp-rawgraph-reconstruction.md), [독립 기계어 기록](recovery-rawgraph-evidence.json) — 실제 프레임·0단계 해시·spot에서 계산하고 graph byte에 반영한다. Pop의 최종 상태를 쓰기 전에 검사한다. 당시 x86 1,536회·내부 검사 132개가 통과했으며 영역/일반 다리·섬 Pop은 위 후속에서 추가했다.

[정수 표면 Graph 연결·flood·병합](../docs/exe/cpp-graph-reconstruction.md), [독립 기계어 기록](recovery-graph-evidence.json) — 기존 표/스택의 할당·반납·표면 수와 탐색 순서를 복원했다. 당시 새 x86 2,560회·내부 검사 128개가 통과했으며 raw 연결은 위 후속에서 추가했다.

[raw 공통 postPop 일부 효과 후속](../docs/exe/cpp-postpop-reconstruction.md), [독립 기계어 기록](recovery-postpop-evidence.json) — 비용 집계·공급/소유자별 작업장 목록·타입 통계·noGraph 리셋을 선택 연결했다. 당시 새 x86 984회·내부 검사 124개와 기존 1-1/TEST01 창 회귀를 통과했다. 표면 그래프는 위 raw 후속에서 연결했으며 영역 통지·AI/배치 선택·생산 덱/자원/SP 차감·raw GameWorld는 남았다.

[raw 공통 표시 활성 후속](../docs/exe/cpp-display-reconstruction.md), [독립 기계어 기록](recovery-display-evidence.json) — Pop·Unpop의 프레임 경계/선택/그림자 갱신을 실제 Renderer 변경 표로 연결했다. 새 x86 1,728회, 내부 검사 120개, 실제 SHP 추가 헤더 6,950개를 확인했다. 공통 postPop 영역/소유자/생산 효과와 raw GameWorld 연결은 남았다.

[raw 일반 Pop 후속·이번 PC의 GUI 회귀](../docs/exe/cpp-pop-reconstruction.md), [독립 기계어 기록](recovery-pop-evidence.json) — SID 풀·해시·spot의 등록→해제→반납을 연결했다. 표시와 postPop 효과가 억제된 비전투 경로이며 기존 GameWorld는 별개다. 2026-10-06 당시 허용으로 클론 창·글꼴·메뉴·1-1/TEST01 조작 회귀를 실행했다. 현재 PC의 창 금지 지시를 우선한다.

[타입·그래픽 복원과 판본 차이](../docs/exe/cpp-assets-reconstruction.md), [VFX 기계어 검증 기록](recovery-graphics-evidence.json).

[설정 계층 복원](../docs/exe/cpp-config-reconstruction.md), [설정 기계어 검증 기록](recovery-config-evidence.json), [타입 표·요새 파일 복원](../docs/exe/cpp-fort-reconstruction.md).

[창·화면 장치·입력 큐·영역 배치 복원](../docs/exe/cpp-screen-reconstruction.md) — 플랫폼 결정(Win32), 원본 시작 순서와 옮긴 범위, 원본과 다르게 둔 것.

[옵션 저장·전체화면 시작 표시 복원](../docs/exe/cpp-options-reconstruction.md) — 저장 시점·바이트 대조, 시작 표시 조회/생성, 전체화면 실패 시 원본도 창 모드로 이어진다는 분석 정정.

[Renderer·글꼴·커서 복원](../docs/exe/cpp-renderer-reconstruction.md), [표시 기계어 검증 기록](recovery-renderer-evidence.json) — 현재 계획의 1단계 표시 기반과 후속 연결점.

[다리 계산 복원·CD판 차이·신뢰도 보강](../docs/exe/cpp-bridge-reconstruction.md), [다리 기계어 검증 기록](recovery-bridge-evidence.json) — 모양·추첨·회전/프레임·열린 끝·수명 접두 구간을 복원했다. 배치 UI·표면 그래프·전체 붕괴의 월드 연결은 후속이다.

[표면 이웃·연결·붕괴 방문 목록](../docs/exe/cpp-surface-reconstruction.md), [표면 기계어 검증 기록](recovery-surface-evidence.json) — 정수 발자국/flag 8 이웃과 재귀 계산을 추가했다. 실제 표면/SID·spot의 GameWorld 연결과 수명/삭제·배치 UI는 후속이다.

[공간 해시·점유 비트 복원](../docs/exe/cpp-hash-reconstruction.md), [해시 기계어 검증 기록](recovery-hash-evidence.json) — 4단계 배열/버킷 주소·객체 단계·발자국/지붕 genus를 복원했다. x87 제어 워드를 명시해 두 판본과 53/64비트 결과를 확인했다.

[객체 공간 등록·해제](../docs/exe/cpp-spatial-reconstruction.md), [등록 기계어 검증 기록](recovery-spatial-evidence.json) — Pop/Unpop의 next 체인·spot OR/AND·좌표 캐시·비전투 상태와 CD판 점유 충돌 차이를 복원했다. 실제 등록 지도를 정수 SurfaceFinder에 전달한다. 가상/영역 효과는 반환 계약·사건 목록이며 SID 풀은 아래 후속에서 복원했으며 실제 GameWorld 연결은 남았다.

[SID 풀·번호 할당/반납](../docs/exe/cpp-sid-reconstruction.md), [SID 기계어 검증 기록](recovery-sid-evidence.json) — 두 판본의 raw 슬롯·번호 경계·FIFO/예측 할당·타입 보존 반납·삭제 기록·서버 목록 재구성을 복원했다. 새 3,168회는 두 판본 합계이며 기존 메모리 초기화와 소진 전 경로만 검증했다. 이 경로에는 세대 비트가 없다. form/process 생성자·공간 수명 효과·GameWorld 연결은 남았다.

[생성자 주소 표·기본 생성/Take·가상 초기화](../docs/exe/cpp-creation-reconstruction.md), [생성 기계어 검증 기록](recovery-creation-evidence.json) — base 생성·free/void Take·owner/HP/깊이를 SID 풀에 연결했다. 새 964회는 합성 타입 입력과 base 가상 초기화 범위이며 생성자 주소 표 359행은 별도다. 미복원 주소는 생성 전에 거부한다. 아래 후속에서 자산 파생 생성자를 연결했다. 공간 수명·실제 GameWorld 연결은 남았다.

[자산 파생 생성자·void Take](../docs/exe/cpp-derived-reconstruction.md), [파생 기계어 검증 기록](recovery-derived-evidence.json) — 패치 82개/CD 71개 생성자와 179개 타입 연결의 원본 쓰기를 복원했다. 새 6,028회는 합성 타입·실제 생성자·공통 가상 초기화·free/void Take 검증이다. 섬 번호/플래그·anim HP 폭·CD 중간 vtable도 보존한다. form/process·다른 가상 메서드·월드 연결은 남았다.

[raw 일반 공간 해제·non-void Take·firstPop 플래그](../docs/exe/cpp-unpop-reconstruction.md), [수명 기계어 검증 기록](recovery-unpop-evidence.json) — 표시 비활성의 일반 자산/매몰 객체를 실제 SID 풀·해시·spot에 연결했다. 새 3,618회에는 실제 non-void Unpop 768회와 CD 체인 미발견 차이를 포함한다. vtable 표 151행은 별도다. 섬·다리·건물 부착/파생 효과·표시 활성·실제 Pop/삭제·GameWorld 연결은 남았다. raw +8 word는 프레임이 아니라 섬 번호이며 이전 생성자 라벨을 정정했다.

**2026-10-06 최신 사용자 지시:** 이번 PC에서는 창 검사를 허용했다. 아래 window/renderer/menu/world 스모크를 실행하여 기본 GUI 회귀를 통과했고 원본/복사본 게임 프로세스는 실행하지 않았다. 이전 창 금지는 과거 PC의 작업 조건이다. raw Pop의 제한과 기존 임시 월드의 검증 범위는 위 후속 문서에 기록했다.

[실제 플레이 복원 계획](../docs/cpp-playable-plan.md)에 현재 실행 순서를, [기반 복원 로드맵](../docs/cpp-roadmap.md)에 완료된 데이터 기반·세부 복원 절차를 정리했다.

## 빌드와 테스트

Windows 10/11, CMake 3.21 이상, C++20 컴파일러(Visual Studio 2022 이상 또는 Build Tools)가 필요하다. 다른 OS에서는 구성 단계에서 멈춘다. 저장소 루트에서 실행한다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

실행 파일은 `cpppj/build/bin/Release/NetstormCpp.exe` 에 생긴다.

```powershell
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originalCD
python tools/cpp_recovery_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originalCD --cd
python tools/cpp_assets_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --export-frame originals sunCannon N00 0 extracted/sunCannon.bmp
cpppj/build/bin/Release/NetstormCpp.exe --config-spec originals missionSpec TEST01
python tools/cpp_config_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-mission originals TEST01
cpppj/build/bin/Release/NetstormCpp.exe --inspect-fort originals thewarbegins
python tools/cpp_fort_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --dump-territories originals
cpppj/build/bin/Release/NetstormCpp.exe --run originals --view types
cpppj/build/bin/Release/NetstormCpp.exe --run originals --view TEST01
python tools/cpp_window_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --run originals --view fonts --window --render-stats
python tools/cpp_renderer_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --run originals --window
python tools/cpp_menu_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --run originals --window --mission TEST01
cpppj/build/bin/Release/NetstormCpp.exe --dump-world originals thewarbegins
python tools/cpp_world_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-bridges originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-bridges originalCD --cd
python tools/cpp_bridge_smoke.py
```

`--run`은 클론 창을 띄운다(원본 게임을 실행하지 않는다). 기본은 메인 메뉴다. Campaign→Struggle For Freedom→1 The War Begins!→Play Mission에서 사제를 좌클릭해 선택하고 땅을 좌클릭해 이동한다. 화살표로 화면 이동, F4는 신전/F5는 사제 보기, Game/ESC는 정지/복귀다. `--mission TEST01`은 같은 브리핑/월드의 검사 진입점이다. `--view TEST01`은 옛 정적 검사 화면으로 화살표 이동/Esc 종료, `--view fonts`는 글꼴 표시다. `--dump-world`는 원본 파일을 읽기만 한다. [자세한 조작과 검사 옵션](../docs/exe/cpp-world-reconstruction.md).

`--run`은 원본과 공유하는 `d/options.cfg`를 갱신한다. 전체화면을 요구하면 게임 폴더의 `fullscreenStateFile.dat`도 원본 시점에 만든다. DirectDraw는 아직 없으므로 창 모드로 나온다. 스모크는 두 파일을 보관하고 검사 후 바이트·존재 여부를 복구한다.

현재 CTest는 **240개 내부 검사**, 누적 제한 x86 입력 **131,542개**다(기존 5,365＋다리 계산 18,053＋표면/재귀 3,450＋해시/점유 4,632＋공간 등록 878＋SID 3,168＋base 생성/Take 964＋자산 파생 생성/Take 6,028＋raw 일반 공간 해제/수명 3,618＋raw 일반 Pop/수명 4,600＋공통 표시 활성 1,728＋공통 postPop 일부 효과 984＋Graph 2,560＋raw Graph 1,536＋영역/일반 Pop 1,536＋삭제 준비/일반 Unpop 1,158＋같은 위치 조회 384＋전역 재구성 288＋상위 소진 복구 192＋다리 붕괴 스캔/한 칸 처리 14,524＋다리 삭제 훅/낙하 좌표 6,170＋일반 탐색/삭제 훅 연결 1,776＋공통 삭제/다리 중첩 1,992＋공통 삭제 장부 1,728＋공통 삭제 Graph 1,728＋삭제 보상(SP)·SP 저장소 7,200＋객체 부착 프로세스 5,893＋다리 이벤트 처리기/끝 칸 변환/글자 표 4,653＋소유자 지정 3,000＋flag 0 첫 이웃/이벤트 1,728＋다리 postPop 접두 240＋다리/섬 연결·소유자 전파 10,800＋프레임 지정 5,772＋섬 삭제 훅 1,560＋noIsland 최초 등록/받침 소유자 1,656). 공간 등록은 가상/영역 효과를 계약으로 대체한 조건부 검증이다. 삭제 준비/일반 Unpop은 공통 표시 활성·정수·dead 원천·소진 전·비전투 null 큐·합성 타입/SHP 범위이고 raw GameWorld는 미연결이다. 새 전역 Graph 288회는 전체 32768 raw 풀 순회와 소진 할당까지 검사하며 전체 자산 풀 계약의 Add/Detach/Pop 자동 복구 192회를 후속에서 추가했다. form/process·실제 raw 월드 연결은 후속이다. 앞선 raw 해제 기록은 표시 비활성·합성 기존 배치 상태이며 대체 함수가 없다. 수명 480개는 첫 외부 효과 전의 접두 구간, 해시 초기화 4개는 할당 없는 경로이고 전체 열린 끝은 패치판만 검증했다. 생성자 359행/vtable 151행은 호출 수와 별도다. 빌드에는 원본 실행 파일·Ghidra·Python·외부 라이브러리가 필요 없다. 기존 메뉴 54개 상태와 월드의 **6개 초기 자료 사례·393,216마스크 바이트·2,593객체·20개 조작 상태** 기록을 유지하며, 다리 자산 검사는 두 판본 합계 **936셀**을 독립 판독과 대조한다. `--inspect-bridges`는 파일을 읽기만 한다. 메뉴/월드 전체 함수의 x86 대조나 원본 화면 전체 픽셀 일치를 의미하지 않는다. 설정을 공유하는 창 스모크는 순차 실행하고 원본 파일을 복구한다. 이 수치는 게임 전체 완성도가 아니다. 일반 탐색은 실제 Begin/Next와 커서 전체를 대조했으며 합성 등록 상태·기본 true 필터·외부 삭제 효과 대체 범위다. 파생 필터와 raw GameWorld는 후속이다. 이전 공통 삭제 1,992개는 실제 destroy/다리 훅/finder/Unpop/반납·삭제 기록이며 공통 pre/post·종속 파생 효과·전파/선택·파편/소리/낙하는 명시적 대체다. 표시 억제·합성 등록/타입/SHP 범위이며 당시 공통 훅 효과는 대체였다. 최신 공통 장부 1,728개는 실제 pre/post·목록·통계·비용과 destroy/Unpop/Release를 대조했다. Graph 비활성·AI null·보상/SP·실제 소리/UI 대체 범위이고 raw GUI 월드는 미연결이다. 이후 공통 삭제 Graph 1,728개(실제 Graph 분할/Free/주변 표면 Add)와 삭제 보상 7,200개(실제 SP 지급/AI 지갑/샘 풀, 패치 SP 저장소 포함)를 더했으며 raw GUI 월드는 여전히 미연결이다. 그 뒤 객체 부착 프로세스 5,893개(실제 BaseProcess/Regular 생성·Kernel·form 부착/해제·공통 destroy)와 다리 이벤트 4,653개(실제 이벤트 처리기·끝 칸 변환·글자 표, 이웃 탐색·표면 알림·생성·destroy·소유자·Pop은 계약 대체)를 더했다. 이어 소유자 지정 3,000개(대체 함수 없음)를 더했다. 후속의 첫 이웃/이벤트 1,728개는 실제 일반 탐색·파생 연결 필터를 사용하지만 생성/삭제/소유자/Pop은 대체다. 다리 postPop 240개는 접두만 실행하며 연결 생성/소유자 전파와 공통 함수는 호출 기록 대체다. 실제 raw 모듈 통합은 별도 콘솔 검사이고 GUI 연결은 남았다. 최신의 다리/섬 연결 10,800개는 실제 연결 순회·탐색기(flags 0~7)·연결 객체 생성·소유자 전파·한 칸 조회를 실행하고 생성·가상 소유자 지정·Pop·프레임 지정을 대체한다. 프레임 지정 5,772개는 표시 갱신·Unpop·Pop을, 섬 삭제 훅 1,560개는 지연 낙하 예약·walker 낙하·공통 pre/postDestroy를 대체한다.

## 폴더

| 폴더 | 내용 |
|---|---|
| `src/o/` | 원본 `\Ns\O\` 의 공용 모듈 (게임 규칙·데이터·프로세스 커널) |
| `src/client/` | 원본 클라이언트 모듈 (화면·입력·소리·메인 루프). 원본처럼 Win32 를 직접 부른다 |
| `src/zacket/` | 원본 `\Ns\Zacket\` 의 네트워크 패킷 |
| `src/platform/` | 새로 쓰는 코드 — 원본에 없는 보조 기능(콘솔 출력·BMP 저장·파일 쓰기) |
| `src/app/` | 새로 쓰는 코드 — 실행 파일 진입점, 임시 검사용 화면 |
| `tests/` | 단위 테스트 (`TestSupport.h` 의 `TEST_CASE`·`CHECK`) |
| `cmake/` | 공통 컴파일 옵션 |

원본 소스 파일이 어느 경로로 가는지는 [SOURCE_MAP.md](SOURCE_MAP.md)에 있다(`python tools/cpp_source_map.py` 로 생성).

## 소스를 옮기는 방법과 규칙

[docs/cpp-build.md](../docs/cpp-build.md)에 정리했다: 디컴파일 결과에서 C++ 소스를 만드는 절차, 출처 주석 규칙, 32비트 코드를 64비트로 옮길 때의 주의, 아직 정하지 않은 것.
