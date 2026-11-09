# 썬 펜스의 전체 장기 연결과 현재 TYPE 사거리

제품 **1209 / 11.01**, 2026-11-09, HJOW-Athlon. 내부 타입 `sunFence`(썬 바리케이드)의 전체 이웃·표식·연결선·재생성·수명을 같은 장기 저장소에 연결했다. 전체 목표는 미완료이며 [현행 인계](../../LEFT_JOBS.md)의 M6-n을 계속한다. [결과 보고서](../../cpppj/fence-owner-20261109.json)에 최종 소스·제품 SHA와 원시를 기록한다.

## 구현과 원문 근거

`RawSurfaceStorage`가 다음 **9본체**를 같은 Pool·현재 TYPE/프레임·실제 Player 동맹·Host에 연결한다. Native Session은 이 본체들을 빌리며 세션마다 별도 사본을 만들지 않는다.

| 본체 | 원문 / 연결 |
| --- | --- |
| `RawFenceNeighbors` | `455f00/455fb0/456220/4573d0`. 현재 사거리·방향별 최근접·후보 owner 행→부모 owner 열의 동맹·연결 수다. |
| `RawFenceLine` | `455b20` mode0. 실제 프레임 J/K·현재 포스 필드 역할·같은 가상 삭제로 연결선을 정리한다. |
| `RawFenceMarkerCreate` | `456460`. 현재 중심·내부 표식 역할·실제 생성/WORD/QA 통지/QB·가상 Owner/Pop 순서를 보존한다. |
| `RawFenceShieldCreate` | `455b20` mode1. 현재 포스 필드 역할/프레임·signed SID owner 선택·최대 100개 생성이다. |
| `RawFenceMarkerClear` | `4566d0`. 현재 중심·끝점 태그·같은 finder와 가상 삭제를 사용한다. |
| `RawFenceRebuild` | `457460`. 실제 동맹 이웃→사거리→표식 생성→현재 owner/좌표→포스 필드 생성 순서다. |
| `RawFencePostPop` | `457560`. 현재 theme·네 이웃 스냅샷·owner/연결 수 DWORD·실제 Regular 예약 뒤 기존 Damageable PostPop이다. |
| `RawFenceRegular` | `457a50`. 같은 Host가 현재 프레임/owner/연결 수·갱신/재생성·기존 Damageable Handle을 호출한다. |
| `RawFenceLifetime` | `4577f0/4578f0/457990`. 동맹 스냅샷/정리/Unpop/재생성/Repop 뒤 일반 네 이웃 중 **처음 두 개만** event2·0.1f로 예약하고 Damageable Pre를 호출한다. |

- `RiftTypeRecord.range`는 양판본 TYPE **+44**의 float다. 원문 전체 초기화의 0을 유지하고 속성이 나온 순서대로 기록한다. Native/typed 펜스 모두 현재 기록을 읽는다.
- 두 역할은 같은 `RawGameAuxState`의 `FenceMarkerType`(**005411c0/90**)·`FenceShieldType`(**00541300/170**)를 빌린다. 세션 전환이나 본체 결합으로 값을 초기화하지 않는다.
- 현재 Player 동맹은 실제 `ClientPlayerRecords`를 **weak 조회→선택 호출 동안 pin**하여 읽는다. 새 Player·동맹 배열 사본·전부 동맹 fallback을 만들지 않는다. Player base가 0이어도 실제 동맹 BSS는 읽을 수 있다.
- 표식 생성/정리는 필요한 **4adea0 중심 본문만** 같은 `RawFenceNeighbors`에서 공유한다. 이를 위해 별도의 GunTarget 상태를 만들지 않았으며 기존 독립 fixture의 GunTarget 생성자도 유지했다.
- Native의 Regular는 기존 실제 `AllocateResourceRegular`/`ConstructResourceRegular`로 각 new40의 성공/실패·같은 확보 주소·Host/현재 시각을 보존한다. typed 조립의 무조건 `true` 예약도 같은 실제 확보/구성으로 교정했다.
- 자산 삭제 분배는 실제 VT509470의 Pre457990을 선택한다. Session의 불필요한 TYPE136 추가 관문을 제거하고 원문의 가상표를 따른다. 기본 PostPop/Regular도 같은 장기 본체로 연결했다.

