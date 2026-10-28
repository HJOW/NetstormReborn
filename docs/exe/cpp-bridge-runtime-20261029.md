# 단위552 — 다리 붕괴의 실제 프레임·이동체 연결

2026-10-29 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 산출물은 수정하지 않았다. 이 호스트는 AGENTS.md의 별도 창 실행 확인 예외다. 원본 게임은 실행하지 않았고 클론은 별도 자료 사본으로 실행한다.

## 구현과 원본 근거

[현재 스캔](cpp-bridge-decay-scan-runtime-20261029.md)의00422bc0과 [실제 한 칸·수명](cpp-bridge-decay-cell-20261029.md)의004227e0/00421c30을 실제 Client 프레임에 연결했다. 설정 저장 뒤·위치 소리00469f60 앞이며 메뉴에서도 같은 장기 저장소 커서/기한을 진행한다. 실제 칸에 도달할 때만 현재 Session을 요구한다. 독립 Session/World의 프레임도 같은 스캔을 Kernel 앞에서 요청하고 실제 Client는 전역 위치에서 한 번만 호출한다. 새 고정 틱·별도 상태 사본·생산 시 수명 선기록은 없다.

현재 boss/server/Editor는 효과 시점의 장기 주소를 읽는다. World는 실제 수명 WORD·현재 FrameCode·표면 통지·화면 갱신·파생 가상 삭제·Unpop·SID 반납을 사용한다. 파편/위치 소리에는 이른 스캔 시점의 현재 게임 시각/프레임을 먼저 공급하며 카메라/AI 준비를 중첩하지 않는다. `bridge_scan_global`, `bridge_decay`, `raw_bridge_fields`는 명시 CLI 보고의 읽기 전용 관측이다.

새 `RawBridgeCarriers`는00427de0의 현재 priest/rainWalker 판정과00427e00의 대피를 복원한다. 실제 칸 위 일반 finder와 반경5 일반 finder의 수명을 분리한다. 현재 이동 종류 선택→현재 Form 조회 뒤 이동이 없으면 현재 Graph·표면·다리 수명·거리로 후보를 고르고 실제 Session의 이동 시작에 전달한다. 최소 거리는 float에 저장하고 동률의 앞선 후보를 유지한다. 거리1도 허용한다. 직접 Retreat와 두 TYPE만 처리하는 wrapper의 범위를 구별하며 별도 Air3 관찰/검사를 추가하지 않았다.

원본421c30은 J/K의0수명 삭제에 flags02000000을 전달한다. [공통 삭제 원본](cpp-destroygraph-reconstruction.md)의004b0950은 이때 pre Graph 분할을 생략한다. 실제 객체가 반납되어도 Graph 표면 수11이 그대로인 합성 통합 결과는 이 계약에 맞는다. 삭제의 임의 보정/전체 Graph 재구성을 추가하지 않았다.

## 독립 원본과 집중 검사

[생성 도구](../../tools/decomp_bridge_carrier_retreat.py)는 실제10.78 PE의 일반 finder·Graph·대상 predicate·수명·거리·후보 선택을 실행한다. 큰 이동 종류 선택/현재 Form/이동 시작/진단 보고만 명시 경계로 대체한다. 258고유 입력 × x8753/64비트의516관측은 정밀도 차이0이다. [고정 입력](../../cpppj/tests/fixtures/bridge-carrier-retreat-x86.tsv)의 효과 기록과 네50바이트 슬롯 전체를 C++로 재생해258행 모두 일치했다. 경계와 해시는 [원본 보고서](../../cpppj/bridge-carrier-retreat.json)에 있다.

실제 등록된 이름으로 집중62검사/실패0을 확인했다. BridgeDecay15(실제 Runtime3 포함), BridgeCarrierRetreat1, RawSurfaceWorld7, DestroyGraph8, StateBridgeReset5, canon_surrounding5, BridgeLifecycle5, CampaignParticles3, rawPathStart2, destroy_dispatch5, DestroyLifecycle6이다. 등록 전체는3398이며 전체 CTest/C++를 새로 실행한 결과가 아니다. 두 Release target 빌드는 경고/오류0이다.

