using System.Buffers.Binary;

// 위험한 다리 위 이동체 확인과 같은 그래프의 안전한 표면 선택을 제공한다.
namespace Netstorm.Core.Original;

/// <summary>현재 그래프·이동 프로세스·맵 경계의 실제 장치를 빌린다.</summary>
public sealed class BridgeCarrierHooks
{
    /// <summary>현재 그래프 번호를 읽으며 254는 없는 그래프다.</summary>
    public Func<int, uint>? GraphFor { get; set; }
    /// <summary>현재 TYPE의 이동 프로세스 종류를 읽는다.</summary>
    public Func<uint, uint>? MovementType { get; set; }
    /// <summary>현재 부모에 부착된 해당 종류의 Form을 읽는다.</summary>
    public Func<int, uint, int>? MovementForm { get; set; }
    /// <summary>선택한 좌표로 실제 이동을 시작한다.</summary>
    public Func<int, RawPathPoint, bool>? Start { get; set; }
    /// <summary>현재 맵의 엄격한 내부 좌표를 검사한다.</summary>
    public Func<RawPathPoint, bool>? Valid { get; set; }
    /// <summary>원문 줄 번호의 진단을 보고하며 정상 복귀하면 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>진단이 켜진 위치에서만 현재 다섯 이동체 TYPE를 비교한다.</summary>
    public Func<uint, bool>? WontWalkOffEdge { get; set; }
    /// <summary>독립 관측에서만 탐색 사각형을 기록하며 제품에서는 생략한다.</summary>
    public Action<SquidSearchArea>? Scan { get; set; }
}

/// <summary>칸 위 탐색과 대피 후보 탐색의 커서를 분리하고 원문 순서로 현재 상태를 읽는다.</summary>
public sealed class RawBridgeCarriers
{
    /// <summary>현재 위치·종류·수명을 제공하는 PATCH 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>두 독립 탐색기가 함께 읽는 실제 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재 표면·이동체 플래그와 발자국 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>대피 입구에서 읽는 현재 권한이다.</summary>
    private readonly Func<bool> _boss;
    /// <summary>소비 위치마다 읽는 현재 진단 DWORD다.</summary>
    private readonly RawCheckingFlag _checking;
    /// <summary>칸 위 순회가 현재 비교하는 사제 TYPE다.</summary>
    private readonly Func<uint> _priest;
    /// <summary>칸 위 순회가 현재 비교하는 레인 워커 TYPE다.</summary>
    private readonly Func<uint> _rainWalker;
    /// <summary>위험한 비영 다리 수명의 현재 signed 경계다.</summary>
    private readonly Func<int> _avoidLife;
    /// <summary>현재 실제 이동·그래프·진단 장치를 보유한다.</summary>
    private readonly BridgeCarrierHooks _hooks;

    /// <summary>현재 저장소를 빌리며 생성 시 전역·슬롯·탐색 커서를 바꾸지 않는다.</summary>
    public RawBridgeCarriers(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types,
        Func<bool> boss, RawCheckingFlag checking, Func<uint> priest, Func<uint> rainWalker,
        Func<int> avoidLife, BridgeCarrierHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(boss);
        ArgumentNullException.ThrowIfNull(checking); ArgumentNullException.ThrowIfNull(priest);
        ArgumentNullException.ThrowIfNull(rainWalker); ArgumentNullException.ThrowIfNull(avoidLife);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
            throw new ArgumentException("다리 이동체 대피의 PATCH 풀·TYPE 오류");
        _pool = pool; _hash = hash; _types = types; _boss = boss; _checking = checking;
        _priest = priest; _rainWalker = rainWalker; _avoidLife = avoidLife; _hooks = hooks;
    }

    /// <summary>현재 다리 한 칸의 사제·레인 워커 중 이동 Form이 없는 이동체만 대피시킨다.</summary>
    public void Check(int bridge)
    {
        RawPathPoint point = Position(bridge);
        int x = Ftol(point.X), y = Ftol(point.Y);
        var finder = new RawSquidFinder(_pool, _hash, _types);
        var area = new SquidSearchArea(x, y, x, y);
        _hooks.Scan?.Invoke(area);
        // 대피가 중첩되어도 이 바깥 탐색기의 다음 raw 링크를 유지한다.
        for (int sid = finder.Begin(area, 0); sid != 0; sid = finder.Next())
        {
            uint kind = _pool.Slot(sid)[10];
            if (kind != _priest() && kind != _rainWalker()) continue;
            if ((Type(sid).Flags2 & 0x30000u) == 0) Need(_hooks.Assertion)(0x2d8);
            uint movement = Need(_hooks.MovementType)(_pool.Slot(sid)[10]);
            if (Need(_hooks.MovementForm)(sid, movement) == 0) Retreat(sid);
        }
    }