실제 TYPE90/136/170의 생성자·가상14/18/20/48/4c/5c/74/90을 보호 PE와 대조했다. 세 역할의 Unpop/Owner/Pop은 **4afe50/4adf00/4b02d0**, Repop은 좌표를 가상90으로 전달하는 **41c0d0**이다. 원문의 표식/포스 필드 Pop·Delete **0x40**은 전송을 억제하는 종속 처리 플래그다. 이를 그대로 사용하여 네트워크 전송을 가짜 성공으로 대체하지 않고 실제 생성/등록/삭제를 검사했다.

## 최초 컴파일 실패와 실제 소비자 교정

첫 빌드 **8733**은 고정20소스/672.97초 뒤 `RawCampaignDevastation.inc(66)`의 `fenceRanges` C2065로 종료1이었다. 제거한 펜스 사거리 사본을 기존 `RawBombTargets`도 공유하고 있었다. 최초 빌드·고정 소스는 `first-build.json/log`·`first-source-frozen.json`에 보존했다.

배열을 복원하지 않고 `RawBombTargets`에 **현재 TYPE를 직접 읽는 명시 생성자**를 추가해 같은 +44 필드를 연결했다. 기존 독립 fixture용 사거리 span 생성자는 유지했다. 현재 필드 변경/음수 절삭 회귀와 기존 300입력 원문 검사를 추가했다. 실제 반경 helper **41e3c0**은 정밀 함수 표에 없어 별도 PE로 `call4ac550; fld[eax+44]; jmp4e49c0`의 **13바이트와 SHA**를 보존했다.

전체 C/PE를 보존한 함수는 **20개**(CD TYPE 로더/초기화·중심 3개 포함)이며 별도 PE helper 1개와 구별한다. 21개 전체 C 디컴파일로 표시하지 않는다. 마지막 정밀 디컴파일은 Client 양판본 **2026-11-03 / HJOW-Athlon**, R **2026-11-07 / 같은 호스트**다. 새 디컴파일·원문 변경은 0이다.

## 검증

- 최종 **빌드75315 종료0**, 677.22초, 경고·오류0. Release/x64 제품2·집중5대상, **23소스 고정**, 4병렬이다.
- **집중62738 종료0, 비MP 237/237**, 44.31초. 기존194에 **신규15**(Native 펜스/공유 반경13·사거리 로더2)와 **기존 관련28**을 추가했다. 원문 fixture/기대값을 제품에 맞춰 고치지 않았다.
- 새 검사에서 현재 사거리/방향별 동맹·같은 두 역할·Player 반납/참조 순환·NoWorld 실제 Pre/Post·처음 두 이웃만 예약·현재 theme의 PostPop/실제 Host 재진입을 확인했다.
- 실제 **표식4개·포스 필드8개**를 생성해 원문 태그·owner·J 첫 프레임·같은 공간 등록을 확인하고, 현재 역할/태그/프레임을 사용한 실제 삭제까지 확인했다. 사거리 밖 재생성 생략→현재 사거리 확장 뒤 두 생성기의 실제 실행도 확인했다.
- 같은 EXE의 **Tutorial4 GUI28302 종료0**. 정상 UI 생산/건설 요청 → 실제 TYPE75/owner1·요청0→1 → **600프레임 대기**, 게임 시각13.353→23.911. `015-generator-settled.png`를 시각 확인했다.
- 같은 EXE/Client800×600의 **Editor13231 종료0**. TEST01↔Main **2회/11관측**, 실행 자료·EXE·보호 파일 불변이다.
- 최종 소스23/두 제품/보호 원본·CD·fixture·AGENTS **2,800파일**을 대조했다. 시작한 빌드·집중 검사·GUI는 모두 종료했다.

## 남은 범위

이번 추가는 **부모 분배71/83·잔여12쌍/전용 통지6건**이다. 큰 하위 효과까지 전체 NoWorld 구현 완료 수가 아니다. 큰 건설 효과16개/Client 장치10개와 실제 Runtime이 필요한 하위의 기존 계약은 유지한다.

큰 비영 AI·다른 Form/Regular·다른 배치 호출자/Fort 상위 Finish·M6 전수 caller/owner 감사·l/m·M7/M8·K/S/B의 **상위16계획과 등록34버그**는 남는다. 이번 연결을 알려진 버그 원인 조사/수정 완료로 세지 않는다. 최신 전체34진입 제품644/검증645·최신ZIP1092·최신 전체 솔루션 통과 기록 없음도 유지한다.

GUI는 건물 완성/Test Battle/전체 자연 완주 검증이 아니다. 사용자 제외 포획/희생·특정 공격·승리·공중3종 전용 관측·MP 단위/패킷/통신/게임/기록 검사는 실행하지 않았다. 원본 게임 실행과 원격 push도 하지 않았다.
