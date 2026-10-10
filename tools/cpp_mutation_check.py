#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cpppj 소스를 일부러 한 군데씩 틀리게 바꿔, 기계어 대조 검사가 그 차이를 실제로 잡아내는지 확인한다.

**저장소의 소스는 바꾸지 않는다.** cpppj(빌드 폴더 제외)를 Git 제외 폴더 extracted/mutation-work/cpppj 로 복사하고
별도 빌드 폴더(extracted/mutation-work/build)에서 변이마다 사본의 파일 하나를 바꿔 Release 빌드와 콘솔 검사를 실행한다.
그래서 확인이 도는 동안 커밋해도 변이된 소스가 저장소에 들어가지 않는다. 사본은 실행할 때마다 저장소와 다시 맞춘다.
게임·클론 창·원본 실행 파일은 실행하지 않는다. 첫 실행은 사본 전체를 빌드하므로 몇 분 더 걸린다.

    python -X utf8 tools/cpp_mutation_check.py --list           # 변이 목록과 적용 가능 여부만 확인(빌드 없음)
    python -X utf8 tools/cpp_mutation_check.py                  # 모든 변이 실행
    python -X utf8 tools/cpp_mutation_check.py --only process-zero-keeps-running
    python -X utf8 tools/cpp_mutation_check.py --cmake "C:/.../cmake.exe"

