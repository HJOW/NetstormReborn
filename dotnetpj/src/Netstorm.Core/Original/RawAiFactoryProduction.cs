using System.Buffers.Binary;

// AI 시설의 현재 기술·생산·업그레이드 목록을 원본 순서로 처리한다.
namespace Netstorm.Core.Original;

/// <summary>생산 등록과 업그레이드 요청 뒤 기록하는 행동 종류다.</summary>
public enum AiFactoryAction
{
    /// <summary>시설에 생산 TYPE을 등록했다.</summary>
    AddedProduct,
    /// <summary>시설의 업그레이드를 요청했다.</summary>
    Upgrade,
}

/// <summary>시설 판단이 각 분기에서 읽는 현재 전역이다.</summary>
public sealed class AiFactoryProductionGlobals
{
    /// <summary>현재 QWORD 게임 시각이다.</summary>
    public Func<double>? Now { get; set; }
    /// <summary>현재 무효 그래프 번호다.</summary>
    public Func<uint>? InvalidGraph { get; set; }
}

/// <summary>같은 Player·생산·시설 레벨·업그레이드 요청과 장치 효과를 연결한다.</summary>
public sealed class AiFactoryProductionHooks
{
    /// <summary>현재 owner의 기술 부모를 읽는다.</summary>
    public Func<uint, int>? Technology { get; set; }
    /// <summary>현재 brain의 기술 부모에서 생산 조각을 찾는다.</summary>
    public Func<uint, uint, int>? Product { get; set; }
    /// <summary>실제 로컬 또는 원격 생산 등록 본체다.</summary>
    public Action<uint, int, uint>? AddProduct { get; set; }
    /// <summary>현재 시설의 가상 레벨을 읽는다.</summary>
    public Func<int, uint>? Level { get; set; }
    /// <summary>실제 업그레이드를 요청한다.</summary>
    public Action<int>? Upgrade { get; set; }
    /// <summary>현재 로그 종류와 TYPE 이름을 기록할 장치다.</summary>
    public Action<uint, AiFactoryAction, uint>? Log { get; set; }
    /// <summary>현재 데모의 지갑 하한 적용 여부다.</summary>
    public Func<bool>? Demo { get; set; }
}

/// <summary>00413570 기술 선택·004136b0 시설 생산·00411180 업그레이드 완료를 실행한다.</summary>
/// <remarks>근거: cpppj/src/o/RawAiFactoryProduction.cpp·h. 2026-11-20 0.663에서 이식했다.</remarks>
public sealed class RawAiFactoryProduction
{
    /// <summary>업그레이드 후보는 signed 지갑이 이 값보다 커야 한다.</summary>
    private const int UpgradeBudget = 1199;
    /// <summary>기존 AI와 같은 본체·목록 메모리다.</summary>
    private readonly RawAiBrainMemory _memory;
    /// <summary>시설·기술·생산 내용물을 조회하는 실제 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 원소·genus·비용을 조회하는 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 권한과 내용물 TYPE을 공유하는 검색 상태다.</summary>
    private readonly ContainedFinderState _finder;
    /// <summary>현재 시각·무효 그래프 장치다.</summary>
    private readonly AiFactoryProductionGlobals _globals;
    /// <summary>선택된 실제 하위 효과다.</summary>
    private readonly AiFactoryProductionHooks _hooks;

    /// <summary>게임 효과 없이 같은 PATCH 풀과 TYPE 계약을 확인한다.</summary>
    public RawAiFactoryProduction(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        ContainedFinderState finder, AiFactoryProductionGlobals globals, AiFactoryProductionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(finder);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count == 0)
        {
            throw new ArgumentException("AI 시설 생산 같은 PATCH/TYPE 계약 누락");
        }
        _memory = memory;
        _pool = pool;
        _types = types;
        _finder = finder;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>현재 기술 내용물 중 원소·치환 모드가 맞으며 아직 생산이 없는 첫 TYPE을 고른다.</summary>
    public uint SelectTechnology(uint token, int theme)
    {
        int parent = Need(_hooks.Technology)(Field(token, 0x260));
        var cursor = new RawContainedFinder(_pool, _finder);
        // 커서가 next를 먼저 저장하므로 생산 조회가 현재 링크를 바꿔도 다음 항목을 잃지 않는다.
        for (int form = cursor.Begin(parent); form != 0; form = cursor.Next())
        {
            ReadOnlySpan<byte> raw = _pool.Slot(form);
            if ((raw[20] & 4) == 0)
            {
                continue;
            }
            uint type = BinaryPrimitives.ReadUInt16LittleEndian(raw.Slice(18, 2));
            OriginalTypeRecord entry = Type(type);
            bool matches = entry.Theme == theme;
            if ((entry.Flags2 & 0x800u) != 0)
            {
                long offset = 0x3b4L + (long)entry.Theme * 4;
                if (offset < 0 || offset > RawAiBrainConstructor.BytesCount - 4)
                {
                    throw new ArgumentOutOfRangeException(nameof(token), "AI 시설 생산 원소 치환 필드 범위");
                }
                uint replacement = Field(token, (int)offset);
                if (replacement == 1)
                {
                    matches = theme == 3;
                }
                else if (replacement == 2)
                {
                    matches = matches || theme == 3;
                }
                else if (replacement != 0)
                {
                    matches = false;
                }
            }
            if (matches && Need(_hooks.Product)(token, type) == 0)
            {
                return type;
            }
        }
        return 0;
    }

