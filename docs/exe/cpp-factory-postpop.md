# Factory·아웃포스트 실제 후처리

마지막 디컴파일 수행 PC: **VM-W11-CODEX, 2026-10-13**. `factorypostpop` 함수 내보내기는 패치 8개/CD 9개이며 기존 정밀 일반 finder 몸체를 함께 사용했다.

| 항목 | 패치 10.78 | CD 10.72 |
| --- | --- | --- |
| 실제 Factory vtable/후처리 | 00509148 / 004538f0 | 00503298 / 0047a4b0 |
| 실제 아웃포스트 vtable/후처리 | 00509210 / 004557c0 | 00503358 / 0047b680 |
| 상태별 프레임 구간 보정 | 00453780 | 0047a1f0 |
| 두 표의 가상 +0x80 상태 getter | 004ad0f0 | 004ace00 |

`RawFactoryPostPop`은 같은 SID 풀·네 공간 해시·현재 타입/프레임 표·장부를 참조한다. `SquidPostPop::SetFactoryPostPop`으로 실제 전체 몸체를 등록해야 파생 Pop이 허용된다. 지원 주소만 늘리거나 공통 장부만 실행하지 않는다.

아웃포스트 wrapper는 최초 flags1이고 extra `&9==0`일 때 별도 아웃포스트 목록과 기존 거주 구역 목록에 SID를 고유 추가한 뒤 섬 소유권 재계산을 요청한다. 목록 용량이 차거나 SID가 있으면 그대로 보존한다. 두 판본 모두 고유 추가다.

Factory는 최초 flags1에서 타입85의 event2/8초, 현재 타입87의 event1/0.5초를 각각 독립 할당한다. 패치만 event5/0.5초를 추가한다. 할당 실패는 해당 예약만 건너뛰며 다음 예약과 Damageable 후처리는 계속한다. 콜백 뒤 현재 타입을 다시 읽는다.

Damageable와 공통 장부를 한 번 실행한 뒤 상태별 첫 프레임·개수 구간을 검사한다. 타입126은 보정을 생략한다. 가상 +0x80은 해당 네 vtable에서 실제 base getter이며 raw+11 상위 세 비트를 반환한다. getter의 조건은 원본의 항상 참 OR 판정을 보존한다. 패치의 signed DWORD/CD의 unsigned BYTE 프레임 차이와 구간 끝의 DWORD 감김을 유지한다. 필요하면 `MakeFactoryPostPopHooks`를 통해 실제 `SquidFrame::Set`의 표시 갱신·Unpop/Repop을 호출한다.

권한이 있으면 현재 float 좌표를 CRT 방식으로 절삭하여 실제 `RawSquidFinder`의 한 점 검색을 진행한다. 자기 이외의 같은 현재 타입이고 extra XOR의 bit8이 같으면 경고 후 가상 삭제(flags0)한다. 다음 연결을 먼저 저장한 원본 finder가 삭제 뒤 타입/extra 변경과 다음 후보를 다시 읽는다.

독립 원본 입력은 **996개**(판본별 498개)다. 각 입력은 x87 53/64비트 두 정밀도로 실제 실행해 출력이 같음을 확인했으며 반복 반환을 고유 입력 합계에 더하지 않았다. 최초/재Pop·권한·abstract/buried·현재 상태/프레임 양끝·빈 구간·signed 값·개별 할당 실패·중복/가득 찬 목록·callback 뒤 타입/상태/다음 후보 변경을 포함한다. C++은 전체 슬롯·목록의 이전 메모리·외부 효과 순서와 후보 상태를 재생한다.

집중 검사는 native 재생, 효과 누락/다른 풀 거부, 실제 `SquidFrame`의 프레임 쓰기와 두 표시 갱신 연결 **3개**다. 성공 결과는 `factory-world.json`과 `tools/analyze_factory_postpop.py`가 최종 빌드·CTest·현재 SHA를 확인해 기록한다.

native의 new/Regular·Damageable·프레임 가상 효과·섬 소유권 재계산·로그/삭제는 명시 관찰 경계다. 상태 getter·프레임 구간 판단·일반 finder/CRT·고유 목록·Factory/아웃포스트 몸체는 실제 기계어다. C++에서는 기존 직접 Damageable→공통 장부를 조합하고 실제 프레임 지정까지 추가 검사했다.

아웃포스트 섬 소유권 재계산00455380/CD0047b3c0, 실제 Regular/건물 삭제 및 GameWorld 비표면 객체 이관은 남아 있다. 건설/전투/사제 운반·의식/승패와 캠페인 1-1의 실제 완주를 입증하는 단위는 아니다. 전체화면 DirectDraw 수정은 하지 않았다.
