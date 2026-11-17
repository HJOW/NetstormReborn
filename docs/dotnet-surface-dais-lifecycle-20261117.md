# dotnetpj 0.593 — 피해/해방 상위 분배의 Dais·Player·Aviary 장치 경계

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 피해/해방 상위 분배의 미연결 7주소를 호스트 장치 경계에 연결했다. 원문 포팅이 아니라 분배 라우팅과 장치 결선이며, 본체 이식은 잔여다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

83 VT표 중 분배(`EmitDefaultAssetEffect`)에서 미연결로 떨어지던 4개 Pre/Post 주소와 3개 통지 주소를 연결했다. 원문 근거는 `cpppj/src/client/RawSurfaceAssetDeletion.cpp`의 switch와 `RawSurfaceAssetNotice.cpp`의 `NotifyAviaryHome`이다.

- 0x0044a4d0(제단 전처리)·0x0044afd0(다이스 전처리)·0x00449580(다이스 후처리)는 `BindDaisLifecycle` 묶음(`DaisAltarPre`·`DaisPreDestroy`·`DaisPostDestroy` 공개 분배기)에 잇는다. 본체(`RawDaisDeletion`: typed 조회·제단 완료·사제 중지·Player 기록 pin·SquidProcessHost 연동)가 R6 장치에 닿으므로, `_vortexPre`와 같은 호스트 공급 장치 경계로 둔다.
- 0x004906c0(Player 전처리)는 `BindPlayerPre` 장치(`PlayerPreDestroy` 공개 분배기)에 잇는다. 본체(`RawPlayerWorld.PreDestroyNative`: Player 기록 view/pin·공통 Pre 래핑)가 R6 장치에 닿으므로 같은 경계로 둔다.
- 가상34 통지 0x0045b4a0·0x0045b5b0·0x0045b680(썬·맨 오 워·데빌메이커 홈)은 `BindAviaryHome` 장치(`NotifyAviaryHome` 공개 분배기, 주소를 함께 전달)에 잇는다. 본체는 비행장 Aviary 프로세스가 필요하므로 같은 경계로 둔다.

장치 없음·전송 전·대여 중·Mid night 중복 결선은 기존 `BindRegionLifecycle`과 같은 계약으로 거부한다. 미연결 실패 메시지에는 기존 계약대로 원문 진입점 주소를 함께 담는다(0x0044A4D0·0x0044AFD0·0x00449580·0x004906C0·통지 주소). 본체 3종 이식 때 분배 호출부만 실제 본체로 바꾸면 된다.

## 검증

최종 **Core 신규 5검사** 통과다. 결선 순서 강제·장치 전달·Emit 라우팅(다이스 2VT의 전/후·Player VT·세 비행장 VT의 통지 주소)·미연결 명시 실패를 확인한다. 원본 fixture 기대값은 바꾸지 않았다. 포팅 과정에서 기존 계약 테스트 2건(`UnconnectedPreStillCarriesAddress`·`UnconnectedBodiesCarryNativeAddresses`)이 새 분배와 충돌함을 확인하고, 메시지 주소 포함 계약을 유지해 기존 기대를 깨지 않았다.

집중 470/470(`Fence|CppPortMap|Surface|Dais` 필터: 신규 5 포함) 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았으며, 마지막 전체 측정(0.589 직전 2842/2846)에서 PortMap 중복 해소 뒤 자료 부재 3건(Burning originalCD 2건·PlayerTechnology NETSTORM_DATA 1건)만 남았다. Client 전체의 11건 실패는 0.589에서 작업 트리를 치워도 동일함을 확인한 사전 존재(환경) 실패다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 기존 대응 파일(`RawSurfaceAssetDeletion.cpp` 대응 `RawSurfaceStorage.AssetDeletion.cs`, 분배 대응 `RawSurfaceStorage.Process.cs`)의 본체 미완 부분만 채워 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 HostSession 별도 풀 전환이다.
