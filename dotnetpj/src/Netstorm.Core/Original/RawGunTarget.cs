using System.Buffers.Binary;

// 실제 후보 Finder·현재 raw 필터와 판본별 사거리/관측각을 복원한다.
namespace Netstorm.Core.Original;

/// <summary>RawGunTarget.cpp/.h의 같은 현재 Pool/Hash/TYPE/SHP를 빌리는 검색기다.</summary>
public sealed partial class RawGunTarget
{
    /// <summary>현재 슬롯과 raw 후보를 빌리는 실제 SID 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>네 단계 실제 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재 번호별 HP/genus/발자국/group/핫스폿이다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 번호별 사거리/테마/방향/확장 높이다.</summary>
    private readonly IReadOnlyList<GunTargetType> _targetTypes;
    /// <summary>현재 SHP의 실제 포구 헤더다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;
    /// <summary>소비 위치에서 읽는 현재 Player/전역이다.</summary>
    private readonly GunTargetState _state;
    /// <summary>함수 값만 복사한 실제 진단 출력 효과다.</summary>
    private readonly GunTargetHooks _hooks;
    /// <summary>발자국 가장자리 거리 점수에서 지상 후보에 더하는 값이다.</summary>
    private const float GroundPenalty = 1000f;
    /// <summary>같은 판본의 전체 자료 표를 복사 없이 빌린다.</summary>
    public RawGunTarget(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<GunTargetType> targetTypes, IReadOnlyList<SquidDisplayShape> shapes, GunTargetState state, GunTargetHooks? hooks = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(targetTypes); ArgumentNullException.ThrowIfNull(shapes); ArgumentNullException.ThrowIfNull(state);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || targetTypes.Count != types.Count || shapes.Count != types.Count)
            throw new ArgumentException("Gun 표적 TYPE/SHP 판본 불일치");
        _pool = pool; _hash = hash; _types = types; _targetTypes = targetTypes; _shapes = shapes; _state = state; _hooks = (hooks ?? new()).Copy();
    }
    /// <summary>같은 실제 GunProcess 풀인지 검사하는 비소유 접근자다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>정상 타입/소유자/유한 문맥의 원문 호출 계약만 확인한다.</summary>
    private void Validate(GunTargetQuery query)
    {
        if (query.Type >= _types.Count || query.Owner > 8 || !float.IsFinite(query.X) || !float.IsFinite(query.Y) || !float.IsFinite(query.CenterX) || !float.IsFinite(query.CenterY))
            throw new ArgumentOutOfRangeException(nameof(query), "Gun 표적 문맥 범위 오류");
    }
    /// <summary>발자국까지의 최소 거리와 Wind 관측각의 판본별 중간 저장을 유지한다.</summary>
    private bool ArcherShot(GunTargetQuery query, int target)
    {
        GunTargetRect rect = Footprint(target);
        double dx = query.CenterX < rect.Left ? (double)rect.Left - query.CenterX : rect.Right < query.CenterX ? (double)query.CenterX - rect.Right : 0;
        double dy = query.CenterY < rect.Top ? (double)rect.Top - query.CenterY : rect.Bottom < query.CenterY ? (double)query.CenterY - rect.Bottom : 0;
        if (_pool.Edition == OriginalEdition.Cd1072) { dx = (float)dx; dy = (float)dy; }
        double distance = Math.Sqrt(dx * dx + dy * dy);
        if (_pool.Edition == OriginalEdition.Patch1078) distance = (float)distance;
        if (!(distance < _targetTypes[(int)query.Type].Range)) return false;
        if (query.Type != _state.WindArcherType) return true;
        var center = Center(target); float angle = 0;
        if (query.CenterX != center.X || query.CenterY != center.Y)
        {
            double radians = Math.Atan2(-((double)center.Y - query.CenterY), (double)center.X - query.CenterX);
            float raw = (float)(450.0 - radians * 57.295827908797776);
            angle = Normalize((float)RawBoardAngle.Normalize(raw, _pool.Edition, _state.Angles));
        }
        float basis = Normalize((float)((double)query.Direction * 45.0));
        return ContainsAngle(angle, Normalize((float)((double)basis - _state.WindHalfAngle)), Normalize((float)((double)basis + _state.WindHalfAngle)));
    }
    /// <summary>현재 표적 중심의 네 방향 사정선과 확장 발자국을 교차한다.</summary>
    private bool CannonShot(GunTargetQuery query, int target)
    {
        var center = Center(target); GunTargetRect ray = CannonRay(query, Direction(query.CenterX, query.CenterY, center.X, center.Y));
        return Intersects(Footprint(target), ray);
    }
    /// <summary>group0 거리/각도, group1 사정선, group3 직접 허용과 PATCH 진단을 유지한다.</summary>
    public bool MayShoot(GunTargetQuery query, int target)
    {
        Validate(query); OriginalTypeRecord type = _types[(int)query.Type];
        if (type.Group == 0) return ArcherShot(query, target);
        if (type.Group == 1) return CannonShot(query, target);
        if (type.Group != 3 && _pool.Edition == OriginalEdition.Patch1078 && _state.Checking && (_types[(int)query.Type].Flags2 & 0x100000) == 0)
            throw new InvalidOperationException("Gun MayShoot flyer 원본 assert");
        return true;
    }
    /// <summary>원문의 HP/서버대역/동맹/extra/사제수량/동일위치 순서로 필터한다. dead/free 검사를 추가하지 않는다.</summary>
    public bool MayTarget(GunTargetQuery query, int target)
    {
        Validate(query);
        if (target < 0 || target >= _pool.Capacity || (target == 0 && _pool.Edition != OriginalEdition.Patch1078)) throw new ArgumentOutOfRangeException(nameof(target));
        ReadOnlySpan<byte> raw = _pool.Slot(target); int number = raw[10];
        if (number >= _types.Count) throw new ArgumentOutOfRangeException(nameof(target));
        OriginalTypeRecord type = _types[number]; bool patch = _pool.Edition == OriginalEdition.Patch1078; byte extra = raw[patch ? 40 : 35];
        if ((type.Flags2 & 0x400000) != 0 ? (extra & 1) == 0 : (type.Flags1 & 0x10) == 0) return false;
        int reservedLast = patch ? 14999 : 5999;
        if (target >= 5 && target <= reservedLast) return false;
        byte owner = raw[patch ? 34 : 32];
        if (owner > 8) throw new ArgumentOutOfRangeException(nameof(target), "Gun 후보 소유자 오류");
        if (!_state.FriendlyFire && (_state.AllyMask(query.Owner) & (1u << owner)) != 0) return false;
        if ((extra & 0x11) != 0) return false;
        if ((_types[number].Flags2 & 0x200000) != 0)
        {
            int quantity = patch ? BinaryPrimitives.ReadInt32LittleEndian(raw[26..]) : BinaryPrimitives.ReadInt16LittleEndian(raw[26..]);
            if (quantity <= 1) return false;
        }
        if (Coordinate(raw, 14) == query.X && Coordinate(raw, 18) == query.Y) return false;
        return MayShoot(query, target);
    }
    /// <summary>반더 타워의 flyer/balloon 선행 필터 뒤 실제 MayTarget/ArcherShot을 실행한다.</summary>
    private bool ArcherTarget(GunTargetQuery query, int target)
    {
        if (query.Type == _state.ThunderArcherType && (_types[_pool.Slot(target)[10]].Flags2 & 0x120000) == 0) return false;
        return MayTarget(query, target) && ArcherShot(query, target);
    }
    /// <summary>현재 높이 표를 Finder가 읽는 값 배열로 준비한다.</summary>
    private int[] Heights()
    {
        int[] heights = new int[_targetTypes.Count];
        // 원문처럼 현재 타입 번호순으로 높이를 복사한다.
        for (int i = 0; i < heights.Length; i++) heights[i] = _targetTypes[i].HeightExtension;
        return heights;
    }
    /// <summary>flags5/현재범위의 실제 Finder와 지상 가산 점수로 첫 최소 후보를 보존한다.</summary>
    private ushort FindArcher(GunTargetQuery query)
    {
        int range = Truncate(_targetTypes[(int)query.Type].Range), x = Ceiling(query.CenterX), y = Ceiling(query.CenterY);
        var finder = new RawSquidFinder(_pool, _hash, _types, Heights()); float best = 1.0e13f; ushort result = 0;
        int first = finder.Begin(new(x - range, y - range, x + range, y + range), 5);
        if (!_state.FriendlyFire && (_state.AllyMask(query.Owner) & (1u << (int)query.Owner)) == 0) throw new InvalidOperationException("Gun 자기 동맹 원본 assert");
        // next는 현재 후보 반환 전에 저장되며 후보의 TYPE/좌표는 소비 시 다시 읽는다.
        for (int sid = first; sid != 0; sid = finder.Next())
        {
            if ((_types[_pool.Slot(sid)[10]].Flags1 & 0x10) == 0 || !ArcherTarget(query, sid)) continue;
            var point = HitPoint(query, Footprint(sid)); double dx = (double)query.CenterX - point.X, dy = (double)query.CenterY - point.Y;
            float distance = (float)(dx * dx + dy * dy); double score = (double)distance + ((_types[_pool.Slot(sid)[10]].Flags2 & 0x100000) != 0 ? 0 : GroundPenalty);
            if (score < best) { best = (float)score; result = (ushort)sid; }
        }
        return result;
    }
    /// <summary>0/2/4/6 사정선마다 실제 Finder를 다시 시작해 첫 최소 점수를 선택한다.</summary>
    private ushort FindCannon(GunTargetQuery query)
    {
        float range = _targetTypes[(int)query.Type].Range, best = (float)((double)range * range * 2.0); ushort result = 0; int[] heights = Heights();
        // 원문의 네 짝수 방향 순서를 유지한다.
        for (int direction = 0; direction <= 6; direction += 2)
        {
            GunTargetRect ray = CannonRay(query, direction); var finder = new RawSquidFinder(_pool, _hash, _types, heights);
            // 최소는 절삭, 최대는 bias ceil이며 같은 점수는 먼저 온 후보를 남긴다.
            for (int sid = finder.Begin(new(Truncate(ray.Left), Truncate(ray.Top), Ceiling(ray.Right), Ceiling(ray.Bottom)), 5); sid != 0; sid = finder.Next())
            {
                if (!MayTarget(query, sid)) continue;
                var point = HitPoint(query, Footprint(sid)); double dx = (double)query.CenterX - point.X, dy = (double)query.CenterY - point.Y;
                double score = _pool.Edition == OriginalEdition.Patch1078 ? (float)(dx * dx + dy * dy) : dx * dx + dy * dy;
                if (score < best) { best = (float)score; result = (ushort)sid; }
            }
        }
        return result;
    }
    /// <summary>현재 발자국 중심/공격 모드 프레임을 조립해 실제 아처 또는 캐논 Finder로 검색한다.</summary>
    public ushort FindTarget(float x, float y, uint type, int direction, uint owner)
    {
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(type));
        var query = new GunTargetQuery { X = x, Y = y, CenterX = (float)((double)x - _types[(int)type].FootX * .5), CenterY = (float)((double)y - _types[(int)type].FootY * .5), Type = type, Direction = direction, Owner = owner, Frame = DirectionFrame(type, direction) };
        Validate(query); int group = _types[(int)type].Group;
        if (group != 0 && group != 1) throw new InvalidOperationException("Gun FindTarget group 원본 assert");
        ushort result = group == 0 ? FindArcher(query) : FindCannon(query);
        if (result != 0 && _state.ShowRanges) (_hooks.DrawRange ?? throw new InvalidOperationException("Gun 최종 표적 진단 출력 경계 누락"))(Footprint(result), true);
        return result;
    }
    /// <summary>썬 캐논의 현재 프레임 방향을 먼저 검사한 뒤 실제 MayTarget으로 판정한다.</summary>
    public bool MayFire(GunTargetQuery query, int target)
    {
        Validate(query);
        if (query.Type == _state.SunCannonType)
        {
            var center = Center(target); int direction = Direction(query.CenterX, query.CenterY, center.X, center.Y), frame = query.Frame;
            if (frame < 0 || frame > 27) throw new ArgumentOutOfRangeException(nameof(query));
            bool allowed = frame < 24 ? frame == SunAllowedFrame[direction / 2] : direction == (frame == 24 ? 4 : frame == 25 ? 0 : frame == 26 ? 6 : 2);
            if (!allowed) return false;
        }
        return MayTarget(query, target);
    }
    /// <summary>같은 Pool의 실제 FindTarget/MayFire만 교체하며 나머지 GunProcess 효과는 값 복사로 보존한다.</summary>
    public static GunProcessHooks MakeGunTargetHooks(RawSidPool pool, RawGunTarget target, GunProcessHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, target.Pool)) throw new ArgumentException("GunTarget/GunProcess 풀 불일치", nameof(pool));
        GunProcessHooks result = hooks.Copy();
        result.FindTarget = (x, y, type, mode, owner) => target.FindTarget(x, y, type, unchecked((int)mode), owner);
        result.Shoot = (words, sid) => target.MayFire(GunTargetQuery.FromWords(words), sid);
        return result;
    }
}