    /// <summary>권한·그래프 확인 뒤 반경5의 안전한 같은 그래프 표면 중 가장 가까운 곳으로 이동한다.</summary>
    public void Retreat(int parent)
    {
        if (!_boss() || Graph(parent) == 0) return;
        uint kind = _pool.Slot(parent)[10];
        if (_checking.Read() != 0 && !Need(_hooks.WontWalkOffEdge)(kind)) Need(_hooks.Assertion)(0x893);
        float best = 100f;
        var chosen = new RawPathPoint(-10000f, -10000f);
        RawPathPoint point = Position(parent);
        int x = Center(point.X), y = Center(point.Y);
        var finder = new RawSquidFinder(_pool, _hash, _types);
        var area = new SquidSearchArea(unchecked(x - 5), unchecked(y - 5), unchecked(x + 5), unchecked(y + 5));
        _hooks.Scan?.Invoke(area);
        // 후보 순서를 보존하며 선택한 최소 거리만 float로 저장하고 동률은 이전 후보를 유지한다.
        for (int sid = finder.Begin(area, 0); sid != 0; sid = finder.Next())
        {
            if ((Type(sid).Flags1 & 0x800u) == 0 || Dangerous(sid)) continue;
            uint mine = Graph(parent), other = Graph(sid);
            if (mine != other) continue;
            RawPathPoint destination = Position(sid), origin = Position(parent);
            double dx = (double)destination.X - origin.X, dy = (double)destination.Y - origin.Y;
            double distance = Math.Sqrt(dy * dy + dx * dx);
            if (distance >= 1.0 && distance < best) { best = (float)distance; chosen = Position(sid); }
        }
        if (Need(_hooks.Valid)(chosen) && !Need(_hooks.Start)(parent, chosen) && _checking.Read() != 0)
            Need(_hooks.Assertion)(0x8b0);
    }

    /// <summary>효과 뒤의 현재 위치 두 float를 다시 읽는다.</summary>
    private RawPathPoint Position(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return new(BitConverter.Int32BitsToSingle(BinaryPrimitives.ReadInt32LittleEndian(raw[14..])),
            BitConverter.Int32BitsToSingle(BinaryPrimitives.ReadInt32LittleEndian(raw[18..])));
    }
    /// <summary>현재 BYTE TYPE 범위를 검사한 뒤 같은 표를 조회한다.</summary>
    private OriginalTypeRecord Type(int sid)
    {
        int type = _pool.Slot(sid)[10];
        return type < _types.Count ? _types[type] : throw new ArgumentOutOfRangeException(nameof(sid), "다리 대피 현재 TYPE 범위");
    }
    /// <summary>NULL 그래프를0으로, 유효한 그래프0부터는 번호+1로 구별한다.</summary>
    private uint Graph(int sid) { uint number = Need(_hooks.GraphFor)(sid); return number == 254 ? 0u : unchecked(number + 1u); }
    /// <summary>다리 속성이 있고 비영 수명이 현재 회피 경계보다 작은 후보를 제외한다.</summary>
    private bool Dangerous(int sid)
    {
        if (sid == 0 || (Type(sid).Flags2 & 4u) == 0) return false;
        int life = (_pool.Slot(sid)[12] & 0x78) >> 3;
        return life != 0 && life < _avoidLife();
    }
    /// <summary>원문 반경 탐색의 거의 올림→정수→float 저장을 재현한다.</summary>
    private static int Center(float value) => Ftol((float)Ftol((double)value + 0.99999f));
    /// <summary>signed64 절삭의 하위 DWORD를 취하고 비유한·범위 밖은 indefinite 하위0으로 만든다.</summary>
    private static int Ftol(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0
        ? 0 : unchecked((int)(uint)(long)value);
    /// <summary>선택한 필수 효과가 없으면 앞선 효과를 보존하며 오류를 전달한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("다리 이동체 대피의 실제 효과 미연결");
}
