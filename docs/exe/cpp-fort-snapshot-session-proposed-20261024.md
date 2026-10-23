# 전체 Fort Snapshot의 Session 연결 제안

2026-10-24, 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 단위381 `fa7762f9`의 공용 소스는 읽기만 했다. [제안 patch](../../extracted/cpp-fort-snapshot-session-20261024-proposed.patch)는 기존 Session 두 파일과 신규 검사 파일 한 개를 포함하며 **공유 소스·공용 CMake·LEFT·공유 build·GUI·git은 변경하지 않았다.**

## 제안하는 API와 같은 저장소

| 제안 위치 | 실제 계약 |
|---|---|
| `RawCampaignSessionServices::fortBadges` | `shared_ptr<o::RawFortBadgesGlobals>`로 같은 Client6160BYTE 소유자를 공유한다. 원시 `.fort` Badges cursor나 별도 초기화 배열로 대체하지 않는다. |
| `RawCampaignSessionServices::fortMission` | `optional<o::FortMissionSaveHooks>`에 **현재 실제 Runtime**의 읽기/진단 장치만 공급한다. current0은 정상 입력이고 optional 누락은 장치 미연결이다. |
| `RawCampaignSession::SnapshotFort(slot)` | 설치된 Session의 명시 Fort 모드에서004bf9a0 전체 부모를 동기 호출한다. 모드·owner·inMission을 자동 변경하지 않는다. |
| `RawCampaignSession::BindFortFileSave(const uint32_t&,const EditorFiles&)` | 같은 Client readOnly DWORD와 실제 작업본 파일 장치를 빌려 `templateState->BindFileSave`에 연결한다. 두 참조는 Session보다 오래 살아야 한다. |
| private `WriteFortTechnology(parent,ref)` | `SaveFortContents(parent,1,ContentWriter(ref))` 본문만 쓴다. 독립 Technology snapshot도 같은 helper를 사용한다. |

구체 본문은 [제안 Session.cpp 사본](../../extracted/cpp-fort-snapshot-session-20261024-work/source/cpppj/src/client/RawCampaignSession.cpp)·[제안 Session.h 사본](../../extracted/cpp-fort-snapshot-session-20261024-work/source/cpppj/src/client/RawCampaignSession.h)이며, 선행 [바인딩 조사](cpp-fort-snapshot-session-bindings-20261024.md)의 위치와 심볼을 따른다. constructor·Load·MakeRuntime은 변경하지 않는다.

부모 순서는 **현재 manager/Fort/선택 진단→TypeNames→기술 getter·현재 manager/Create Technology·기술 본문→Objects→Territory→Deck→현재 manager/Create Money·Money 본문→현재 manager/진단/Create Mission·Mission 본문→Badges→Notify**다. Technology/Money/Mission Create를 각 단독 부모로 중복하지 않는다.

호출 지역 `vector<TemplateWriteStream>`는 manager/section/generation을 그대로 보관한다. 상위 토큰은1기반 표 인덱스이고, 기술 body의 내부 토큰은 항상1이다. 다른 child가 current를 바꾸어도 기술/Money/Mission은 최초 Create가 반환한 같은 참조에 쓴다. 지역 write callback을 services에 다시 저장하지 않는다.

Money는 `Impl::random` 자체를 `SaveFortMoney`에 전달한다. 기존 SP·영역 seed·게임 로직과 같은 RNG이며 현재 SP 복사나 별도 난수 객체를 만들지 않는다. Objects·Territory·TypeNames·Deck은 기존 실제 Session 메서드/풀/버전 DWORD/현재 장부를 재사용한다.

## 도달한 누락과 파일 저장

Missing Fort-mode 진단은 첫 child 전에 실패한다. Mission 서비스 누락은 부모의 **Mission Create 뒤** 실패한다. Badges 소유자 누락은 자체 부모의 **Create/초기 count BYTE0 뒤 첫 getter**에서 실패한다. 첫부분 쓰기를 되돌리거나 앞에서 서비스 전체를 검사하지 않는다.

마지막 Notify는 같은 `RawPlayerTemplateState::Changed`다. 파일 저장 callback이 없으면 원본 +8/isMine·현재 inMission·manager·+10/preventSave 관문 뒤 선택된 지점에서 오류가 난다. preventSave가 실제로1이면 자연 반환하며 저장 성공을 주장하지 않는다. manager0의 첫 filename BYTE 경계와 빈 정상 CString은 단위381의 [Notify 정밀 감사](cpp-fort-notify-next-20261024.md)를 따른다.

Bind는 `Storage.SaveFile(slot,{readOnlyForts,playerState.battleOwner,hiddenPlayers},image,files.Writer())`를 호출한다. hiddenPlayers는 같은 `battleOptions+0x3e`9BYTE다. ReadOnly·숨김은 실제 이미지 생성 뒤 저장 부모가 재조회하며 callback에서 선행 생략하지 않는다. 실패한 권한/파일 쓰기를 성공으로 바꾸지 않는다.

