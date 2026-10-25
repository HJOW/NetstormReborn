# 현재 Fort 이름과 실제 파일 장치 연결 — 2026-10-25 단위419

단위418 `9ce63eec` 뒤 AGENTS.md와 공통·닷넷 인계를 재독했다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 기존 정밀 C와10.78 보호 PE를 정적으로 읽었고 새 전체 디컴파일·원본 게임 실행·새 Native 실행·원격 push는 없다. 보호1821파일의 시작 기준은 `extracted/cpp-fort-name-419-baseline.json`이다.

## 이름/파일의 같은 물리

이미 복원된 `RawFortCurrentName`의00460980/004bf630 본체와876행 fixture는 수정하지 않았다. 새 `ClientFortGlobals`가 current/pending의 별도256BYTE, FileSpec9BYTE와 그 선택 포인터, 선택 모드, 같은50행 목록/dirty를 소유한다. 이름 버퍼는 원문00460610처럼 첫BYTE만0이며 NUL 뒤를 추가 초기화하지 않는다. 고정 소유자는 복사하지 않는다. pending의 실제 span 경로도 진단 후 현재 source를 재조회하는 전방 BYTE 복사와 범위 밖 중단을 유지한다.

현재/선택 이름과 같은00542678이 기존에는 `menuConfigRevision_`·`SoloMapCatalog.cache.dirty`로 나뉘었다. 이제 실제 메뉴의 설정 증가·이름 변경·목록 정리가 Client의 같은 DWORD를 직접 사용한다. 독립 Catalog만 자체 캐시를 사용한다. 원문 PE의 초기 dirty는 **1**이다. 최초 정적 검사 기대와 새 소유자의0을 발견해 빌드2 소스/제품을 보존한 뒤 실제 초기1로 고쳤다. PE의 두 이름 포인터 초기0, FileSpec 포인터005327ac, 선택 모드0도 확인했다. 이 기록은 정적 관측이며 새 Native 실행이 아니다.

`CampaignTemplateStorage::BindCurrentName`은 같은20BYTE 헤더와 해당 manager+c의256BYTE를 제공한다. `Name(slot)`은 manager의 실제 NUL 파일명을 읽어 기존 const string API에 투영한다. 이름을 바꿀 때 manager/서명/현재 섹션/stream을 재생성하지 않으며 같은 manager를 빌린 슬롯도 같은 변경을 본다. 캐시 문자열을 변경의 원본으로 두지 않는다.

`FortCurrentNameDevices`는 기존 전체 부모의 두 snapshot 요청/파일명 자체 Notify/readonly 캡처/후속 삭제/제목/dirty/설정 순서를 그대로 사용한다. 실제 Config.PathSpec과 현재 FileSpec 포인터를 읽고, 명시 편집 namespace만 기존 resolver로 검사된 절대 작업 경로에 매핑한다. FileSpec 선택은 버퍼 내용을 fortSpec으로 덮지 않는다. 같은 CRT의 NUL/대소문자 비교를 사용한다. 새 `EditorFiles::Delete`는 실제 삭제 직전에 기존 canonical 보호 검사를 다시 수행한 뒤 단일 파일 unlink를 호출한다. 원문0041aef0의 직접 경로 선택과 CRT004e58ba의 DeleteFileA 경계를 대조했다. 경로 없는 이름의 원본 기본 디렉터리 조합 전체를 새로 복원한 것은 아니다. 실제 편집 경로는 절대 작업 경로다.

Session 바인더는 같은 현재 State/Notify와 실제 Fort 전체 snapshot을 사용한다. 설치 전에는 실제 checkpoint 동기 범위 밖의 snapshot/Notify를 거부한다. 전체 Battle snapshot이 선택되면 명시 미연결을 유지한다. 실제 파일 장치 없는 삭제도 선택 위치에서 거부한다. 실제 Native 부모가 앞서 쓴 이름/manager/헤더는 뒤의 실패로 되돌리지 않는다.

## 실제 미션/선택 경계

