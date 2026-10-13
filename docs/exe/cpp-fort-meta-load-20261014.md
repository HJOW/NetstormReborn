# Fort Badges·Technology 상위 복원과 같은 풀 Contents 연결

작업 PC 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**, 날짜는 **2026-10-14**다. 10.78이 cpppj 복원 기준이며 CD10.72의 실제 주소·차이를 별도로 보존했다. 보호 원본은 읽기만 했고 게임·GUI·OS 호출은0이다. 이 작업은 단위12용 새 파일이며 CMake/LEFT/공유 빌드와 커밋은 root 담당이다.

## 주소와 적용 순서

실제 저장 Fort 부모의 후처리는 **Money→Badges→Technology**다. 앞의 Money와 기존 Terr→Chaff/Contents는 완료 몸체를 재사용한다. 새 `RawFortMetaLoad`는 장기 raw Badges 저장소·현재 loadingDepth·Template/현재 플레이어 상태를 참조한다. `MakeFortMetaContentsHooks`는 이미 파싱한 같은 FortTemplate의 원본 Technology 섹션과 현재 스트림 identity를 확인해 기존 `RawFortObjectLoad::Contents`로 전달한다.

| 몸체/현재 전역 | 10.78 | CD10.72 |
|---|---|---|
| Badges wrapper | 004be8e0 | 00428fc0 |
| 수량 setter | 0047d800 | 00475660 |
| 첫 타입 linear lookup | setter 내부 | 00475560 |
| Technology wrapper | 004beee0 | 004293d0 |
| isMine 판정 | wrapper 인라인 | 00426fe0 |
| 변환표 정리 | wrapper 인라인 | 00426830 |
| loadingDepth | 005c89b8 | 005178d4 |
| 변환표 pointer | 005c89bc | 005178d8 |
| Badges 28개220BYTE 표 | 00593160 | 0052ae20 |
| Template 기준/현재 owner | 005c89f0/00540cac | 00564bd0/0050f6d4 |

현재 정밀 내보내기는 `tools/ghidra/fortmeta-functions.json`의 **4/11주소**다. CD00426fa0의 일반 Template ordinal 및 양판본 TypeNames 전신도 읽기 자료에 포함되지만 이번 native는 그것을 새 전체 완료 몸체로 세지 않는다.

## Badges의 실제 저장 의미

표는 **28×220=6160BYTE**, type은 기록+0의 DWORD이고 count는 **+216의 DWORD**다. 0..255 밖의 count는 원본 line0x165b/CD0x10df 진단 뒤 계속하여 DWORD 전체를 쓴다. BYTE truncate/clamp를 추가하지 않는다. 28개 선형 순서의 **첫 타입 일치**만 변경하고 못 찾으면 아무것도 쓰지 않는다. callback이 표의 현재 type을 바꾼 경우도 진단 뒤 다시 검색한다. padding과 다른 기록의 전체 바이트를 대조한다.

Badges는 Template pointer0을 진단한 뒤 섹션 조회(`Badges`, resetpos=1)로 pos0을 만든다. data pointer가nonnull이고 end가nonzero일 때 countBYTE→typeBYTE/countBYTE 쌍을 순서대로 읽는다. 이 raw 직접 읽기에는 `pos>end` 진단이나 end 증가가 없다. C++는 물리 bytes가 부족할 때 계약 오류로 거부하며 이 거부를 원본 정상/assert 완료 입력으로 세지 않는다.

**CD는 진입 시 depth+1 후00429085→00429f30이 즉시 depth−1을 수행한다.** Template 진단과 섹션 조회보다 앞이며 함수 끝에 한 번 더 감소시키지 않는다. 10.78 Badges에는 이 깊이 쓰기가 없다.

## Technology의 실제 상위 순서

depth+1→Template pointer 진단→isMine 진단→현재 technology 부모 **0048fc60/CD00406d90**→`Technology` 섹션 조회/resetpos1 순서다. 섹션이nonnull/nonempty일 때만 **004bd800/CD00427e00** TypeNames 준비→**004bd130/CD00426dd0(parent,stream,0x11,1)**→현재nonnull 변환표 free→pointer0을 수행한다. missing/empty Technology에서는 기존 변환 pointer가 남는다. **0042f540/CD004b2e70 UI**는 빈 섹션에서도 실행하며 마지막 현재 depth에서−1한다. UI가 depth를 바꾸면 바뀐 현재 값을 감소시키는 것도 관찰했다.

Contents의 네 번째 인자1은 초기화가 아니다. `(param4==0)&&debug&&mappedType==0` 진단을 건너뛰는 **relaxedConversion**이다. 현재 parsed 목록에서 type0을 만나 자연 false로 끝나는 기존 Contents 의미를 유지하며 Technology 상위는 그 반환값을 사용하지 않는다.

Technology의 stream.end를 consumedpos까지 늘리지 않는다. TypeNames 준비 몸체가 조정하는 **별도 TypeNames stream.end**와 구별한다. samePool 어댑터의 pos는 기존 parser의 `Technology.size−technologyTrailingBytes`로 연결하고 end는 그대로 보존한다. 이 어댑터는 이미 파싱한 자료와 현재 metadata가 동일하게 유지되는 계약이며 runtime TypeNames 전신을 대신 완료했다고 주장하지 않는다.

