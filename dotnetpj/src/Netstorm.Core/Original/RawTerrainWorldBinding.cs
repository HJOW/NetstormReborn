// 초기 도장 본체가 쓰는 실제 월드 생성·소유자·Pop·지역 효과를 묶는다.
namespace Netstorm.Core.Original;

// 초기 지형 월드 바인딩 모듈을 담는 네임스페이스다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.

// 첫 생성 이전에 월드 조회·진단을 확인하고 같은 Core의 실제 후크만 둔다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.
public static class RawTerrainWorldBinding
{
    /// <summary>같은 월드의 실제 생성·소유자·Pop·지역 효과를 도장 훅에 연결한다.</summary>
    /// <param name="world">생성·Pop을 수행하는 같은 실제 월드다.</param>
    /// <param name="state">현재 특수 타입·원본 입력을 읽는 도장 상태다.</param>
    /// <param name="regionDiagnostic">지역 setter의 판본별 진단이다. 없으면 거부한다.</param>
    /// <param name="effects">지역 조회·진단을 담은 호출자 효과다. 생성·소유자·Pop·지역을 덮는다.</param>
    /// <returns>같은 월드에 연결된 도장 훅이다.</returns>
    public static TerrainStampHooks MakeTerrainStampWorldHooks(
        RawSurfaceWorld world,
        TerrainStampState state,
        Action<SquidRegionAssert>? regionDiagnostic,
        TerrainStampHooks effects)
    {
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(effects);
        if (effects.ChunkRegion is null || effects.Assertion is null || regionDiagnostic is null)
        {
            throw new ArgumentException("초기 지형 월드 지역 조회/진단 연결 누락");
        }

        RawWorldCore core = world.Core();
        effects.Create = (type, flags) => world.Create(type, flags);
        // 본관·거주 구역·간헐천 목록 규칙의 실제 vtable owner 처리를 같은 mutable 영역에 분배한다.
        effects.SetOwner = RawBridgeConnect.MakeOwnerDispatch(core.Pool, core.BridgeState, (sid, player) => core.Owner.Set(sid, player));
        effects.Pop = (sid, x, y, flags) => world.Pop(sid, x, y, flags);
        effects.SetRegion = (sid, region) => RawSquidRegion.SetSquidRegion(core.Pool, core.Types, sid, unchecked((uint)region), unchecked((uint)state.NoRegion), regionDiagnostic);
        return effects;
    }
}
