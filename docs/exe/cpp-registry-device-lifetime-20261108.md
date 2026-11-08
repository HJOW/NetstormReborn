# C++ Registry 실제 장치·등록 Node·App 종료 수명 접합

2026-11-08 / 단위1178 / HJOW-Athlon / 표시·제품11.01. 1177 뒤 M6-l의 7ch 장기 F와 실제 장치 호출범위·등록 Node 보관·앱 최종 반납을 부분 접합했다. 전체 l/n과 지연 큐는 미완료다.

## 실제 선택과 호출 수명

실제 Client의 Registry만 Binding.node로 같은 PacketSources의 실제 device를 선택한다. SendMutable/Native·직접 송신·Readiness·Destroy·Release는 선택된 함수값과 실제 owner를 구별해 보유한다. 실제 선택 전의 슬롯·Playerflag·NULL 자연 반환과 초기 packet free 분기는 유지한다. 독립 typed/R의 빈 getter와 기존 계약은 보존한다.

Assign 및 Native 정리 부모의 마지막 slot/count 쓰기까지 최외곽 RunDeviceScope가 실제 owner들을 보유한다. 등록 당시 Binding.deviceOwner와 현재 getter가 선택한 Source 양쪽을 같은 control block 단위로 중복 제거한다. 따라서 개별 Release의 지역 사본 반납이 원문 부모 후속 쓰기 전에 옛 Source의 Forget/Close를 실행하지 않는다. 카운터 자체를 owner로 간주하거나 World/Session·지연 수신 F를 보관하지 않는다.

4df470(this,1)은 공통 직접 호출이며 flags2→flushedCursor=base→writeCursor=base+4를 같은 실제 노드에 적용한다. TCP의 남은 wire/sent/aggregate와 Editor/internal 현재 output만 재설정하고, 이미 전달된 반대편 데이터는 지우지 않는다. internal의 owner/물리 끝 선택은 현재 Registry와 node.sender를 따른다. UDP의 NULL base·논리 cursor 4는 실제 상태로 보존하지만 이후 flag2 UDP 송신/Flush는 별도 미완료다.

## 7ch 고정 본문

기본표에 4b2cc0/version1/policy0/28BYTE와 비NULL 빈40b380을 설치한다. 저장 F는 장기 Client만 참조하며 NULL connection·현재 Meta/phase/known·Session을 선행 관문으로 추가하지 않는다. Meta의 명시 호출도 같은 Client 본문을 사용한다.

status0은 현재 master/자기idx와 요청자의 subscriber/battle/seat를 순서대로 읽어 현재 sender에 응답한다. 수락 시에만 현재 seat를 재조회하여 Disconnect→server1→seat 재조회/Assign→정상 server0을 수행한다. 미작성 tail의 CurrentAllocationBytes와 예외 후 부분 효과를 보존한다. status1은 현재 master/subscriber 조건 뒤 같은 자기 CS record를 먼저 선택해 1fh header/subscriber·flags2000 저장·현재 flags 본문·전역11 송신을 연결한다. 기타 status는 cachedPeerIndex만 ffffffff로 쓴다.

## 등록 Node와 앱 최종 반납

Source는 소켓/endpoint를 열기 전에 shared 상태로 Publish한다. TCP/UDP는 실제 Node를 목록에 넣은 뒤 Track한다. 실제 Client Binding은 등록 Node 수명 동안 선택한 Source를 보유하므로 원문 전역 표에 남은 live/pending과 슬롯에서 덮인 노드도 임의 Source 소멸로 사라지지 않는다. World/Session은 강하게 소유하지 않으며 record callback의 weak World는 호출 중에만 lock한다.

