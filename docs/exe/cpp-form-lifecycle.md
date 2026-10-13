# cpppj 감시 쌍·생산 목록 부착 복원

작업일 **2026-10-13**, 마지막 디컴파일 수행 PC **VM-W11-CODEX**. AGENTS.md와 두 LEFT_JOBS를 먼저 읽었으며 최신 정밀 디컴파일 PC와 현재 PC가 일치했다. 이 작업은 캠페인 1-1의 객체/UI 수명 연결을 위한 단계다. 캠페인 완주와 전체 원본 동일성은 아직 미완료이며 Windows 10/11 전체화면 DirectDraw 오류 수정은 제외한다.

## 구현

`DependFormHost`는 원본 인자 순서 `Create(watched, watcher)`로 두 타입1 Form을 같은 SID 풀에 만든다. watcher 쪽 Form을 먼저 할당·부착하고 watched 쪽 Form을 두 번째로 부착한다. 어느 한 부모가 client 번호면 두 Form 모두 client 영역을 쓴다. 서버 쌍의 Pop은 현재 raw 상태/flags를 전달하는 명시 전송 경계를 사용한다.

조회는 watched 안의 감시 Form에서 상대 부모를 확인한다. 제거는 watched가 dead이면 먼저 반환하며 일치하는 모든 쌍을 삭제한다. FormFinder처럼 next를 콜백 전에 저장한다. 생성은 ProcessForm 부모와 같은 부모 안의 두 Form도 허용한다. 제거 helper의 타입 제한을 생성에 추가하지 않는다.

가상 preDestroy는 watchedFid 상대→콜백 뒤 현재 watcherFid 상대 순서로 중첩 삭제하고 공통 깊이를 반납한다. 같은 Form의 재삭제는 기존 공통 삭제기의 dead 분기에서 반환한다. 부모 삭제 통지는 상대 Form이 client 번호이거나 서버 권한일 때만 watcher 부모의 가상 +34 효과에 도달한다. 콜백 후 watchedFid를 다시 확인한다. Unpop/반납·최근 삭제 기록·부모 dead의 링크 보존은 기존 공통 처리기를 공유한다.

`SquidForms`에는 기본 머리 부착과 공통 Pop을 추가했다. void 해제→서버/로컬 flags 정규화→필요한 전송→빈 Form postPop의 순서를 보존한다. 잘못된 상태와 필수 전송 경계 누락은 쓰기 전에 거부한다. 기존 GumpForm/ProcessForm 전용 처리기의 자동 전체 월드 연결을 완료했다는 뜻은 아니다.

`ContentFormHost`는 목록 flags의 **0x300을 제외한 그룹**과 기존 순서 BYTE를 읽어 첫 순서 공백에 삽입하고, 없으면 전체 체인 끝에 넣는다. 일반 Form/contained 자산이 섞인 previous 필드 위치 차이를 유지한다. 순서 BYTE는 패치 +23/CD +22다. 부착 후 부모 추가1, 살아 있는 부모에서 제거 후 제거2를 알린다. 기존 `RawContainedCreate`의 생성·필드 쓰기 순서를 유지하는 부착 연결기도 제공한다.

Robust는 WORLD(0x80) assert 경로를 거부하고 0x34를 검사한다. 양수 피해는 현재 부모 저장→Unpop→제거 알림 뒤 현재 0x100 플래그 해제→로컬 재부착→0 반환이다. 피해가 0/음수면 입력 amount를 유지한다. **[후속 원본 의미 교정](cpp-content-lifetime.md): 전투 부착 경계는 사제 포획 알림이며, 검사하는 것은 목록 flags가 아닌 부모 체인 포함 여부다.** API와 현재 연결 검사를 교정했고 내용물 특수 preDestroy의 너겟 지급도 후속에서 구현했다.

## 원본 근거와 독립 대조

`tools/ghidra/formlifecycle-functions.json`으로 두 PE의 **25/25개 함수**를 읽기 전용으로 내보냈다. 최초 CD 목록에 있던 불필요한 주소는 제거했고 실제 가상 표/실행 추적으로 필요한 thunk·finder 필터·깊이 반납 함수를 추가했다. 최종 산출물과 각 단계 로그는 `extracted/formlifecycle/` 및 `extracted/formlifecycle-export*.log`다.

