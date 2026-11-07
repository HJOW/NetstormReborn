// 원문 0046e140의 일반 중앙섬 생성·중심 기록·거주 구역·군도 분리 부모다.
using System;
using System.Collections.Generic;

// Template 특수 배치와 metadata/State는 외곽 부모가 선택하는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Original;

// 중앙섬 생성 시 읽고 쓰는 현재 전역 상태를 묶는 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class CentralIslandGlobals
{
    // 현재 전투 모드 불리언 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public bool Battle;

    // 현재 boss 권한 불리언 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public bool Boss;

    // 원문 00540a2c의 현재 terrainEnabled DWORD 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint TerrainEnabled;

    // 원문 005c8bb0의 현재 archipelago DWORD 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint Archipelago;

    // 원문 00568afc/00568b00의 같은 장기 중심 X/Y 2원소 배열 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public float[] Position { get; init; } = new float[2];

    // 원문 00540a30의 현재 invalidRegion 127이며 owner 0과 구별하는 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public int UnassignedTerritory = 127;
}

// 중앙섬 생성 시 연결되는 상위 동기 장치를 묶는 훅 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class CentralIslandHooks
{
    // 원문 ea/eb/ec/ed 진단 뒤 복귀하면 현재 상태로 계속하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<string, uint>? Assertion { get; set; }

    // 00431ff0의 원점 6/6과 현재 패턴 크기를 계산하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<byte[], ChunkRect>? Placement { get; set; }

    // 같은 실제 0046da70의 지형/간헐천/섬 후처리를 수행하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<RawIslandBuilderState, int>? Generate { get; set; }

    // 권한 및 terrainEnabled 0일 때만 실제 0046df80을 실행하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<int>? Residence { get; set; }

    // 현재 archipelago 비영일 때 실제 0046f7d0의 지역 분리를 실행하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action? Partition { get; set; }

    // 마지막 00463110(0)의 실제 Graph 전체 재구성 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<bool>? Graph { get; set; }

    // 기존 Place의 같은 진단 장치 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<RawIslandBuilderAssertion>? BuilderAssertion { get; set; }

    // Client의 현재 Canon 진단 수명을 빌리는 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public CanonDecoderDiagnostics? DecoderDiagnostics { get; set; }
}

// 실제 builder/목록/프레임과 현재 전역의 수명을 새 사본 없이 조립하는 중앙섬 생성기 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class RawCentralIsland
{
    // 동일 Pool 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawSidPool _pool;

    // 전역 청크 지도 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly ChunkMap _world;

    // 원본 100개 섬 목록 런타임 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawIslandRuntime _islands;

    // 현재 타입 프레임 표 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    // 실제 패턴 TYPE 전역을 읽는 조회자 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Func<uint> _getPuzzlePiece;

    // 현재 전역 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly CentralIslandGlobals _globals;

    // 선택한 실제 부모 장치 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly CentralIslandHooks _hooks;

    // 모든 자료가 Build의 동기 호출보다 오래 살아 있도록 연결하는 생성자다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public RawCentralIsland(
        RawSidPool pool,
        ChunkMap world,
        RawIslandRuntime islands,
        IReadOnlyList<RiftTypeFrames> frames,
        Func<uint> getPuzzlePiece,
        CentralIslandGlobals globals,
        CentralIslandHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(getPuzzlePiece);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(islands.Pool(), pool))
        {
            throw new ArgumentException("중앙섬의 실제10.78 Pool 불일치");
        }

        _pool = pool;
        _world = world;
        _islands = islands;
        _frames = frames;
        _getPuzzlePiece = getPuzzlePiece;
        _globals = globals;
        _hooks = hooks;
    }

    // 값으로 전달된 실제 Zone 6바이트로 일반 중앙섬을 만들고 앞선 부분 효과를 보존하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void Build(ReadOnlySpan<byte> record)
    {
        if (record.Length < 6)
        {
            throw new ArgumentException("Zone 레코드는 최소 6바이트여야 합니다.", nameof(record));
        }

        byte[] recordCopy = record[..6].ToArray();
        RawIslandBuilderState state = new(new ChunkRect(-1, -1, -1, -1), 0u, _hooks.DecoderDiagnostics);
        if (!_globals.Battle)
        {
            CentralNeed(_hooks.Assertion)("inBattleMode", 0xeau);
        }

        if (recordCopy[2] != 0)
        {
            CentralNeed(_hooks.Assertion)("terr.xChunkCoord == 0 && terr.yChunkCoord == 0", 0xebu);
        }

        if (_islands.Count() != 0u)
        {
            CentralNeed(_hooks.Assertion)("islandList.getTotal() == 0", 0xecu);
        }

        if (state.Owner != 0u)
        {
            CentralNeed(_hooks.Assertion)("playerId == INVALID_PLAYER_ID", 0xedu);
        }

        state.Bounds = CentralNeed(_hooks.Placement)(recordCopy);
        _globals.Position[0] = (float)(((double)CentralCell(state.Bounds.Right) + CentralCell(state.Bounds.Left)) * 0.5);
        _globals.Position[1] = (float)(((double)CentralCell(state.Bounds.Bottom) + CentralCell(state.Bounds.Top)) * 0.5);
        RawIslandBuilder builder = new(_world, _world, _islands, _frames, _getPuzzlePiece, state, _hooks.BuilderAssertion);
        uint island = builder.Place(recordCopy, unchecked((uint)_globals.UnassignedTerritory), true);
        CentralNeed(_hooks.Generate)(state, unchecked((int)island));
        if (_globals.Boss && _globals.TerrainEnabled == 0u)
        {
            CentralNeed(_hooks.Residence)(unchecked((int)island));
        }

        if (_globals.Archipelago != 0u)
        {
            CentralNeed(_hooks.Partition)();
        }

        CentralNeed(_hooks.Graph)(false);
    }

    // 선택된 하위만 해당 원문 위치에서 요구하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static T CentralNeed<T>(T? hook)
        where T : class
    {
        if (hook is null)
        {
            throw new InvalidOperationException("중앙섬 생성의 선택된 실제 장치 미연결");
        }

        return hook;
    }

    // 0040d900의 DWORD 감김 16배 뒤 float32 저장을 수행하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static float CentralCell(int chunk)
    {
        return (float)unchecked((int)((uint)chunk << 4));
    }
}