`MissionFortRequest.selectFort`의004b51e0→00460980 위치를 현재 이름 부모에 연결했다. 기존에는 요청의 문자열 복사만 했다. 원문 Init이 연결된 미션의 현재 이름은 실제 Client 버퍼에서 읽고, 요청은 전체 이름 부모가 복귀한 뒤 실제 현재 이름을 소유한다. 제목·HTML 현재 맵 조회도 이 연결 범위에서 같은 값을 읽는다. 다른 기존 로더의 이름 계약은 아직 전체 Native State 전환 완료와 다르다.

실제 초기 미션의 ID는 `Tutorial1`이지만 원본 `loadFort`는 **`BridgeTheGap`**이다. 첫 GUI 도구가 두 이름을 같다고 잘못 기대해 종료1이었으며 클론 자체는 종료0이었다. 첫 도구/입력/실행/상태를 보존했고 보호 원문 `loadFort`를 읽는 검사로 고쳤다. 같은 제품의 메인 Campaign/Early/MORE/OK/F4/B., 목록/Back가 통과했다. 실제 Edit 작업목록의 TEST01 체크→TEST02 선택 예약과 취소도 통과했다. 이는 아직 소비하지 않는 next1 예약이며 Editor 설치 완료가 아니다.

## 집중 검사와 최초 실패

신규8검사는 두 고정 버퍼/진단 뒤 source/부분 복사, 실제 공유 dirty로 캐시 반납 후 작업 디스크 재조회, 같은 manager/빌린 슬롯/stream의 실제 새 파일 Notify 저장과 이전 파일 삭제, readonly 이전 파일 보존, Notify 실패 뒤 부분 상태,256BYTE 범위 중단, 보호 경로/단일 파일/디렉터리 유지, 실제 Client 이름을 사용하는 기존 Editor checkpoint 앞부분을 확인한다. 이름 장치 검사에서 snapshot 요청은 명시 관측 장치이며 게임 전체 snapshot 완료 증거가 아니다. 파일명/Notify/이미지 writer와 작업 파일 읽기·삭제는 실제 본체를 사용한다.

신규 checkpoint 검사는 실제 current/pending가 다른 상태에서 이름 부모→같은 작업 파일→Subscriber→World의 기존 Chat 미연결 경계까지 확인한다. Chat을 가짜 성공으로 바꾸지 않았고 checkpoint/loaded는 미완료로 남는다. 처음 사용한 설치 전 게임 메뉴 getter를 같은 실제 Client의 Gameplay/Subscriber/Player 소유자로 교정했다. 소유자가 서로 다를 때의 기존 거부도 유지했다.

첫 빌드는 새 보고서의 string_view→string 명시 변환 누락으로 종료1이었다. 빌드2~6은 종료0이다. 빌드3의 신규7검사 중 Notify 실패의 경로 기대 하나가 실패했다. 진단용 빌드4에서 실제 Notify 실패/현재 `After` 보존/manager의 정규화된 `after.fort`를 확인했다. 파일이 아직 없을 때 대문자 경로를 기대하던 검사를 기존 resolver 계약에 맞췄다. 빌드4/5의 checkpoint 검사는 각각 설치 전 메뉴 getter와 불일치 Subscriber owner에서 실패했으며 실제 Client의 같은 소유자들을 함께 제공했다. 제품 관문을 완화하지 않았다. 실패 소스1~5·바이너리·로그를 보존했다.

최종 빌드6의 게임 제품은 **`ad1bda98675901c6ddbf84433ebc4c84cb52932d57e345bb49476c95296b3f3e`**, 검사는 **`e9ff3afe9c2efb551ab326a7d7a91e12e9c216b49add95ed096445253648d388`**다. 빌드3~6의 게임 제품은 같고 마지막 세 빌드는 검사만 바뀌었다.22개 소스/빌드파일과 두 실행 파일을 `final-source6/`에 고정했다. 최종 집중은 장치7+checkpoint9+기존 Native 이름2+선택4+목록9+파일5=**36회·고유36·실패0**이다.

실제 자연 Early 회귀·전체 진입·전체 CTest·보호 감사 결과는 다음과 같다.

