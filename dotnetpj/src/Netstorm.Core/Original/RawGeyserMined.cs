using System.Buffers.Binary;
using System.Globalization;

// 같은 간헐천 수량/공간/게임 난수로 채집 통지와 고갈 생성을 처리한다.
namespace Netstorm.Core.Original;

/// <summary>RawGeyserMined.h의 현재 권한/진단/타입 전역이며 독립 입력과 실제 owner를 구별한다.</summary>
public sealed class GeyserMinedState
{
    /// <summary>독립 입력의 고갈 삭제 권한이다.</summary>
    public bool Boss { get; set; } = true;
    /// <summary>실제 Client의 현재 권한이며 음향 뒤 소비 위치에서 읽는다.</summary>
    public Func<bool>? SharedBoss { get; set; }
    /// <summary>실제 현재 checking DWORD 뷰다.</summary>
    public RawCheckingFlag Checking { get; } = new();
    /// <summary>독립 입력의 현재 너겟 단가 TYPE다.</summary>
    public uint NuggetType { get; set; } = 154;
    /// <summary>실제 Client의 장기 너겟 TYPE owner다.</summary>
    public UIntBox? SharedNuggetType { get; set; }
    /// <summary>고갈 뒤 지면 밖에 생성할 현재 TYPE다.</summary>
    public uint EmptyGeyserType { get; set; } = 84;
    /// <summary>생성을 생략하는 실제 표면 TYPE다.</summary>
    public uint IsleType { get; set; } = 97;
    /// <summary>PATCH 수량 setter의 진단 대상 TYPE다.</summary>
    public uint BridgeType { get; set; } = 82;
    /// <summary>음향과 setter 뒤 같은 권한을 읽는다.</summary>
    public bool CurrentBoss() => SharedBoss?.Invoke() ?? Boss;
    /// <summary>각 단가 조회 위치에서 같은 장기 번호를 읽는다.</summary>
    public uint CurrentNuggetType() => SharedNuggetType?.Value ?? NuggetType;
}

/// <summary>같은 실제 월드가 공급해야 하는 음향/전체 가상삭제/생성/Pop 함수 값이다.</summary>
public sealed record GeyserMinedHooks
{
    /// <summary>현재 위치에서 실제 geyserMined1..6.wav를 flags0으로 재생한다.</summary>
    public Action<float, float, string, uint>? Sound { get; init; }
    /// <summary>고갈한 간헐천의 실제 가상10(flags1000h) 분배다.</summary>
    public Action<int, uint>? Destroy { get; init; }
    /// <summary>같은 공장의 실제 emptyGeyser 생성이다.</summary>
    public Func<uint, uint, int>? Create { get; init; }
    /// <summary>삭제 전 보관한 위치에 같은 생성물을 실제 Pop한다.</summary>
    public Action<int, float, float, uint>? Pop { get; init; }
}