    /// <summary>현재 mask0x10 생산 내용물 총수다. 같은 TYPE의 중복도 각각 센다.</summary>
    public int CountProducts(int factory) => RawContainedFinder.CountContained(_pool, _finder, factory, 0x10, 0);

    /// <summary>첫 미처리 시설에 생산을 추가하거나 첫 업그레이드 대기 시설을 요청한다.</summary>
    public void Manage(uint token)
    {
        if (Field(token, 0x294) == 0 || Field(token, 0x298) == Need(_globals.InvalidGraph)() || !Ready(token))
        {
            return;
        }
        if (Signed(Field(token, 0x314)) < 1)
        {
            if (Field(token, 0x314) == 0 && Field(token, 0x2cc) != 0
                && Signed(Field(token, 0x2d8)) > UpgradeBudget && Signed(Field(token, 0x320)) > 0)
            {
                uint value = Item(token, 0x318, 0);
                Need(_hooks.Upgrade)(PhysicalSid(value));
                Remove(token, 0x318, value);
                Action(token, AiFactoryAction.Upgrade, 0);
            }
            return;
        }
        uint current = Item(token, 0x30c, 0);
        int factory = PhysicalSid(current);
        uint level = Need(_hooks.Level)(factory);
        int capacity = Signed(unchecked(level + 2u));
        int count = CountProducts(factory);
        uint type = SelectTechnology(token, Theme(factory));
        if (type != 0 && count < capacity)
        {
            Need(_hooks.AddProduct)(token, factory, type);
            _ = Type(type);
            Action(token, AiFactoryAction.AddedProduct, type);
            if (Signed(unchecked((uint)count + 1u)) < capacity)
            {
                return;
            }
        }
        Remove(token, 0x30c, current);
        if (type != 0 && Signed(Need(_hooks.Level)(factory)) < 2 && SelectTechnology(token, Theme(factory)) != 0)
        {
            Append(token, 0x318, current);
        }
    }

    /// <summary>완료 시설을 중복 허용 추가한 뒤 현재 TYPE 음수 비용을 절삭 가산하고 데모 하한을 적용한다.</summary>
    public void Upgraded(uint token, int factory)
    {
        Append(token, 0x30c, unchecked((uint)factory));
        uint delta = Truncate(-(double)Type(_pool.Slot(factory)[10]).Cost);
        Field(token, 0x2d8, unchecked(Field(token, 0x2d8) + delta));
        if (Need(_hooks.Demo)())
        {
            int wallet = Signed(Field(token, 0x2d8));
            Field(token, 0x2d8, unchecked((uint)(wallet > 1190 ? wallet : 1190)));
        }
    }