## 독립 PE 관찰과 C++ 검증 범위

`decomp_fortmeta_oracle.py`는 제한된 두 PE에서 Badges wrapper/setter와 CD lookup, Technology 상위/SEH/depth/isMine/free gate를 실제 실행한다. 섹션 lookup·현재 부모 getter·TypeNames prepare·하위 Contents·free/UI·진단 보고는 이름이 정해진 **외부 경계**다. synthetic Contents 경계는 pos5만 소비하며 end를 보존한다. 입력/출력과 전체6160BYTE 무손실 run을 fixture에 저장하고 x87 control0x027f/0x037f의 동일 결과·정상 스택/보존 레지스터를 확인했다.

| 판본 | 고유 입력 | 정상 | 진단 후 반환 | 반복 ABI 복귀 |
|---|---:|---:|---:|---:|
| 10.78 | 470 | 192 | 278 | 940 |
| CD10.72 | 480 | 202 | 278 | 960 |
| 합계 | **950** | **394** | **556** | **1900** |

fixture SHA256은 **47c1ad6441ef2a9b62e010f32862171cb7ead5f174c4f7edbad81f037c2fe4a9**다. smoke/두 x87 반복·기존1218 FortObjectLoad 관찰·새 samePool 소비자 연결을950개에 추가하지 않는다. `python tools/decomp_fortmeta_oracle.py --verify`와 `python tools/analyze_fortmeta_cpp_binding.py`가 현재 source/tool/PE/export/fixture SHA 및 기존 **5bdb5be 전체 몸체**의 Git/작업 바이트 출처를 연결한다.

새 `FortMetaLoadTests.cpp`의 **TEST_CASE3개**는 950행 전체 장부/pos/end/depth/변환/호출/진단을 대조하고, 같은 실제 Factory→ContainedCreate→ContentForm으로 두 Contents를 생성하며 부모/형제 링크·수량 WORD·공유 난수 보존을 확인한다. 부모 getter/변환 준비/해제/UI는 계속 명시 경계다. 선택 Contents 분기에서 HP/Frame/Pop/Owner/일반 객체 삭제 등 미복원 가상 요청이 발생하면 즉시 거부한다. 부족한장부/필수효과/물리stream/다른풀/다른parsed섹션도 거부한다.

**현재 C++ 공유 등록/실행 대기**다. 전체 Fort 부모·TypeNames runtime 전신·실제 UI·GameWorld/캠페인1-1은 미완료다. 모든 hooks를 성공 noop으로 채운 완료 판정은 하지 않는다.

## 단위12 첫 공유 빌드의 namespace 교정

첫 공유 빌드의 C1075는 `RawFortMetaLoad.cpp` 마지막 namespace 닫힘 누락이었다. 당시 h/cpp·3검사·950fixture·두보고서·두도구·spec·문서·초기/두번째 실패로그를 **전체12파일**로 `hist/fortmeta-bef-039ea`에 보존했다. 생산 CPP EOF에 **`}`와LF의2BYTE만** 추가했고 정확히 그2BYTE를 제거하면 이전 전체파일과 동일하다.

950행/원본 결과/native 도구·기존 보고서의 당시 source SHA는 불변이다. 현재 CPP source SHA는 새 `fortmeta-namespace-lineage.json`으로 연결한다. **새native0, 기존950=정상394/assert556 유지**다. 현재 전집 감사 명령은 **`python tools/analyze_fortmeta_namespace_lineage.py`**다. 이 도구는 초기 원문 두 감사기를 그대로 읽고 해당 CPP만 교정전 whole snapshot으로 명시 연결해 모든 다른 현재 PE/tool/export/fixture/의존성 SHA를 엄격히 확인한다. 초기 두 감사기의 standalone 현재CPP SHA 불일치를 숨기거나 원래 보고서의 source SHA를 새 값으로 덮지 않는다. 공유 재빌드와 새3검사는 대기한다.

Root의 **12 세 번째 공유 Release 빌드 종료0** 후 **FortMeta 새3검사 모두 통과**했다. 950행/같은풀 Contents/거부 경계 대조가 성공했으며 source/tool/report/fixture는 현재 그대로 동결했다. 당시 빌드의 PostHost tuple 경고1과 다른 신규 소비자 실패는 별도 교정 중이므로 단위12 전체 경고0/전체검사 성공은 아직 완료로 표시하지 않는다.

Root의 최종 증분 Release는 **경고0·오류0**, **FortMeta 새3검사/950행 모두 통과**다. 단위12 새18검사와 기존24검사가 모두 성공했으며 전체1056검사는 현재 진행 중이다. 앞 문단의 경고/소비자 대기 설명은 세 번째 빌드 당시 상태로 보존한다. 전체 Fort 부모·runtime TypeNames·실제 UI 경계는 계속 후속이며, 생산/Test/tool/report/fixture를 최종 교정 상태로 동결한다.