| 역할 | 패치 10.78 | CD 10.72 |
| --- | --- | --- |
| 감시 쌍 생성 | 004b1510 | 004af940 |
| 부모 통지 / preDestroy | 004ae410 / 004ae4b0 | 004af600 / 004af6a0 |
| 상대 부모 / 감시 조회 / 제거 | 004ae510 / 004ae900 / 004ae780 | 004af700 / 004afbc0 / 004afa50 |
| Content 정렬 부착 / Robust / 피해 | 004aec10 / 004aea60 / 004af070 | 004afff0 / 004afe20 / 004b0350 |
| 기본 Form 부착 / Pop | 004ac800 / 004ac560 | 004ac2e0 / 004abfb0 |

`tools/decomp_formlifecycle_oracle.py`는 C++를 읽거나 호출하지 않고 두 실제 PE를 별도 Unicorn 주소 공간에서 실행한다. 각 판본 **456개**, 합계 **912개**의 루트 관찰을 fixture에 저장했다. 판본마다 생성11·삭제12·통지8·조회16·제거9·부착136·Robust256·양수 피해8이다. 준비 초기화는 이 수에 포함하지 않는다. 내부 감시 재삭제·공통 Unpop·SID 반납/삭제 기록·실제 가상 postPop/postDestroy도 실제 명령이다.

client/서버/혼합 번호, 서버 권한에 따른 통지 생략, 양방향 삭제, 부모 dead 링크 보존, ProcessForm/같은 부모, 중복 감시 제거, 0~4개 혼합 항목, 머리/일반 Form 뒤/자산 뒤 순서 공백, 전체 low7 Robust 플래그, 피해의 제거 콜백 payload 변경을 포함한다. 명령 범위/쓰기 범위를 제한하고 정상 EIP/ESP·보존 레지스터를 확인했다. assert 도달은 두 판본 모두 0회다.

명시 경계는 부모 +24/+34 통지·서버 전송·선택·진단 출력이다. 실제 부모 UI/생산 효과와 패킷 송신을 실행한 검증이 아니다. 당시 전투 부착 경계의 C++ 검사는 실제 사제 포획 경고를 실행한 근거가 아니다. 전투/사제/워커 부착의 독립 PE 경계 대조는 위 후속에서 수행했다.

새 C++ 검사는 fixture의 모든 watched 슬롯·콜백 당시 두 슬롯 전체 바이트·풀 카운터/머리/꼬리·공통 깊이를 직접 비교하고 나머지 풀은 Adler-32로 비교한다. Adler-32는 전체 바이트의 암호학적 동일성 증명이 아니다. 별도 검사로 쌍 생성→조회→제거·콜백 중 같은 쌍 삭제·원본 삭제 기록 순서·contained 실제 생성→부착·현재 생산 보충 상태·피해/알림·오류 계약을 확인한다.

## 검증과 후속

최종 Release 라이브러리/게임/검사 빌드 **경고 0·오류 0**, Form 수명 검사 **4개·실패 0**, **912개 PE 출력/C++ 대조 통과**, 최종 전체 CTest **863개·실패 0**(85.59초), PE/몸체/fixture 해시·사례/정상 명령 근거 감사 통과다. 처음 706개 입력의 전체 회귀도 통과했으며 최종 순서 공백/중복 사례는 912개 fixture에서 다시 검증했다. 중복 재실행은 별도의 새 독립 입력 수로 누적하지 않는다. 이전 누적에 더한 원본 관찰은 **525,146개**, C++ 대조는 **524,886개**다. SOURCE_MAP 내용 차이는 없다.

ContentForm 특수 preDestroy/너겟 지급과 공유 Form 삭제 분배·실제 표면 월드 프로세스 연결은 위 후속에서 구현했다. 초기화 앞 UI/Kernel 정리·전체 Client/비표면 월드 등록·사용 중인 지정 프로세스 재등록, 실제 사제 포획 경고/커서 효과·동적 메뉴/현재 UI 전역/명령 큐/변경 영역·OS 입력/overlay·자동 raw UI·건설 GUI·캠페인 완주는 계속 남았다.

게임/GUI는 실행하지 않았고 보호 PE 두 파일의 SHA-256은 기존 기록과 같다. AGENTS.md·보호 원본·dotnetpj 변경, Git 커밋/푸시는 없다. 근거 보고서는 `cpppj/recovery-formlifecycle-evidence.json`, fixture는 `cpppj/tests/fixtures/formlifecycle-x86.tsv`, 로그는 `extracted/formlifecycle-*.log`다.