최초 대조에서는 거리1 제외 오류를 찾아 수정했다. 다음 실패는 비표면 해시 머리의 원본 진단1849를 검사 Graph 경계가 기록하지 않은 것이므로 그 경계를 수정했다. 실제 Runtime 검사의 마지막 Graph 수10 예상도 위 원본 noGraph 규칙에 맞춰11·전체 표 보존·실제 반납/비용 차감으로 정정했다. 실패 원시 로그를 지우거나 기존 기대값을 변경하지 않았다. 범위 설정 중 미등록 필터3개는0검사로 실패했으며 실제 등록 이름으로 교체했다.

## 같은 제품의 실제 실행

현재 게임 SHA-256은 `b6d6fc4c4f7309de0ec5f4779e074ec2130eacbe579f72336aa07da80002b22e`, 검사 실행 파일은 `05410a108e42a0ef37dc90739fe7645fd92bdcba5f462cc291a1c650c5fc7939`다. 일반31·Demo3의34/34 짧은 진입과 실제 메뉴/배치2개를 합친36게임이 모두 정상 종료했다. 34경로의 실제 플레이 보고 구간은 최소7.175초 증가했고 공통 전역/World 스캔의 처음·끝·커서·기한·주기가 모든 관측에서 일치했다. 34경로는1024×768 호환 전체화면이며 자연 timeout/표시된 안내 버튼 뒤 phase4·정지0·같은 미션/Guide·실제Session1/1을 확인했다.

[GUI 도구](../../tools/cpp_bridge_decay_runtime.py)의 창 모드 메뉴는 World 대여0·대상0 상태에서도 커서/기한이 진행됐다. 자료/초기 설정이 정하는 현재 범위를 그대로 사용했다. TheWarBegins에서는 명시 검사 입력으로 패턴0 J1만 고정하고 실제 Q→우회전→합법 미리보기→배치 클릭을 사용했다. 신규 SID15193의 **TYPE82·owner1·K1 frame41·위치110,97**을 확인했고 SP3000은 유지됐다. 배치 보고의 수명0이 자연 스캔에서7→6→5가 됐고 마지막 게임시각41.275초에도 같은 다리가 남았다. 배치 시각20.509초부터20.766초를 관측했다. 제품이 생성 시 수명을 미리 쓰도록 변경하지 않았다. 패턴 고정은 이1회뿐이며 일반34경로/메뉴에는 적용하지 않았다. 최종 캡처도 확인했다.

이 실제 관측은 균열 전까지다. 균열 프레임·위치 소리 요청·현재 server의0수명 삭제/반납·J/K Graph 보존은 실제 World 통합 검사에서 확인했다. 사제/rainWalker의 대피 후보/시작 인자는 독립 원문258행에서 검증했으며 실제 게임의 모든 운반 상황/음향 청취 증거로 확대하지 않는다. 최종 [통합 보고서](../../cpppj/bridge-runtime-20261029.json)에 각각 구분해 기록했다.

원시는 `extracted/cpp-bridge-runtime-552-20261029/`의 `before.json`, 원본 `native-v1/v2.log`, 빌드 `build-carriers-v1/v2.log`·`build-runtime-v1~v4.log`, 실패/최종 검사 `test-carriers-v1~v3.log`·`test-runtime-v1~v3.log`, 개별 집중 로그, 실제 실행 폴더에 보존한다. 최초218입력 뒤 소수 부모 좌표를 추가한 최종258입력을 구별한다. 최종 보호1820파일과 기존fixture711개는 모두 바이트 동일하다. 각 실제 실행의 PID/명령/게임SHA·종료/입력 계획/상태/캡처를 보존했고 자료 사본과 보호 원본은 전후 동일하다.

549의 자동 전술 실패/보류·전체 C++ 미재검증, 자연 완주/사용자 개입·스펠 도움말/직사각형/영문 부재 보류와 공중3종·MP 신규 검사 제한은 유지한다. CD의 별도 인라인 프레임 스캔, 모든 다리 모양/진행 상태·원본 외관/음향 전체 동일성, Win11 실기는 이번 완료 범위가 아니다. 전체 목표는 진행 중이다.