    /// <summary>실제 월드 조립 시 같은 풀을 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>현재 시설 BYTE TYPE에서 원소를 다시 읽는다.</summary>
    private int Theme(int factory) => Type(_pool.Slot(factory)[10]).Theme;
    /// <summary>NaN 비교도 원본처럼 거짓이며 QWORD 정밀도를 보존한다.</summary>
    private bool Ready(uint token) => Need(_globals.Now)() >= BitConverter.UInt64BitsToDouble(Field(token, 0x5a8) | ((ulong)Field(token, 0x5ac) << 32));
    /// <summary>현재 float 주기·QWORD 시각으로 생각 기한 두 개를 쓴 뒤 선택된 로그를 보낸다.</summary>
    private void Action(uint token, AiFactoryAction action, uint type)
    {
        double rate = BitConverter.UInt32BitsToSingle(Field(token, 0x228));
        double now = Need(_globals.Now)();
        ulong first = BitConverter.DoubleToUInt64Bits(rate + now);
        ulong second = BitConverter.DoubleToUInt64Bits((rate + 1.0) + now);
        Field(token, 0x5a8, unchecked((uint)first));
        Field(token, 0x5ac, unchecked((uint)(first >> 32)));
        Field(token, 0x5b0, unchecked((uint)second));
        Field(token, 0x5b4, unchecked((uint)(second >> 32)));
        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.Log)(token, action, type);
        }
    }
    /// <summary>모든 일치 항목만 안정 압축하고 사용하지 않는 꼬리를 지우지 않는다.</summary>
    private void Remove(uint token, int pointer, uint value)
    {
        uint removed = 0;
        // 현재 count·포인터를 매 방문에서 다시 읽어 상대 순서를 보존한다.
        for (int index = 0; index < Signed(Field(token, pointer + 8)); index++)
        {
            uint item = Item(token, pointer, index);
            if (item == value)
            {
                removed++;
            }
            else
            {
                BinaryPrimitives.WriteUInt32LittleEndian(_memory.Bytes(Field(token, pointer)).AsSpan(checked((int)((uint)index - removed) * 4), 4), item);
            }
        }
        Field(token, pointer + 8, unchecked(Field(token, pointer + 8) - removed));
    }
    /// <summary>여유가 있을 때 중복 허용 추가한다. 가득 찼으면 backing을 읽지 않는다.</summary>
    private void Append(uint token, int pointer, uint value)
    {
        int count = Signed(Field(token, pointer + 8));
        if (count >= Signed(Field(token, pointer + 4)))
        {
            return;
        }
        if (count < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "AI 시설 생산 음수 추가 인덱스");
        }
        BinaryPrimitives.WriteUInt32LittleEndian(_memory.Bytes(Field(token, pointer)).AsSpan(checked(count * 4), 4), value);
        Field(token, pointer + 8, unchecked(Field(token, pointer + 8) + 1u));
    }
    /// <summary>현재 본체가 소유한 목록 backing의 물리 칸만 읽는다.</summary>
    private uint Item(uint token, int pointer, int index) => index >= 0
        ? BinaryPrimitives.ReadUInt32LittleEndian(_memory.Bytes(Field(token, pointer)).AsSpan(checked(index * 4), 4))
        : throw new ArgumentOutOfRangeException(nameof(index), "AI 시설 생산 음수 목록 인덱스");
    /// <summary>DWORD SID를 다른 WORD로 자르지 않고 물리 범위를 확인한다.</summary>
    private int PhysicalSid(uint value) => value < (uint)_pool.Capacity && value <= ushort.MaxValue ? (int)value
        : throw new ArgumentOutOfRangeException(nameof(value), "AI 시설 생산 현재 SID 범위");
    /// <summary>현재 TYPE 0도 허용하고 표 밖만 거부한다.</summary>
    private OriginalTypeRecord Type(uint type) => type < (uint)_types.Count ? _types[(int)type]
        : throw new ArgumentOutOfRangeException(nameof(type), "AI 시설 생산 현재 TYPE 범위");
    /// <summary>현재 살아 있는 실제 0xb08 본체만 읽는다.</summary>
    private byte[] Body(uint token)
    {
        byte[] bytes = _memory.Bytes(token);
        return bytes.Length == RawAiBrainConstructor.BytesCount ? bytes
            : throw new ArgumentException("AI 시설 생산 실제0xb08 본체 누락", nameof(token));
    }
    /// <summary>하위 효과 뒤 현재 DWORD를 다시 읽는다.</summary>
    private uint Field(uint token, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(Body(token).AsSpan(offset, 4));
    /// <summary>선택한 현재 DWORD만 쓴다.</summary>
    private void Field(uint token, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Body(token).AsSpan(offset, 4), value);
    /// <summary>signed 비교에서 DWORD 비트를 유지한다.</summary>
    private static int Signed(uint value) => unchecked((int)value);
    /// <summary>원본 ftol의 signed64 절삭 하위 DWORD와 무효 입력의 0을 보존한다.</summary>
    private static uint Truncate(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0
        ? 0u : unchecked((uint)(long)Math.Truncate(value));
    /// <summary>선택된 누락 효과를 명시 오류로 남긴다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("AI 시설 생산 실제 하위·전역 미연결");
}