최종 Client 소멸은 UI·시간·표시 장치 반납보다 먼저 ShutdownDevices를 호출한다. 같은 실제 Sources를 지역 shared 값으로 보유→기존 Native CloseAll/ClosePending→남은 실제 Binding/초기 packet 명시 반납→각 Source 닫힘/weak 장부 비움 순서로 순환을 끊는다. 이 추가 drain은 C++ 앱 최종 종료용이며 State7/19/Meta 정리의 대체가 아니다. Native pendingCount 미정규화·초기 packet 분기와 Forget 무pin 경계는 유지한다.

## 근거와 적용

클라이언트 마지막 정밀 디컴파일은2026-11-03, R은2026-11-07이며 둘 다 현재 HJOW-Athlon이다. 생성 원문과 원본/CD는 변경하지 않았다.

| 층 | 동결 사본 | 범위 |
| --- | --- | --- |
| Registry/7ch | `extracted/cpp-m6-registry-native7c-patch/rebase-1177/` |10파일, 원문11함수 C/PE |
| Node/App | `extracted/cpp-m6-global-device-owner-patch/node-app/` |7파일, 원문10함수 C/PE |
| 두 owner 보관 | `extracted/cpp-m6-global-device-owner-patch/node-app-owner-closure/` |1파일 추가 교정 |

각 집합의 중복을 합쳐 고유 원문 함수 수로 세지 않는다. inclusive PE 범위/원문·EXE·사본 해시는 해당 evidence와 현재 보고서 nativeEvidence에 보존했다. 최초 동결 사본을 유지하고 별도 추가 사본을 순차 최소 hunk로 적용했다. 실제 고유 제품 변경은14파일이며 적용 전후·동결·최종 SHA는 `extracted/cpp-resume-1178/`에 있다.

## 빌드와 허용 확인

NetstormCpp/NetstormRoot Release/x64 최종0·경고/오류0이다. 격리1920×1080/영어/무음 Tutorial1에서 정상 UI 다리 건설·실제 서버 객체/건설 요청 증가·90frame 대기·정상 종료0을 확인했다. TSV/상태/PNG를 관측했고 보호2800파일·설치 사본·EXE·동결14소스가 불변이다. 이 비MP 사례는 MP 수명/7ch 동작의 실행 검증이 아니다.

새 MP·단위·패킷·통신/ZIP 검사와 사용자 제외 포획·희생·특정 적 공격·승리·공중3종 전용 검사는 실행하지 않았다. 이전1177의 T4/캠페인1-1을 현재 새 실행으로 합산하지 않는다.

| 실행물 | 바이트 | SHA-256 |
| --- | ---: | --- |
| Netstorm.exe | 13,470,720 | `34c8d8bf59196ff67a787dc2046cdcc1876cfd6b67e405fc12c535a773383152` |
| R.exe | 677,376 | `95dc0dac85779a77421eb8afd8bc96f75e048f08258a4d4620aa7aea81638264` |

## 필수 잔여

장기 Node 보관과 전역 listener 재사용은 서로 다르다. 원문 비영 TCP/SPX/zlocal listener 재사용·Editor의 기존 zlocal 명시 교체, 실제 공통 수신표·전역 listener/pending/양수 Poll은 아직 미완료다. Source별 Poll 연속 호출로 같은 ID를 중복 순회하는 설계를 추가하지 않았다.

직접 Send/Fail/Notify·즉시 type1/2 Enqueue·실제 Node queue/depth 반납과 UDP flag2 송신/Flush도 남는다. Native F15태그/무태그73은 기본전역/observer/같은F대여반납·실제Source/수명 전범위 완료수가 아니다. 이 범위와 잔여 F를 마치기 전 지연 큐를 활성화하지 않는다.

M6-m JoinTestBattle·M6-n Host/자원파생/NoWorld caller·debugBurst와 M7 다섯 항목·M8, 이후 K1~K5·S1~S4를 계속한다. 전체 `/goal`은 진행 중이다. [현행 보고서](../../cpppj/registry-device-lifetime-20261108.json)·[LEFT_JOBS](../../LEFT_JOBS.md)를 따른다.
