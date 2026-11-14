// 사제의 현재 genus 계약과 후보별 지형 효과를 일반 충돌 몸체/실제 finder에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>사제와 일반 배치가 같은 Client/마스크·미리보기 상태를 공유하는 충돌 처리기다.</summary>
public sealed class RawPriestPlacementCollision
{
    /// <summary>현재 raw와 같은 판본을 확인하는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>모양마다 실제 finder를 구성하는 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>후보마다 현재 사제 genus를 확인하는 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원문처럼 별도로 복사한 사제 지역/지형 함수다.</summary>
    private readonly PriestPlacementCollisionHooks _hooks;
    /// <summary>같은 현재 전역/미리보기를 빌리는 일반 한 후보 계산기다.</summary>
    private readonly RawCanonPlacementCollision _collision;
    /// <summary>필수 사제 함수를 가리는 빈 람다 없이 먼저 확인하고 원래 여섯 인자로 변환한다.</summary>
    private static CanonPlacementCollisionHooks AdaptHooks(PriestPlacementCollisionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.InspectRegions is null || hooks.InspectCandidateTerrain is null) throw new ArgumentException("사제 충돌 지역 경계 누락");
        Func<PriestPlacementQuery, Func<SquidSearchArea, bool>, bool> regions = hooks.InspectRegions; Action<PriestPlacementQuery, int> terrain = hooks.InspectCandidateTerrain;
        return new() { InspectRegions = (q, scan) => regions(new(q.Type, q.X, q.Y, q.Flags, q.Owner, q.Mode), scan), InspectCandidateTerrain = (q, sid) => terrain(new(q.Type, q.X, q.Y, q.Flags, q.Owner, q.Mode), sid) };
    }
    /// <summary>사제 함수 사본과 일반 충돌을 같은 실제 자료/미리보기로 조립한다.</summary>
    public RawPriestPlacementCollision(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, CanonPlacementCollisionState state, CanonPlacementPreviewState preview, PriestPlacementCollisionHooks hooks)
    {
        _collision = new(pool, hash, types, state, preview, AdaptHooks(hooks)); _pool = pool; _hash = hash; _types = types; _hooks = hooks.Copy();
    }
    /// <summary>각 사제 진입에서 현재 genus·유한 좌표를 확인하며 일반 타입 성공으로 오해하지 않게 한다.</summary>
    private void Placing(PriestPlacementQuery query)
    {
        if (query.Type >= _types.Count || (_types[(int)query.Type].Flags2 & 0x200000) == 0 || !float.IsFinite(query.X) || !float.IsFinite(query.Y)) throw new ArgumentException("사제 충돌 요청 타입/좌표 오류");
    }
    /// <summary>현재 사제 계약을 확인한 뒤 같은 원문 무시/표시/extra 순서의 한 후보를 처리한다.</summary>
    public bool InspectCandidate(PriestPlacementQuery query, bool localOwner, int candidate)
    {
        Placing(query); return _collision.InspectCandidate(new(query.Type, query.Type, query.X, query.Y, query.Flags, query.Owner, query.Mode), localOwner, candidate);
    }
    /// <summary>사제 genus를 매 후보 다시 확인하고 무시 후보도 지형 효과 뒤 실제 Next로 진행한다.</summary>
    public bool InspectArea(PriestPlacementQuery query, bool localOwner, SquidSearchArea area)
    {
        Placing(query); var finder = new RawSquidFinder(_pool, _hash, _types);
        // 첫 거부 뒤 현재 후보의 지형과 Next를 실행하지 않는다.
        for (int sid = finder.Begin(area); sid != 0; sid = finder.Next()) { if (!InspectCandidate(query, localOwner, sid)) return false; _hooks.InspectCandidateTerrain!(query, sid); }
        return true;
    }
    /// <summary>요청과 로컬 비교를 저장하며 지역 정책의 거부 덮어쓰기를 막는다.</summary>
    public bool Inspect(PriestPlacementQuery query, bool localOwner)
    {
        Placing(query); bool rejected = false; bool allowed = _hooks.InspectRegions!(query, area => { if (rejected) return false; rejected = !InspectArea(query, localOwner, area); return !rejected; }); return allowed && !rejected;
    }
    /// <summary>미리보기와 같은 실제 풀을 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>사제 미리보기의 후보 정책만 실제 충돌로 바꾸고 기존 범위 함수 사본은 보존한다.</summary>
    public static PriestPlacementPreviewHooks MakePriestPlacementCollisionHooks(RawSidPool pool, RawPriestPlacementCollision collision, PriestPlacementPreviewHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(collision); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, collision.Pool)) throw new ArgumentException("사제 충돌/미리보기의 SID 풀이 다릅니다");
        PriestPlacementPreviewHooks result = hooks.Copy(); result.InspectCollisions = collision.Inspect; return result;
    }
}