/// <summary>cpppj/src/o/RawGeyserMined.cpp의 PATCH4266c0/CD4e41e0 전체 본체다.</summary>
public sealed class RawGeyserMined
{
    /// <summary>원문 채집 음향 난수 구간의 폭이다.</summary>
    private const uint SoundChoices = 6000;
    /// <summary>원문 채집 음향 난수 구간의 최솟값이다.</summary>
    private const uint SoundMinimum = 1000;
    /// <summary>간헐천 고갈의 실제 가상삭제 플래그다.</summary>
    private const uint DestroyFlags = 0x1000;
    /// <summary>원문 ftol의 유효 signed64 배타적 상한이다.</summary>
    private const double Int64Boundary = 9223372036854775808.0;
    /// <summary>간헐천/생성물의 같은 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>삭제 뒤 현재 표면 머리를 읽는 같은 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>음향 뒤 현재 단가를 읽는 실제 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>살아 있는 채집마다 한 번만 전진하는 게임 난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>콜백 뒤 다시 읽는 같은 현재 전역이다.</summary>
    private readonly GeyserMinedState _state;
    /// <summary>원문 함수 객체처럼 값 복사한 실제 효과다.</summary>
    private readonly GeyserMinedHooks _hooks;
    /// <summary>판본별 TYPE 수와 필수 효과를 첫 수량/난수 변경 전에 확인한다.</summary>
    public RawGeyserMined(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, GameRandom random, GeyserMinedState state, GeyserMinedHooks hooks)
    {
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.Sound is null || hooks.Destroy is null || hooks.Create is null || hooks.Pop is null) throw new ArgumentException("간헐천 채집 타입/효과 연결 오류");
        Pool = pool; _hash = hash; _types = types; _random = random; _state = state; _hooks = hooks with { };
    }
    /// <summary>두 판본의+12 unsigned WORD만 읽는다.</summary>
    private ushort Quantity(int sid) => BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(sid)[12..]);
    /// <summary>정확한 double 차이를 signed64로 절삭하고 하위 WORD를 보존한다.</summary>
    private static ushort ReducedQuantity(ushort quantity, float cost)
    {
        double value = (double)quantity - cost; return !double.IsFinite(value) || value >= Int64Boundary || value < -Int64Boundary ? (ushort)0 : unchecked((ushort)(long)value);
    }
    /// <summary>원문 ftol의 EAX만 읽는 좌표 caller와 불확정0을 보존한다.</summary>
    private static int CoordinateInteger(float coordinate)
    {
        double value = coordinate; return !double.IsFinite(value) || value >= Int64Boundary || value < -Int64Boundary ? 0 : unchecked((int)(long)value);
    }
    /// <summary>WORD 저장 뒤 PATCH의 현재 다리TYPE/checking/buried 진단만 선택한다.</summary>
    private void SetQuantity(int sid, ushort quantity)
    {
        Span<byte> raw = Pool.AllocatedBytes(sid); BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], quantity);
        if (Pool.Edition == OriginalEdition.Patch1078 && raw[10] == _state.BridgeType && (raw[40] & 8) == 0 && _state.Checking && ((raw[12] >> 3) & 15) > 7) throw new InvalidOperationException("Bridge 수량 num 원본 assert");
    }
    /// <summary>편향 없는 signed64 절삭의 낮은 DWORD로 현재 hash0의 TYPE만 비교한다.</summary>
    private bool OnIsle(float x, float y)
    {
        int ix = CoordinateInteger(x), iy = CoordinateInteger(y); if (ix < 0 || iy < 0 || ix >= 256 || iy >= 256) return false;
        int surface = _hash.Cell(0, ix, iy); return surface != 0 && Pool.Slot(surface)[10] == _state.IsleType;
    }
    /// <summary>free 반환→음향→현재 단가/수량→setter→현재권한→삭제→현재 표면→생성/Pop 순서다.</summary>
    public void Handle(int geyser)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(geyser); if ((raw[11] & 1) != 0) return;
        uint choice = (_random.Next(SoundChoices) + SoundMinimum) / 1000; string sound = "geyserMined" + choice.ToString(CultureInfo.InvariantCulture) + ".wav";
        _hooks.Sound!(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]), sound, 0);
        if (_state.CurrentNuggetType() >= _types.Count) throw new ArgumentOutOfRangeException(nameof(geyser), "간헐천 채집 현재 nugget 타입 범위");
        SetQuantity(geyser, ReducedQuantity(Quantity(geyser), _types[(int)_state.CurrentNuggetType()].Cost));
        if (!_state.CurrentBoss() || Quantity(geyser) != 0) return;
        ReadOnlySpan<byte> current = Pool.Slot(geyser); float x = BinaryPrimitives.ReadSingleLittleEndian(current[14..]), y = BinaryPrimitives.ReadSingleLittleEndian(current[18..]);
        _hooks.Destroy!(geyser, DestroyFlags);
        if (!OnIsle(x, y)) { int next = _hooks.Create!(_state.EmptyGeyserType, 0); _hooks.Pop!(next, x, y, 0); }
    }
    /// <summary>선언 머리나 handler 인자를 읽지 않고 실제+4 WORD만 현재 본체에 전달한다.</summary>
    public void HandlePacket(ReadOnlyMemory<byte> packet)
    {
        if (packet.Length < 6) throw new ArgumentOutOfRangeException(nameof(packet), "간헐천 채집 packet SID 범위"); Handle(BinaryPrimitives.ReadUInt16LittleEndian(packet.Span[4..]));
    }
}