파일 이름은 현재 manager의 실제 이름이다. 캠페인 설치는 아직 mission base 이름을 사용하므로 실제 사용자 namespace/작업 경로를 준비하고 같은 manager 이름을 변경하는 후속 Client 연결이 필요하다. 검사에서 `InstallFile`로 준비한 작업 manager를 다시 설치하는 명시 입력은 제품의 이름 변경 대체안이 아니다. [Client 후속](cpp-editor-client-next-20261023.md)·[generic 설치 제안](cpp-generic-editor-install-20261024.md)을 별도로 통합해야 한다.

## 실제 Client에 남은 연결

실제 Client는 동일 Badges 소유자를 생성/초기화/reset/MetaLoad/저장에 공유해야 한다. 그 소유자는 fury의 별도 제안 범위다. ReadOnly는 `Client::ReadOnlyForts()`의 장기 DWORD 참조를 빌리며 매 저장마다 Config 값 복사본을 만들지 않는다.

Mission hooks는 Guide-only 여부와 관계없이 `UberGump::missionRuntime_` 현재 존재를 읽고, `Runtime::init.name`의 NUL 포함 안정 범위와 같은 `state.sequence/flags`를 제공해야 한다. field 토큰은 해당 Runtime 수명을 검증하는 불투명 값이어야 하며 host pointer를 DWORD로 자르지 않는다. Runtime 교체 때 토큰 세대를 바꾸고 현재 getter를 다시 읽는다. 이 제안에는 **UberGump 수정 hunk를 포함하지 않았다.**

`ClearWorld`가 Runtime을 Session보다 먼저 반납하므로 hooks는 고정 Runtime 포인터가 아닌 UberGump의 현재 getter를 빌려야 한다. 소멸 중 자동 Snapshot을 추가하지 않는다. ReadOnly/EditorFiles/Client 장부는 Session보다 오래 살아야 한다. generic Editor 설치 tail에서 Load 완료 전 snapshot을 호출할 때는 무검사 private 본문 분리가 추가로 필요하며 현 공개 `loaded` 계약을 임의 해제하지 않는다.

## 격리 검증과 한계

[신규 검사 제안 사본](../../extracted/cpp-fort-snapshot-session-20261024-work/source/cpppj/tests/CampaignFortSnapshotTests.cpp)은 공유 tests에 생성하지 않고 patch의 추가 파일 hunk에만 넣었다. [독립 CMake](../../extracted/cpp-fort-snapshot-session-20261024-work/CMakeLists.txt)는 별도 source/build/executable을 사용하며, 공용381 ABI `netstorm.lib` SHA256 `294838a30d18599db72c9afaaa2b2fdd230ec3529f32a8687094823ebdd2a991`를 읽기 링크한다.

첫 Release 빌드는 경고·오류0이며 최초 두 검사/실패check0이다. 최종 세 검사와 patch 정적 검증 결과는 [검증 기록](../../extracted/cpp-fort-snapshot-session-20261024-work/verification.json)에 기록한다. [생성 스크립트](../../extracted/cpp-fort-snapshot-session-20261024-work/propose.py)는 원본 두 파일의 SHA를 확인하면서 별도 사본과 diff만 만든다.

실제 왕복은 보호 TheWarBegins를 캠페인 Load한 뒤 **fort=1/battle=0/bridgeState.mission=0/objectState.mission=0**, 작업본 manager+8=0을 명시 입력한다. 실제 World에서 새 건물을 Create/Owner/Pop하고 현재 owner·기술·같은 Client 장부·Mission 상태·SP를 바꾸어 두 번 전체 저장한다. 파일 재파싱에서 모든 변경 섹션/전체 객체 버전/노드 순서를 확인하고 별도 실제 Session에서 같은 좌표·타입·owner가 재로드되는지 대조한다.

추가로 같은 ReadOnly 참조1→0, Fort모드/Mission/Badges/FileSave의 네 누락 지점, 현재 Mission0·preventSave1→0을 대조한다. Money 두 결과의 정수범위/현재 SP와의 차이도 관측하지만 새 RNG 상태 API를 만들지 않았으므로 Session RNG의 전체 소비열 replay 판정으로 확대하지 않는다. Money 본문 자체의 원문 난수 replay는 기존 독립 fixture 검사 범위다.

이는 **캠페인 설치 뒤 명시 Fort 저장 입력의 실제 왕복**이다. generic Editor 최초 설치·UI 메뉴·사용자 저장·Client 실제 Runtime/장부 업데이트 연결·Battle CoreData·EditorPlayers tail 완료를 뜻하지 않는다. 관련 통합 후에는 공용 빌드·기존 회귀·실제 사용자 경로 검증이 필요하다.