각 변이는 기대하는 실패 검사 이름의 접두사를 갖는다. 그 접두사의 검사가 하나도 실패하지 않으면 "미검출"로 보고한다.
변이를 적용하지 않은 사본의 검사가 먼저 모두 통과해야 한다(아니면 "검출"이 의미가 없으므로 중단한다).
"""
import argparse
import subprocess
import sys
from pathlib import Path

# 저장소 루트, 저장소의 cpppj, Git 제외 작업 사본과 그 빌드/검사 실행 파일 위치.
ROOT = Path(__file__).resolve().parent.parent
REPOSITORY = ROOT / 'cpppj'
WORK = ROOT / 'extracted/mutation-work'
SOURCE = WORK / 'cpppj'
BUILD = WORK / 'build'
TESTS = BUILD / 'bin/Release/netstorm_tests.exe'
# 사본에 복사하지 않는 저장소 cpppj 안의 최상위 폴더(빌드 산출물).
EXCLUDED = {'build'}
# 변이 목록: 이름, 대상 파일, 바꿀 원문(파일에 정확히 한 번 있어야 한다), 바꾼 문장, 실패해야 하는 검사 이름 접두사.
MUTATIONS = [
    dict(name='process-zero-keeps-running', file='cpppj/src/o/SquidProcess.cpp', expect='process_',
         before='if (result>0.0f || (patch && std::isnan(result))) {',
         after='if (result>=0.0f || (patch && std::isnan(result))) {',
         note='처리기 반환 0을 종료가 아니라 재예약으로 취급'),
    dict(name='process-unlink-under-dead-parent', file='cpppj/src/o/SquidProcess.cpp', expect='process_',
         before='    if (parentState&kDead) return;\n',
         after='    // 변이: dead 부모에서도 종속 체인을 고친다.\n',
         note='부모가 지워지는 중에도 form을 체인에서 뺌'),
    dict(name='process-skip-previous-link', file='cpppj/src/o/SquidProcess.cpp', expect='process_',
         before='    if (head) SetPrevious(Sid{head},form.value);\n',
         after='    // 변이: 기존 머리의 이전 항목 연결 생략.\n',
         note='부착 때 기존 머리의 이전 항목을 쓰지 않음'),
    dict(name='process-run-abstract-parent', file='cpppj/src/o/SquidProcess.cpp', expect='process_patch',
         before='    if (patch && (raw[40]&1) && !(types_.at(raw[kType]).flags2&kAbstractRunGenus)) return;\n',
         after='    // 변이: 패치판 abstract 부모 건너뛰기 생략.\n',
         note='패치판에서 abstract 부모의 Regular도 실행'),
    dict(name='process-no-geyser-poll', file='cpppj/src/o/SquidProcess.cpp', expect='process_',
         before='    if (raw[kType]==state_.pollType && process.time_-state_.now>kPollLimit) process.time_=state_.now;\n',
         after='    // 변이: geyser의 먼 예약 당김 생략.\n',
         note='geyser에 붙은 Regular의 0.5초 당김 생략'),
    dict(name='reward-percent-rounding', file='cpppj/src/o/SquidReward.cpp', expect='reward_',
         before='static_cast<std::uint32_t>(state_.percent)*static_cast<std::uint32_t>(cost)))/100);',
         after='static_cast<std::uint32_t>(state_.percent)*static_cast<std::uint32_t>(cost)))/100+1);',
         note='삭제 보상 지급액을 1 크게 계산'),
    dict(name='reward-skip-noise', file='cpppj/src/o/SpStore.cpp', expect='reward_patch',
         before='        for (int n=0;n<kNoisePerBit;++n) word|=(std::uint32_t{1}<<rng_.Next(kBitLimit))&~mask;\n',
         after='        // 변이: SP 저장소의 난수 잡음 생략.\n',
         note='패치판 SP 저장소가 난수를 소비하지 않음'),
    dict(name='bridge-end-hard-flag-of-new-frame', file='cpppj/src/o/RawBridgeEvents.cpp', expect='bridge_event_',
         before='if (codes[static_cast<std::size_t>(current)].flags & kHardFrameFlag) return;',
         after='if (codes[static_cast<std::size_t>(first)].flags & kHardFrameFlag) return;',
         note='단단한 프레임 검사를 되돌린 옛 프레임이 아니라 새 끝 프레임의 플래그로 함'),
    dict(name='bridge-end-nan-direction', file='cpppj/src/o/RawBridgeEvents.cpp', expect='bridge_event_patch',
         before='letter = (objectY < y) ? kEndN : kEndL;',
         after='letter = (objectY >= y) ? kEndL : kEndN;',
         note='J 칸 방향 판정에서 NaN을 L이 아니라 N으로 취급'),
    dict(name='bridge-end-life-bits', file='cpppj/src/o/RawBridgeEvents.cpp', expect='bridge_event_',
         before='(kBridgeCrackLife * 8 - 8) & kBridgeLifeMask',
         after='(kBridgeCrackLife * 8) & kBridgeLifeMask',
         note='끝 칸의 수명 비트를 금 간 수명 - 1이 아니라 금 간 수명으로 씀'),
    dict(name='bridge-end-skip-flag-copy', file='cpppj/src/o/RawBridgeEvents.cpp', expect='bridge_event_',
         before='(pool_.Slot(bridge)[flagOffset] & kCopiedFlagBit)',
         after='0',
         note='새 객체로 플래그 비트 0x10을 옮기지 않음'),
    dict(name='bridge-event-no-authority-keeps', file='cpppj/src/o/RawBridgeEvents.cpp', expect='bridge_event_',
         before='if (event == kBridgeFallEvent) {\n        if (!state_.authority) return kBridgeEventEnd;',
         after='if (event == kBridgeFallEvent) {\n        if (!state_.authority) return kBridgeEventKeep;',
         note='권한이 없을 때 지연 낙하 이벤트가 종료(0)가 아니라 유지(-1)를 돌려줌'),
    dict(name='letter-run-last-frame', file='cpppj/src/o/RiftType.cpp', expect='bridge_event_',
         before='if (run.first == -1) run.first = static_cast<int>(i);',
         after='run.first = static_cast<int>(i);',
         note='글자별 첫 프레임이 아니라 마지막 프레임을 기록'),
    dict(name='bridge-dispatch-ignores-vtable', file='cpppj/src/o/RawBridgeEvents.cpp', expect='bridge_event_',
         before='if (vtable == bridgeVtable) return events.Handle(parent, event, count, payload);',
         after='if (vtable != 0) return events.Handle(parent, event, count, payload);',
         note='다리가 아닌 객체의 이벤트도 다리 처리기로 보냄'),
    dict(name='owner-add-void-object', file='cpppj/src/o/SquidOwner.cpp', expect='owner_',
         before='if (listed && !(raw[kState] & kVoid) && !(raw[extraOffset] & kBuried) && player)',
         after='if (listed && !(raw[extraOffset] & kBuried) && player)',
         note='void 객체도 새 소유자의 작업장 목록에 넣음'),
    dict(name='owner-remove-first-only', file='cpppj/src/o/SquidOwner.cpp', expect='owner_',
         before='if (value == sid.value) ++removed;',
         after='if (value == sid.value && removed == 0) ++removed;',
         note='이전 소유자 목록에서 중복 가운데 첫 항목만 뺌'),
    dict(name='owner-ignore-mode', file='cpppj/src/o/SquidOwner.cpp', expect='owner_',
         before='const bool listed = (mode_.fort || mode_.battle) && (types_[type].flags2 & kFactoryMask) != 0;',
         after='const bool listed = (types_[type].flags2 & kFactoryMask) != 0;',
         note='요새/전투 모드가 아니어도 작업장 목록을 고침'),
    dict(name='owner-append-when-full', file='cpppj/src/o/SquidOwner.cpp', expect='owner_',
         before='if (list.count < list.entries.size()) list.entries[list.count++] = sid.value;',
         after='if (list.count < list.entries.size()) list.entries[list.count++] = sid.value; else if (!list.entries.empty()) list.entries.back() = sid.value;',
         note='가득 찬 목록의 마지막 항목을 덮어씀'),
    dict(name='neighbor-candidate-spot-no-bias', file='cpppj/src/o/RawSquidNeighbors.cpp', expect='bridge_connect_',
         before='return static_cast<int>(static_cast<double>(value)+static_cast<double>(kSpotBias)); }',
         after='return static_cast<int>(static_cast<double>(value)); }',
         note='후보 기준점의 spot 칸을 0.9999 보정 없이 고름'),
    dict(name='neighbor-flag4-keeps-abstract', file='cpppj/src/o/RawSquidNeighbors.cpp', expect='bridge_connect_',
         before='if ((flags&NeighborFlag::kSkipAbstract)!=0 && (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kAbstract)!=0) return false;',
         after='// 변이: flag 4의 abstract 후보 제외 생략.',
         note='flag 4에서도 abstract 후보를 이웃으로 받음'),
    dict(name='neighbor-flag2-needs-surface', file='cpppj/src/o/RawSquidNeighbors.cpp', expect='bridge_connect_',
         before='if ((types_.at(number).flags1&TypeFlag1::kSurface)==0 && (flags&NeighborFlag::kAnyType)==0) return false;',
         after='if ((types_.at(number).flags1&TypeFlag1::kSurface)==0) return false;',
         note='flag 2에서도 비표면 후보를 제외'),
    dict(name='neighbor-flag1-still-joins', file='cpppj/src/o/RawSquidNeighbors.cpp', expect='bridge_connect_',
         before='if ((flags&NeighborFlag::kNoJoint)==0) {',
         after='if ((flags&NeighborFlag::kSupported)!=99) {',
         note='flag 1에서도 프레임 접합 검사를 함'),
    dict(name='connect-mask-swapped', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='const std::uint32_t mask = bridge ? kIslandSurfaceMask : TypeFlag2::kBridge;',
         after='const std::uint32_t mask = bridge ? TypeFlag2::kBridge : kIslandSurfaceMask;',
         note='다리가 다리와, 섬 표면이 섬 표면과 연결을 만듦'),
    dict(name='connect-owner-of-second', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='Link(first, second, pool_.Slot(first)[ownerOffset], preview);',
         after='Link(first, second, pool_.Slot(second)[ownerOffset], preview);',
         note='연결 객체의 소유자를 다리가 아니라 상대 표면에서 읽음'),
    dict(name='link-frame-not-halved', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='SetFrameField(born, direction / 2);',
         after='SetFrameField(born, direction);',
         note='연결 객체의 프레임을 방향/2가 아니라 방향으로 씀'),
    dict(name='link-center-not-rounded', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='const float targetX = static_cast<float>(RoundUp(center[0])), targetY = static_cast<float>(RoundUp(center[1]));',
         after='const float targetX = center[0], targetY = center[1];',
         note='상대 중심을 거의 올림하지 않고 방향을 정함'),
    dict(name='link-references-swapped', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='Write(pool_.AllocatedBytes(born), kSecondReference, second.value, 2);',
         after='Write(pool_.AllocatedBytes(born), kSecondReference, first.value, 2);',
         note='연결 객체의 둘째 참조에 상대가 아니라 다리 번호를 씀'),
    dict(name='link-preview-skips-screen', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='    Write(raw, kScreenY, static_cast<std::uint32_t>(Ftol(static_cast<double>(y) * kScreenScaleY + kScreenBias)), 2);\n',
         after='    // 변이: 미리보기의 화면 y 좌표 생략.\n',
         note='미리보기 연결 객체의 화면 y 좌표를 쓰지 않음'),
    dict(name='owner-propagates-to-owned-island', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='if (pool_.Slot(base)[ownerOffset] != 0 || (pool_.Slot(base)[extraOffset] & kAbstractOrBuried) != 0) return;',
         after='if ((pool_.Slot(base)[extraOffset] & kAbstractOrBuried) != 0) return;',
         note='이미 주인이 있는 섬 받침의 소유자도 바꿈'),
    dict(name='owner-ignores-abstract-bridge', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='if ((pool_.Slot(first)[extraOffset] & kAbstractOrBuried) != 0 || base.value == 0) return;',
         after='if (base.value == 0) return;',
         note='abstract/buried 다리도 소유자를 전파'),
    dict(name='owner-footprint-not-rounded', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='             RoundUp(foot.right), RoundUp(foot.bottom)});',
         after='             Ftol(static_cast<double>(foot.right)), Ftol(static_cast<double>(foot.bottom))});',
         note='섬 받침 발자국의 오른쪽/아래를 올림하지 않고 자름'),
    dict(name='owner-any-type-in-footprint', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='if (static_cast<std::uint32_t>(pool_.Slot(current)[kType]) == state_.noIslandType) hooks_.setOwner(current, owner);',
         after='hooks_.setOwner(current, owner);',
         note='발자국 안의 모든 객체에 소유자를 줌'),
    dict(name='findat-never-skips-level0', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='(types[type].flags2 & kSurfaceLevelGenus) == 0 ? 1U : 0U);',
         after='0U);',
         note='한 칸 조회가 항상 해시 0단계부터 찾음'),
    dict(name='color-not-decremented', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='std::bit_cast<std::uint32_t>(state.ownerColors[static_cast<std::size_t>(owner)]) - 1U);',
         after='std::bit_cast<std::uint32_t>(state.ownerColors[static_cast<std::size_t>(owner)]));',
         note='소유자 색 프레임에서 1을 빼지 않음'),
    dict(name='island-owner-ignores-mission', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='    if (state.mission && ReadFrame(pool, sid) == kNeutralOwnerFrame) return;\n',
         after='    // 변이: 미션 중 중립 섬의 프레임 유지 생략.\n',
         note='미션 중에도 중립 프레임의 섬을 색칠'),
    dict(name='island-post-ignores-first-pop-bit', file='cpppj/src/o/RawBridgeConnect.cpp', expect='bridge_connect_',
         before='    if ((flags & 1U) == 0) return;\n',
         after='    // 변이: 최초 등록 비트 검사 생략.\n',
         note='최초 등록이 아니어도 종유석을 만들고 연결 순회를 함'),
    dict(name='direction-tie-horizontal', file='cpppj/src/o/RawSquidNeighbors.cpp', expect='bridge_connect_',
         before='if (!(std::abs(static_cast<double>(dy))<std::abs(dx))) return dy>0 ? 4 : 0;',
         after='if (std::abs(static_cast<double>(dy))>std::abs(dx)) return dy>0 ? 4 : 0;',
         note='가로·세로 차이가 같을 때 세로가 아니라 가로 방향을 고름'),
    dict(name='frame-single-update', file='cpppj/src/o/SquidFrame.cpp', expect='set_frame_',
         before='        write();\n        hooks_.update(sid, flags);\n        return;',
         after='        write();\n        return;',
         note='같은 단계 경로에서 새 프레임의 표시 갱신을 생략'),
    dict(name='frame-write-before-first-update', file='cpppj/src/o/SquidFrame.cpp', expect='set_frame_',
         before='        hooks_.update(sid, flags);\n        write();\n        hooks_.update(sid, flags);',
         after='        write();\n        hooks_.update(sid, flags);\n        hooks_.update(sid, flags);',
         note='옛 프레임의 표시 갱신 전에 프레임을 먼저 씀'),
    dict(name='frame-same-frame-still-repops', file='cpppj/src/o/SquidFrame.cpp', expect='set_frame_',
         before='    if (frame == current) return;\n',
         after='    // 변이: 같은 프레임이어도 다시 등록한다.\n',
         note='단계가 다를 때 프레임이 그대로여도 Unpop/Pop을 함'),
    dict(name='frame-repop-plain-flags', file='cpppj/src/o/SquidFrame.cpp', expect='set_frame_',
         before='flags | kFrameRepopFlags);',
         after='flags);',
         note='다시 등록할 때 flags에 0x50을 더하지 않음'),
    dict(name='frame-bridge-level-by-size', file='cpppj/src/o/SquidFrame.cpp', expect='set_frame_',
         before='if ((genus & (TypeFlag2::kIsland | TypeFlag2::kBridge)) == 0) {',
         after='if ((genus & TypeFlag2::kIsland) == 0) {',
         note='다리의 해시 단계를 0이 아니라 프레임 크기로 정함'),
    dict(name='frame-level-of-stored-not-written', file='cpppj/src/o/SquidFrame.cpp', expect='set_frame_',
         before="    pool_.AllocatedBytes(sid)[pool_.Edition() == OriginalEdition::Patch1078 ? 0x21 : 0x1f] = static_cast<std::uint8_t>(level);\n",
         after='    // 변이: 단계 바이트를 쓰지 않는다.\n',
         note='현재 프레임의 단계를 단계 바이트에 쓰지 않음'),
    dict(name='frame-write-after-repop', file='cpppj/src/o/SquidFrame.cpp', expect='set_frame_',
         before='    hooks_.unpop(sid, flags);\n    write();\n',
         after='    write();\n    hooks_.unpop(sid, flags);\n',
         note='Unpop 전에 프레임을 먼저 씀'),
    dict(name='island-pre-ignores-keep-flag', file='cpppj/src/o/RawIslandLifecycle.cpp', expect='island_lifecycle_',
         before='const bool keepBridges = (flags & kIslandDestroyKeepsBridges) != 0;',
         after='const bool keepBridges = false;',
         note='삭제 flags 0x1000이 있어도 둘레 다리에 지연 낙하를 예약'),
    dict(name='island-pre-schedules-dead-bridge', file='cpppj/src/o/RawIslandLifecycle.cpp', expect='island_lifecycle_',
         before="if (!keepBridges && (Genus(other) & TypeFlag2::kBridge) != 0 && (pool_.Slot(other)[kState] & kDead) == 0)",
         after="if (!keepBridges && (Genus(other) & TypeFlag2::kBridge) != 0)",
         note='죽은 다리에도 지연 낙하를 예약(연결 필터가 dead를 먼저 거르므로 검출되지 않을 수 있다 — 결과를 문서에 적을 것)'),
    dict(name='island-pre-schedules-any-surface', file='cpppj/src/o/RawIslandLifecycle.cpp', expect='island_lifecycle_',
         before="if (!keepBridges && (Genus(other) & TypeFlag2::kBridge) != 0 && (pool_.Slot(other)[kState] & kDead) == 0)",
         after="if (!keepBridges && (pool_.Slot(other)[kState] & kDead) == 0)",
         note='다리가 아닌 표면 이웃에도 지연 낙하를 예약'),
    dict(name='surface-post-includes-level0', file='cpppj/src/o/RawIslandLifecycle.cpp', expect='island_lifecycle_',
         before='constexpr std::uint32_t kSkipSurfaceLevel = 1;',
         after='constexpr std::uint32_t kSkipSurfaceLevel = 0;',
         note='표면 칸 삭제 때 해시 0단계의 walker도 떨어뜨림'),
    dict(name='surface-post-ignores-buried', file='cpppj/src/o/RawIslandLifecycle.cpp', expect='island_lifecycle_',
         before="void RawIslandLifecycle::SurfacePostDestroy(Sid surface, std::uint32_t flags) const {\n    const std::size_t extraOffset = pool_.Edition() == OriginalEdition::Patch1078 ? 0x28 : 0x23;\n    if ((pool_.Slot(surface)[extraOffset] & kAbstractOrBuried) == 0) {",
         after="void RawIslandLifecycle::SurfacePostDestroy(Sid surface, std::uint32_t flags) const {\n    const std::size_t extraOffset = pool_.Edition() == OriginalEdition::Patch1078 ? 0x28 : 0x23;\n    if ((pool_.Slot(surface)[extraOffset] & 0) == 0) {",
         note='abstract/buried 표면 칸도 walker 낙하를 부름'),
    # 2026-10-10 소리 프로세스/이름 표(client/SoundProcess.cpp, client/Sound.cpp). 대조 검사 이름은 SoundProcess_ 로 시작한다.
    dict(name='sound-nan-gate-patch', file='cpppj/src/client/SoundProcess.cpp', expect='SoundProcess_ReplaysOriginals',
         before='    if (patch ? !(now>=startTime_) : startTime_>now) return;\n',
         after='    if (startTime_>now) return;\n',
         note='패치판도 CD판처럼 NaN 시각에서 실행'),
    dict(name='sound-loop-ignores-type-frame', file='cpppj/src/client/SoundProcess.cpp', expect='SoundProcess_',
         before='            if (current_!=0 || !played) current_=hooks.playLoopAt(x,y,current_,sound);\n',
         after='            current_=hooks.playLoopAt(x,y,current_,sound);\n',
         note='같은 프레임에 이미 소리를 낸 타입도 반복 재생을 시작'),
    dict(name='sound-mark-when-silent', file='cpppj/src/client/SoundProcess.cpp', expect='SoundProcess_',
         before='            if (current_!=0) MarkTypeFrame();\n',
         after='            MarkTypeFrame();\n',
         note='반복 재생이 시작되지 않아도 타입의 마지막 소리 프레임을 적음'),
    dict(name='sound-once-kept-when-played', file='cpppj/src/client/SoundProcess.cpp', expect='SoundProcess_',
         before='    if (playOnce) host->Kill(*this,0);\n',
         after='    if (playOnce && !played) host->Kill(*this,0);\n',
         note='재생하지 못한 한 번 재생 프로세스를 끝내지 않음'),
    dict(name='sound-global-loop-flag', file='cpppj/src/client/SoundProcess.cpp', expect='SoundProcess_',
         before='        current_=hooks.play(sound,playOnce ? 0U : 1U,0,0,priority,0);\n',
         after='        current_=hooks.play(sound,playOnce ? 1U : 0U,0,0,priority,0);\n',
         note='전역 재생의 반복 인자를 뒤집음'),
    dict(name='sound-destroy-stops-once', file='cpppj/src/client/SoundProcess.cpp', expect='SoundProcess_',
         before='    if (current_==0 || (flags_&kSoundPlayOnce)) return;\n',
         after='    if (current_==0) return;\n',
         note='한 번 재생 프로세스도 삭제 때 소리를 멈춤'),
    dict(name='sound-alternate-word-low-byte', file='cpppj/src/client/SoundProcess.cpp', expect='SoundProcess_',
         before='    if ((flags_&kSoundUseAlternate) && (raw[kWord]|raw[kWord+1])) sound=alternate_;\n',
         after='    if ((flags_&kSoundUseAlternate) && raw[kWord]) sound=alternate_;\n',
         note='부모 상태 단어의 하위 바이트만 보고 대체 소리를 고름'),
    dict(name='sound-list-limit-inclusive', file='cpppj/src/client/Sound.cpp', expect='SoundProcess_',
         before='    if (at>=kSoundListLimit) return fallback_;\n',
         after='    if (at>kSoundListLimit) return fallback_;\n',
         note='빈 위치가 정확히 한도일 때도 새 항목을 추가'),
    dict(name='sound-fold-one-past-z', file='cpppj/src/client/Sound.cpp', expect='SoundProcess_',
         before='    return static_cast<std::uint8_t>(byte-0x41U)<0x1aU ? static_cast<std::uint8_t>(byte+0x20U) : byte;\n',
         after='    return static_cast<std::uint8_t>(byte-0x41U)<0x1bU ? static_cast<std::uint8_t>(byte+0x20U) : byte;\n',
         note="대소문자 접기를 'Z' 다음 글자('[')까지 넓힘"),
    dict(name='sound-robust-report-on-patch', file='cpppj/src/client/Sound.cpp', expect='SoundProcess_ReplaysOriginals',
         before="        if (edition_==o::OriginalEdition::Cd1072 && existing.back()!='v') Report(kCdRobustExpression,kCdRobustLine);\n",
         after="        if (existing.back()!='v') Report(kCdRobustExpression,kCdRobustLine);\n",
         note='CD판 전용 항목 이름 검사 보고를 패치판에서도 함'),
    # 2026-10-10 소리 재생 계층(client/Sound.cpp의 SoundPlayer와 화면 위치 계산). 대조 검사 이름은 SoundPlay_ 로 시작한다.
    dict(name='soundplay-max-ignores-priority', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    if (priority==0 && state_.maxPlaying!=0 && state_.playing>=state_.maxPlaying) return 0;\n',
         after='    if (state_.maxPlaying!=0 && state_.playing>=state_.maxPlaying) return 0;\n',
         note='우선 재생도 동시 재생 한도에 걸림'),
    dict(name='soundplay-max-strict', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    if (priority==0 && state_.maxPlaying!=0 && state_.playing>=state_.maxPlaying) return 0;\n',
         after='    if (priority==0 && state_.maxPlaying!=0 && state_.playing>state_.maxPlaying) return 0;\n',
         note='재생 수가 한도와 같을 때 재생을 허용'),
    dict(name='soundplay-limit-strict', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='        if (limit!=0 && count>=limit) return 0;\n',
         after='        if (limit!=0 && count>limit) return 0;\n',
         note='같은 소리가 한도만큼 재생 중일 때 하나 더 허용'),
    dict(name='soundplay-gain-no-upper-clamp', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    return sum<kSoundMinimum ? kSoundMinimum : sum>0 ? 0 : sum;\n',
         after='    return sum<kSoundMinimum ? kSoundMinimum : sum;\n',
         note='음량을 0 위로 자르지 않음'),
    dict(name='soundplay-duplicate-copies-attenuation', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    Put(at+4,root);Put(at+8,0);Put(at,buffer);\n',
         after='    Put(at+4,root);Put(at+8,0);Put(at,buffer);Put(at+0xc,Field(previous,SoundField::Attenuation));\n',
         note='복제 항목에 소리별 감쇠를 물려줌(원본은 0으로 둔다)'),
    dict(name='soundplay-refill-from-root', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='                if (hooks_.duplicate(Get(sound,SoundField::Buffer),copy)<0) { failed(kRefillLine);Set(walk,SoundField::Buffer,0);return walk; }\n',
         after='                if (hooks_.duplicate(Get(Get(sound,SoundField::Root),SoundField::Buffer),copy)<0) { failed(kRefillLine);Set(walk,SoundField::Buffer,0);return walk; }\n',
         note='쉬는 항목을 요청 항목이 아니라 원본 항목의 버퍼로 채움'),
    dict(name='soundplay-loop-keeps-other-name', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='        else { StopBuffer(current);current=0; }\n',
         after='        else current=0;\n',
         note='다른 이름으로 바꿀 때 지금 소리를 멈추지 않음'),
    dict(name='soundplay-loop-offscreen-no-stop', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='        if (BufferPlaying(current)) { StopBuffer(current);return 0; }\n',
         after='        if (BufferPlaying(current)) return 0;\n',
         note='화면 밖으로 나간 반복 소리를 멈추지 않음'),
    dict(name='soundplay-update-logs-negative-only', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    if (hooks_.setVolume(buffer,Gain(current,volume))!=0)\n',
         after='    if (hooks_.setVolume(buffer,Gain(current,volume))<0)\n',
         note='위치 음량 설정의 양수 반환을 실패로 기록하지 않음'),
    dict(name='soundplay-pan-ignores-swap', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    if (swap) distance=0U-distance;\n',
         after='    static_cast<void>(swap);\n',
         note='좌우 바꿈 옵션을 무시'),
    dict(name='soundplay-volume-divides-by-height', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    return Limit(Divide(scaled,static_cast<std::int32_t>(static_cast<std::uint32_t>(halfWidth)*static_cast<std::uint32_t>(halfWidth))));\n',
         after='    return Limit(Divide(scaled,static_cast<std::int32_t>(static_cast<std::uint32_t>(halfHeight)*static_cast<std::uint32_t>(halfHeight))));\n',
         note='위치 음량을 세로 절반의 제곱으로 나눔'),
    dict(name='soundplay-onscreen-margin', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='constexpr std::uint32_t kMarginX=40,kMarginY=30;\n',
         after='constexpr std::uint32_t kMarginX=41,kMarginY=30;\n',
         note='화면 안 판정의 좌우 여유를 1픽셀 넓힘'),
    dict(name='soundplay-stop-skips-rewind', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    hooks_.stop(buffer);\n    hooks_.setPosition(buffer,0);\n',
         after='    hooks_.stop(buffer);\n',
         note='정지 뒤 처음 위치로 되돌리지 않음'),
    dict(name='soundplay-byname-unique-check-always', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='    if (loop!=0) {\n        const auto entry=list_.Lookup(name);\n',
         after='    {\n        const auto entry=list_.Lookup(name);\n',
         note='반복이 아닌 이름 재생에도 유일성 보고를 함'),
    dict(name='soundplay-screen-no-half-pixel', file='cpppj/src/client/Sound.cpp', expect='SoundPlay_',
         before='constexpr double kCellWidth=16.0,kCellHeight=11.0,kHalfPixel=0.5;\n',
         after='constexpr double kCellWidth=16.0,kCellHeight=11.0,kHalfPixel=0.0;\n',
         note='월드→화면 변환에서 0.5를 더하지 않음'),
    # 2026-10-10 장면별 음악 감독(client/SoundSceneMusic.cpp). 대조 검사 이름은 SceneMusic_ 로 시작한다.
    dict(name='scene-lock-nan-is-false', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='bool BelowOrUnordered(double left,double right) { return !(left>=right); }\n',
         after='bool BelowOrUnordered(double left,double right) { return left<right; }\n',
         note='비교 불가(NaN) 시각을 "작음"으로 보지 않음'),
    dict(name='scene-fanfare-ignores-result-state', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    if (SameName(name,kFanfareMusic) && state_.resultState!=1) return;\n',
         after='    // 변이: fanfare의 결과 상태 검사 생략.\n',
         note='결과 상태가 1이 아니어도 승리 곡 요청을 받아들임'),
    dict(name='scene-lock-blocks-anticipation', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    if (ResultMusicLocked() && !SameName(name,kAnticipationMusic)) return;\n',
         after='    if (ResultMusicLocked()) return;\n',
         note='결과 곡 잠금이 대기실 곡 요청도 막음'),
    dict(name='scene-long-song-inclusive', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    if (kLongSongSeconds<hooks_.duration()) {\n',
         after='    if (kLongSongSeconds<=hooks_.duration()) {\n',
         note='정확히 30초인 곡의 길이를 믿음'),
    dict(name='scene-short-song-uses-long-seconds', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    state_.songEnd=hooks_.wallSeconds()+kShortSongSeconds;\n',
         after='    state_.songEnd=hooks_.wallSeconds()+kLongSongSeconds;\n',
         note='짧은 곡의 다시 확인 시간을 30초로 둠'),
    dict(name='scene-defeat-end-skipped', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='        state_.defeatEnd=length+hooks_.wallSeconds();\n',
         after='        state_.defeatEnd=length;\n',
         note='패배 곡 끝 시각에 현재 시각을 더하지 않음'),
    dict(name='scene-next-mod-three', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    state_.index=next%4;\n',
         after='    state_.index=next%3;\n',
         note='다음 곡 색인을 3으로 나눈 나머지로 정함'),
    dict(name='scene-sacrifice-also-weather', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    if (state_.playersReady!=0 && hooks_.sacrificing(state_.localPlayer)) { Request(kSacrificeMusic);return; }\n',
         after='    if (state_.playersReady!=0 && hooks_.sacrificing(state_.localPlayer)) { Request(kSacrificeMusic);ApplyWeather();return; }\n',
         note='희생 곡을 고른 뒤에도 날씨 효과를 적용'),
    dict(name='scene-next-ignores-players-ready', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    if (state_.playersReady!=0 && hooks_.sacrificing(state_.localPlayer))',
         after='    if (hooks_.sacrificing(state_.localPlayer))',
         note='플레이어 표가 준비되지 않아도 희생 여부를 물음'),
    dict(name='scene-frame-battle-equals-one', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    if (state_.battle!=0) { Next();return; }\n',
         after='    if (state_.battle==1) { Next();return; }\n',
         note='전투 여부를 DWORD가 1일 때만 참으로 봄'),
    dict(name='scene-start-skips-frame', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    Frame();\n    state_.tint=At(state_.tints,state_.index);\n',
         after='    state_.tint=At(state_.tints,state_.index);\n',
         note='시작 뒤 Frame을 한 번 부르지 않음'),
    dict(name='scene-thunder-inverted', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    if (state_.index==kThunderMusicIndex) hooks_.thunderFlash();\n',
         after='    if (state_.index!=kThunderMusicIndex) hooks_.thunderFlash();\n',
         note='번개 효과를 천둥 곡이 아닐 때 시작(처음의 ">=" 변이는 색인 3의 효과음 칸이 비어 동작이 같은 동등 변이여서 이것으로 바꿨다)'),
    dict(name='scene-dirty-increments', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='        state_.paletteDirty=1;\n',
         after='        ++state_.paletteDirty;\n',
         note='팔레트 갱신 표시를 1로 두지 않고 증가'),
    dict(name='scene-weather-skips-refresh', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before='    hooks_.refresh();\n    if (hooks_.ascendancyPalette()) {\n',
         after='    if (hooks_.ascendancyPalette()) {\n',
         note='날씨 효과의 화면 갱신 요청을 생략'),
    dict(name='scene-name-compare-case-sensitive', file='cpppj/src/client/SoundSceneMusic.cpp', expect='SceneMusic_',
         before="        const auto fold=[](char value) { return value>='A' && value<='Z' ? static_cast<char>(value-'A'+'a') : value; };\n",
         after='        const auto fold=[](char value) { return value; };\n',
         note='곡 이름 비교가 대소문자를 구분'),
    dict(name='audio-volume-step1', file='cpppj/src/client/ClientAudio.cpp', expect='ClientAudio_',
         before='    case 0: return -4000;\n',
         after='    case 0: return -2000;\n',
         note='음량 단계 1의 값을 바꿈'),
    dict(name='audio-volume-step4', file='cpppj/src/client/ClientAudio.cpp', expect='ClientAudio_',
         before='    case 3: return -500;\n',
         after='    case 3: return -1000;\n',
         note='음량 단계 4의 값을 바꿈'),
    dict(name='audio-volume-zero-is-step1', file='cpppj/src/client/ClientAudio.cpp', expect='ClientAudio_',
         before='    const auto index=static_cast<std::uint32_t>(step)-1U;\n',
         after='    const auto index=static_cast<std::uint32_t>(step<=0 ? 1 : step)-1U;\n',
         note='단계 0·음수를 1로 취급(원본은 부호 없는 비교로 최대 음량)'),
    dict(name='audio-volume-high-not-max', file='cpppj/src/client/ClientAudio.cpp', expect='ClientAudio_',
         before='    default: return 0;\n',
         after='    default: return -500;\n',
         note='범위 밖(5 이상) 단계의 음량을 최대로 두지 않음'),
    dict(name='audio-scene-battle-always', file='cpppj/src/client/ClientAudio.cpp', expect='ClientAudio_',
         before='    scene_.battle=battle ? 1U : 0U;\n',
         after='    scene_.battle=1U;\n',
         note='메뉴 장면에서도 전투 여부를 켬'),
    dict(name='audio-scene-frame-skipped', file='cpppj/src/client/ClientAudio.cpp', expect='ClientAudio_',
         before='    sceneMusic_.Frame();\n',
         after='    static_cast<void>(sceneMusic_);\n',
         note='프레임마다 곡 끝 확인을 하지 않음'),
    dict(name='audio-request-ignored', file='cpppj/src/client/ClientAudio.cpp', expect='ClientAudio_',
         before='    sceneMusic_.Request(name);\n',
         after='    static_cast<void>(name);\n',
         note='곡 직접 요청을 무시'),
    dict(name='preload-always-loads', file='cpppj/src/client/Sound.cpp', expect='ClientAudio_',
         before='    const auto entry=list_.Lookup(name);\n    if (Get(entry,SoundField::Buffer)==0) {\n        const auto loaded=hooks_.load(name);\n',
         after='    const auto entry=list_.Lookup(name);\n    {\n        const auto loaded=hooks_.load(name);\n',
         note='이미 적재된 항목도 다시 적재'),
    dict(name='preload-attenuation-without-buffer', file='cpppj/src/client/Sound.cpp', expect='ClientAudio_',
         before='        if (loaded.buffer!=0) Set(entry,SoundField::Attenuation,static_cast<std::uint32_t>(loaded.attenuation));\n    }\n    return entry;\n',
         after='        Set(entry,SoundField::Attenuation,static_cast<std::uint32_t>(loaded.attenuation));\n    }\n    return entry;\n',
         note='버퍼가 없는데도 감쇠를 적음'),
    dict(name='preload-requires-ready', file='cpppj/src/client/Sound.cpp', expect='ClientAudio_',
         before='    const auto entry=list_.Lookup(name);\n    if (Get(entry,SoundField::Buffer)==0) {\n',
         after='    const auto entry=list_.Lookup(name);\n    if (state_.initialized && Get(entry,SoundField::Buffer)==0) {\n',
         note='준비되지 않았으면 적재하지 않음(원본은 검사하지 않는다)'),
]


def run(command):
    """명령을 실행하고 표준 출력/오류를 합쳐 돌려준다. 실패 코드는 호출자가 판단한다."""
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, encoding='utf-8', errors='replace')
    return result.returncode, result.stdout + result.stderr


def sync():
    """저장소의 cpppj(빌드 폴더 제외)를 작업 사본과 바이트 단위로 맞춘다. 달라진 파일만 써서 증분 빌드를 유지한다."""
    wanted, copied, removed = set(), 0, 0
    # 저장소 쪽 파일을 모두 훑어 사본에 없거나 다른 것만 복사한다.
    for path in REPOSITORY.rglob('*'):
        relative = path.relative_to(REPOSITORY)
        if relative.parts[0] in EXCLUDED or path.is_dir():
            continue
        wanted.add(relative)
        target = SOURCE / relative
        data = path.read_bytes()
        if not target.exists() or target.read_bytes() != data:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            copied += 1
    # 저장소에서 없어진 파일은 사본에서도 지운다.
    for path in list(SOURCE.rglob('*')) if SOURCE.exists() else []:
        if path.is_file() and path.relative_to(SOURCE) not in wanted:
            path.unlink()
            removed += 1
    return copied, removed


def configure(cmake):
    """사본의 빌드 폴더를 처음 한 번 구성한다."""
    if (BUILD / 'CMakeCache.txt').exists():
        return 0, ''
    return run([cmake, '-S', str(SOURCE), '-B', str(BUILD)])


def build(cmake):
    """사본을 Release로 빌드한다. 빌드 오류는 변이가 컴파일되지 않았다는 뜻이므로 그대로 보고한다."""
    return run([cmake, '--build', str(BUILD), '--config', 'Release'])


def test():
    """사본의 검사 실행 파일을 돌려 (실패한 검사 이름 목록, 실패 CHECK 수)를 돌려준다."""
    _, output = run([str(TESTS)])
    failed = [line.split('] ', 1)[1].strip() for line in output.splitlines() if line.startswith('[FAIL]')]
    checks = sum('CHECK failed' in line for line in output.splitlines())
    return failed, checks


def adapt(text, mutation):
    """파일이 CRLF로 체크아웃된 경우(core.autocrlf=true) 변이 정의의 줄바꿈을 파일에 맞춰 돌려준다."""
    before, after = mutation['before'], mutation['after']
    if '\r\n' in text:
        # 이미 CR이 있는 문장은 건드리지 않도록 LF만 CRLF로 바꾼다.
        before = before.replace('\r\n', '\n').replace('\n', '\r\n')
        after = after.replace('\r\n', '\n').replace('\n', '\r\n')
    return before, after


def check(mutation, base=ROOT):
    """대상 파일에 원문이 정확히 한 번 있는지 확인하고 (경로, 원래 바이트, 횟수)를 돌려준다. base가 WORK면 사본을 본다."""
    path = base / mutation['file']
    original = path.read_bytes()
    text = original.decode('utf-8')
    count = text.count(adapt(text, mutation)[0])
    return path, original, count


def main():
    """사본을 저장소와 맞추고, 변이를 사본에 차례로 적용/빌드/검사한 뒤 사본을 원래 바이트로 되돌린다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--list', action='store_true', help='변이 목록과 적용 가능 여부만 출력한다')
    parser.add_argument('--only', action='append', help='이 이름의 변이만 실행한다(여러 번 줄 수 있다)')
    parser.add_argument('--cmake', default='cmake', help='cmake 실행 파일 경로')
    options = parser.parse_args()
    selected = [m for m in MUTATIONS if not options.only or m['name'] in options.only]
    if options.list:
        # 빌드 없이 저장소 소스에서 원문 위치만 확인한다.
        for mutation in selected:
            _, _, count = check(mutation)
            print(f"{mutation['name']}\t{mutation['file']}\t원문 {count}곳\t{mutation['note']}")
        return 0 if all(check(m)[2] == 1 for m in selected) else 1
    copied, removed = sync()
    print(f'작업 사본 맞춤: 복사 {copied}개, 삭제 {removed}개 ({SOURCE})', flush=True)
    code, output = configure(options.cmake)
    if code != 0:
        print('사본 빌드 구성 실패:\n' + output[-2000:])
        return 2
    code, output = build(options.cmake)
    if code != 0:
        print('변이 전 사본 빌드 실패:\n' + output[-2000:])
        return 2
    failed, checks = test()
    if failed or checks:
        print(f"변이 전 사본의 검사가 실패한다({', '.join(failed)}; CHECK {checks}개). 먼저 저장소 소스를 고쳐야 한다.")
        return 2
    print('변이 전 사본: 빌드·검사 통과', flush=True)
    missed = 0
    # 변이는 한 번에 하나만 사본에 적용한다. 원래 바이트는 메모리에 들고 있다가 finally에서 되돌린다.
    for mutation in selected:
        path, original, count = check(mutation, WORK)
        if count != 1:
            print(f"{mutation['name']}: 원문이 {count}곳이라 건너뜀(소스가 바뀌었으면 변이 정의를 고쳐야 함)")
            missed += 1
            continue
        try:
            text = original.decode('utf-8')
            before, after = adapt(text, mutation)
            path.write_bytes(text.replace(before, after).encode('utf-8'))
            code, output = build(options.cmake)
            if code != 0:
                print(f"{mutation['name']}: 빌드 실패(변이가 컴파일되지 않음)")
                missed += 1
                continue
            failed, checks = test()
            hit = [name for name in failed if name.startswith(mutation['expect'])]
            status = '검출' if hit else '미검출'
            if not hit: missed += 1
            print(f"{mutation['name']}: {status} — 실패 검사 {len(failed)}개({', '.join(failed) or '없음'}), 실패 CHECK {checks}개", flush=True)
        finally:
            path.write_bytes(original)
    print(f'변이 {len(selected)}개 중 미검출/건너뜀 {missed}개 (저장소 소스는 바꾸지 않았다)')
    return 1 if missed else 0


if __name__ == '__main__':
    sys.exit(main())
