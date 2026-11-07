// 일반 전투 참가자의 현재 Territory 한 개를 실제 월드에 배치하는 원문 부모다.
using System;
using System.Collections.Generic;

// 파일 수신/State 전환은 소유하지 않고 같은 섬/지형/객체 부모만 호출하는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Original;

// 전투섬 배치 시 현재 Template/Player/지형/객체/Graph의 실제 동기 장치를 연결하는 훅 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class BattleIslandHooks
{
    // owner 0의 Islandbuilder.cpp:0x10f 진단이며 복귀하면 계속하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<uint>? Assertion { get; set; }

    // 004bd660의 현재 Template Territory 120바이트 읽기 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<uint, byte[]>? Territory { get; set; }

    // 00432050의 현재 Player별 실제 배치 범위를 계산하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<uint, byte[], ChunkRect>? Placement { get; set; }

    // 현재 Player+78에 배치된 섬 번호를 쓰는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<uint, uint>? Region { get; set; }

    // 0046da70의 현재 bounds/owner/섬 지형 생성 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<RawIslandBuilderState, int>? Generate { get; set; }

    // 지형 생성 뒤 현재 00540bc4를 다시 읽는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<bool>? Boss { get; set; }

    // 권한이 있을 때만 현재 Template의 선택 Terr 범위를 읽어 실제 객체를 만드는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<uint, int, int>? Objects { get; set; }

    // 마지막 00463110(0)의 실제 전체 Graph 재구성 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<bool>? Graph { get; set; }

    // 기존 RawIslandBuilder가 선택한 진단 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<RawIslandBuilderAssertion>? BuilderAssertion { get; set; }

    // 같은 실제 Client Canon 진단 수명을 빌리는 진단 설정 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public CanonDecoderDiagnostics? DecoderDiagnostics { get; set; }
}

// PATCH 0046d9b0 → 0046da60 → 0046e2c0의 생성/선택/현재 필드 쓰기 순서를 복원하는 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class RawBattleIsland
{
    // 기존 런타임과 같은 판본/물리 Pool 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawSidPool _pool;

    // builder의 SetWorld가 선택하는 실제 전역 청크 지도 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly ChunkMap _world;

    // 같은 현재 섬 목록의 Add/Bounds를 사용하는 섬 런타임 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawIslandRuntime _islands;

    // 현재 puzzlePiece가 조회하는 실제 타입 프레임 표 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    // 패턴 배치마다 다시 읽는 같은 Client 전역 퍼즐 조각 조회자 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Func<uint> _getPuzzlePiece;

    // 현재 Template/Player/지형/객체/Graph의 실제 동기 장치 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly BattleIslandHooks _hooks;

    // 동일 Pool/월드/타입/장치를 검증하고 연결하는 생성자다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public RawBattleIsland(
        RawSidPool pool,
        ChunkMap world,
        RawIslandRuntime islands,
        IReadOnlyList<RiftTypeFrames> frames,
        Func<uint> getPuzzlePiece,
        BattleIslandHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(getPuzzlePiece);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(islands.Pool(), pool) || pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("전투섬의 실제10.78 Pool 불일치");
        }

        _pool = pool;
        _world = world;
        _islands = islands;
        _frames = frames;
        _getPuzzlePiece = getPuzzlePiece;
        _hooks = hooks;
    }

    // 첫 flag 4 영역 또는 0번을 배치하고 권한별 객체 적재/Graph(false)를 수행하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void Build(uint owner)
    {
        RawIslandBuilderState state = new(new ChunkRect(-1, -1, -1, -1), owner, _hooks.DecoderDiagnostics);
        if (state.Owner == 0u)
        {
            Need(_hooks.Assertion)(0x10fu);
        }

        byte[] records = new byte[120];
        Need(_hooks.Territory)(state.Owner, records);
        int selected = 0;
        // 레코드의 flags BYTE+1에서 0x04 비트가 켜진 첫 영역을 찾고 없으면 원문처럼 0번을 유지하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int index = 0; index < 20; ++index)
        {
            if ((records[(index * 6) + 1] & 4) != 0)
            {
                selected = index;
                break;
            }
        }

        byte[] record = records.AsSpan(selected * 6, 6).ToArray();
        state.Bounds = Need(_hooks.Placement)(state.Owner, record);
        RawIslandBuilder builder = new(_world, _world, _islands, _frames, _getPuzzlePiece, state, _hooks.BuilderAssertion);
        uint island = builder.Place(record, (uint)selected, true);
        Need(_hooks.Region)(state.Owner, island);
        Need(_hooks.Generate)(state, unchecked((int)island));
        if (Need(_hooks.Boss)())
        {
            Need(_hooks.Objects)(state.Owner, selected, selected + 1);
        }

        Need(_hooks.Graph)(false);
    }

    // 선택된 실제 콜백만 필요해진 위치에서 검사해 반환하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static T Need<T>(T? hook)
        where T : class
    {
        if (hook is null)
        {
            throw new InvalidOperationException("전투섬 생성의 선택된 실제 장치 미연결");
        }

        return hook;
    }
}
