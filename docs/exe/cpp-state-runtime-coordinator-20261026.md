# State의 실제 Runtime 반납 조정기 — 단위462

2026-10-26 **HJOW-Athlon**에서 원본10.78을 기준으로 작업했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 산출물을 바꾸지 않았다.

## 원문 위치와 실제 소유권

`004b2df0`은 owner1..8의 AI 정리 → Player 링크 → Kernel 단일 Clear → UI 정리·새 HUD → 색 표 → known0·Core0 → Pool 초기화 순서로 진행한다. C++ Session의 모듈과 고정 Player/표면 뷰는 앞의 자식을 실행할 때 살아 있어야 하지만, Pool·Player 재초기화 전에는 모두 사라져야 한다.

454에서 실제 AI·Deck·Kernel 정상 복귀의 이동 전용 증빙과 증빙을 모두 소비한 Session 소멸자의 보존 반납을 마련했다. 이번 `ClientStateRuntimeRetirement`는 이 자식들을 동일한 실제 Client/Session에 연결하고 Core0 뒤 반납을 조정한다. 새로운 전체 State 부모나 별도 게임 전역을 만들지 않는다.

- 생성 시 실제 Client 권한·보조 전역·표면·Kernel을 확인하고 현재 Session과 Player 고정뷰를 대조한다. 기존 표면 대여 세대도 보관하여 같은 메모리 주소가 재사용돼도 이전 조정기로 새 Session을 정리하지 못하게 한다.
- `ResetAi(owner)`는 실제 `ResetAiForState` 정상 복귀 뒤 같은 현재 모듈에 그 증빙을 소비한다. `ClearPlayerLinks()`도 실제9기록·Deck 부모와 같은 반환 증빙을 사용한다.
- `ClearKernel()`은 실제 단일 Clear의 증빙만 보관한다. 이 시점에 Runtime 처리기를 반납하지 않아 뒤의 UI 정리·교체가 현재 장치를 사용할 수 있다.
- `Retire()`는 실제 known0·Core0, 아홉 Deck 링크와 여덟 AI 반환 증빙을 확인한다. 같은 Kernel 증빙으로 처리기 대여를 반환한 뒤 외부의 실제 소유자 반납 장치를 동기 호출한다. 호출 후 이전 Session은 역참조하지 않고 현재 Gameplay/표면 대여가 모두 해제됐는지 확인한다. 소유자를 반납하지 않는 빈 callback은 성공이 아니다.
- 처리기 반납 후 외부 소유자 장치가 실패하거나 실제 반납을 하지 않으면 앞선 효과를 되돌리지 않는다. 같은 실제 반환 상태에서 소유자 반납만 재시도할 수 있으며 Kernel을 다시 Clear하지 않는다.

첫 메뉴에 실제 Runtime이 없을 때도 모든 게임 전역을 가짜0으로 제공하지 않는다. `ResetAi`는 실제 Player+0xb0이0인 원문의 자연 반환만 허용한다. 비영 brain이면 실제 owner 부재를 진단한다. Player 링크는 기존 `CleanupPlayerLinks`를 그대로 실행하여9기록의 무조건 쓰기를 수행하고, 선택된 비영 Deck의 free 장치가 없을 때만 그 위치에서 중단한다. Kernel은 실제 현재 Client 표를 단일 Clear한다. 이 분기를 전체 State 정리 완료로 판정하지 않는다.

## 검증과 범위

신규4개는 다음을 실제 Client/보호 미션/게임 모듈로 확인한다.

1. 실제 AI와9Deck·Kernel 정리 후 모든 고정뷰 반납, 소멸자의 게임 전역 보존, 이후 실제 Pool·Player 초기화 진행.
2. Core/한 owner 증빙/실제 소유자 장치 누락 거부, 빈 callback의 반납 실패와 처리기 재정리 없는 정상 재시도.
3. 첫 메뉴의 현재 비영 brain/Deck 거부와 앞선 실제 Player store 보존, 실제 Kernel 단일 Clear.
4. 다른 권한·표면·Kernel 및 다음 대여 세대 거부, 이전 Session이 사라진 뒤의 잘못된 재사용 거부.

Release build1 종료0·경고/오류0, 신규4개 포함 **집중197고유/202관측·실패0**이다. 기존 CampaignSession138개, 실제 자식 반납·Host·Player·State Native 회귀와461의 음향 검사도 같은 제품에서 통과했다. 기존 검사 본문은 새 include1개와 끝에 추가한 helper/4사례를 제외하고 시작 HEAD의 바이트와 같다.

직전461에서 고정한 명시 보호범위(`originals/`·`originalCD/`·`AGENTS.md`)1820파일 및 기존 fixture654파일을 이번 신규 검사 뒤와 집중 검사 전후에 대조하여 모두 같음을 확인했다. 이 집합을 과거1821파일 manifest와 같은 것으로 간주하지 않는다. 전체 SHA·검사 이름·원시 경로는 [실행 보고서](../../cpppj/state-runtime-coordinator-20261026.json)에 있다. 원시는 `extracted/cpp-state-runtime-coordinator-462-*`다.

이번에는 전체 CTest와 GUI를 반복하지 않았다. 전체 State 부모의 자연 호출은 아직 연결하지 않았으므로 실제 Edit 설치·새 HUD와의 통합 성공으로 세지 않는다. 마지막 전체 실행은459의3130개와 실제 Early1 ESC·메뉴 복귀·재진입이다.

## 다음 조립 위치

실제 `CleanupStateWorld`의 `resetAi`·`ClearPlayerLinks`·`ClearKernel`을 조정기의 같은 세 메서드에 연결한다. 원문 Core0 선택과 두 직접 store 뒤, `resetPool`의 실제 Pool 초기화 **앞**에서 `Retire()`로 기존 Runtime 소비자를 반납한다. 그 뒤 기존 `ResetClientSquidPool`과 실제 State 물리 정리·Player 초기화를 이어 간다. `UberGump::ClearWorld()`의 기존 일반 종료는 CaptureEnd·미션 정지·전역을 별도로 쓰므로 조정기의 반납 callback으로 그대로 호출하지 않는다. UI/Guide/입력 참조의 반납과 이미 정리한 게임 전역의 중복 쓰기를 분리해야 한다.

이어 전체34Effect와 비enum hook, 기존 Edit prefix/checkpoint/tail을 자연 Tick에 설치한다. 별도22칸 송신버퍼와 실제 Edit 편집·저장·전체 미션 완주·화면/음향 동일성은 계속 남는다. 이번 C++ 수명 조정기에는 Native 주소 대응 몸체를 새로 부여하지 않으며, CD10.72 전체 State 동일성도 주장하지 않는다.