Early1 북서 경로는 **326.94초·종료0**으로 실제 A~G·SP0/200/400/600·사제 이동/귀환 뒤 Tutorial2 Guide4의 같은 프레임 브리핑까지 통과했다. 실제 현재 이름은 `BridgeTheGap`→`SecretWorkshop`이며 미션 ID와 구별된다. 두 단계에서 전체 HUD 루트1/구분선3·생산1/미니맵1을 유지했고 실제 다음 안내 이미지를 확인했다. 제품 타이머1·게임 상태 주입0이며 기존 완료값1을 보존한 실행이다. 새 진행0→1이나 디스크 저장 증거로 세지 않는다.

Early2는 **198.55초·종료0**으로 실제 A~I·신전/워크샾·발사기4개 생산/1개 회수·환불75/SP3075 뒤 Tutorial3 Guide4의 같은 프레임 브리핑까지 통과했다. 현재 이름은 `SecretWorkshop`→`CaptureThePriest`이고 다음 안내 이미지도 확인했다. 두 자연 실행은 같은 최종 제품이며 설정을 원래 바이트로 복원했다.

같은 최종 제품에서 원문 목록의 **일반31개(Early6/훈련9/캠페인16)와 실제 Demo 메뉴3개가 모두 초기 플레이 진입**에 통과했다. 일반31개 모두 실제 현재 Fort 이름이 비어 있지 않았고 데모도 같은 이름/전체 HUD를 보유했다. 데모는 원본15초 안내 timeout 뒤 AI와 실제 월드가 진행했다. 말기 캠페인/마지막 데모 이미지도 확인했다. 모든 실행 종료0·설정 복원 뒤 원본 동일이다. 이는34개 사용자 자연 완주/승패나 완전 동일성의 증거가 아니다.

기존 공용 `cpp_solo_entry_smoke.py`는 구형 control 행만 조건으로 사용했다. 실제 Native HTML 버튼 행도 인식하고 같은 실제 현재 이름/HUD 수명을 결과에 남기도록 갱신했다. 일반31 실행에 쓴 사본은 import 경로 추가를 제외하면 공용 도구와 같은 본체다. 데모3개는 갱신한 공용 도구로 실행했다. 게임 상태/단계/돈/다리 패턴을 직접 쓰는 도구로 바꾸지 않았다.

모든 GUI가 종료한 뒤 전체 CTest는 **2814검사·실패0·323.55초**로 통과했다. 검사 뒤 고정22개 소스/빌드파일과 두 실행 파일의 바이트가 같았고 보호1821파일과 원본/CD 파일명 목록도 기준과 일치했다. 최종 제품·검사·원시 실패/입력/결과의 SHA256을 [단위419 보고서](../../cpppj/fort-name-client-runtime-20261025.json)에 기록한다. 원본 게임 실행·새 Native oracle 실행·전체 재디컴파일은 이번 단위에서 하지 않았다. 마지막 전체 정밀 디컴파일 PC/날짜는 **HJOW-Athlon / 2026-10-23**으로 유지한다. 인수인계 갱신 뒤 **20261025-13 CP**로 로컬 커밋한다.

## 남은 전체 목표

현재 이름/파일 장치는 실제 Editor 설치를 위한 선행 연결이다. 전체 Native State1의 정리/prefix·현재 Fort/작업본·checkpoint·현재 HUD 명령/팔레트·State 확정/다음Poll4·loaded 공개와 실제 편집 명령, Create New Map/Save as/언어별 시나리오·Test Battle/Return·저장/재로드는 계속 남는다. **418 다음 인계의 “HUD mode4”는 교정한다.** [기존 실제 tail](../../cpppj/src/client/ExistingEditorAfterCheckpoint.cpp)004b743b와 `NotifyHudMode`는 현재 명령에1/0/숨은 자식 포함1을 보내며,4는 그 뒤 예약하는 **다음 State 번호**다. 일반 팔레트/Chat/구름 장치와 전체 Early·캠페인·세 Demo의 사용자 완주 및 완전 동일성도 미완료다. `/goal`은 활성 상태를 유지한다.
