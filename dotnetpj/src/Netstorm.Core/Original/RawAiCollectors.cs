// 원본 공통 AI의 수집자 확보00415a10과 자기 영역 후보 배치00415920을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 수집자 모듈을 담는 네임스페이스다. 2026-11-19 0.655에서 R4-5 AI 수집용으로 추가했다.

// 같은 원본 현재 시각/로컬 소유자/무효 그래프를 읽는다.
public sealed class AiCollectorsGlobals
{
    /// <summary>현재 QWORD 시각이다. 0.655에서 추가했다.</summary>
    public Func<double>? Now { get; set; }

    /// <summary>현재 로컬 소유자다. 0.655에서 추가했다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>현재 무효 그래프다. 0.655에서 추가했다.</summary>
    public Func<uint>? InvalidGraph { get; set; }
}

// 이미 복원된 실제 생산/비용/배치와 현재 사제 상태·로그/음향을 연결한다.
public sealed class AiCollectorsHooks
{
    /// <summary>같은 본체→현재 기술부모 mask8 조회다. 0.655에서 추가했다.</summary>
    public Func<uint, uint, int>? Product { get; set; }

    /// <summary>실제 signed 비용/지갑 비교다. 0.655에서 추가했다.</summary>
    public Func<uint, uint, bool>? CanAfford { get; set; }

    /// <summary>현재 최대HP이며 실제HP는 호출 전에 저장한다. 0.655에서 추가했다.</summary>
    public Func<int, int>? MaxHitPoints { get; set; }

    /// <summary>현재 Player flags22와 ownerState0이다. 0.655에서 추가했다.</summary>
    public Func<uint, bool>? EnemyAvailable { get; set; }

    /// <summary>전체 배치 가능 검사이며 mode1/방향0/현재owner바이트를 전달한다. 0.655에서 추가했다.</summary>
    public Func<CanonPlacementQuery, bool>? MayPlace { get; set; }

    /// <summary>실제 생산 소비/건설이다. 0.655에서 추가했다.</summary>
    public Action<uint, uint, float, float, uint, uint>? Deploy { get; set; }

    /// <summary>현재 Lay 형식/로그 종류 장치다. 0.655에서 추가했다.</summary>
    public Action<uint, uint, int, int>? Log { get; set; }

    /// <summary>로컬 owner일 때만 droppiece.wav를 재생한다. 0.655에서 추가했다.</summary>
    public Action? DropSound { get; set; }
}

/// <summary>별도 공략/수집자 목록 없이 같은 본체/목록/TYPE/난수/원본 배치를 사용한다.
/// 취약 사제용 비행 수집자 또는 일반 수집자를 선택한다.</summary>
/// <remarks>2026-11-19 0.655에서 추가했다. 근거: cpppj <c>o/RawAiCollectors.h/.cpp</c>의 <c>RawAiCollectors</c>다. 후보 배치에서 같은 게임 난수를 소비한다.</remarks>
public sealed class RawAiCollectors
{
    /// <summary>후보 배치 최대 반복 횟수다. 0.655에서 추가했다.</summary>
    private const int Attempts = 20;

    /// <summary>같은 PATCH 풀/현재 TYPE만 먼저 확인한다. 0.655에서 추가했다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>사제와 현재 수집자의 실제 슬롯을 읽는다. 0.655에서 추가했다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 생산/수집자의 flags2를 읽는다. 0.655에서 추가했다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>기존 SP/생산/전투와 같은 원본 난수 스트림이다. 0.655에서 추가했다.</summary>
    private readonly GameRandom _random;

    /// <summary>현재 전역의 참조/조회기다. 0.655에서 추가했다.</summary>
    private readonly AiCollectorsGlobals _globals;

    /// <summary>선택된 실제 하위/장치 효과다. 0.655에서 추가했다.</summary>
    private readonly AiCollectorsHooks _hooks;

    /// <summary>AI 수집자 본체를 만든다. 같은 PATCH 풀/현재 TYPE만 먼저 확인한다.</summary>
    public RawAiCollectors(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, GameRandom random,
        AiCollectorsGlobals globals, AiCollectorsHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count == 0)
        {
            throw new ArgumentException("AI 수집자 같은 PATCH/TYPE 계약 누락", nameof(pool));
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _random = random;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>00415a10의 취약 사제용 비행 수집자 또는 일반 수집자 선택이다. 0.655에서 추가했다.</summary>
    public void Ensure(uint token)
    {
        if (Field(token, 0x294) == 0 || Field(token, 0x298) == Need(_globals.InvalidGraph)() || Field(token, 0x28c) == 0)
        {
            return;
        }

        uint enemySid = Field(token, 0x2a0);
        // 원본 취약 사제용 비행 수집자 경로는 일반 비용/대기 조각 판정을 추가로 적용하지 않는다.
        if (Ready(token, 0x5a8) && _random.Next(1000) < 100 && (Field(token, 0x248) & 0x100u) != 0
            && Signed(Field(token, 0x38c)) < Signed(Field(token, 0x388))
            && enemySid >= 1 && enemySid < unchecked((uint)_pool.Capacity)
            && Need(_hooks.EnemyAvailable)(Field(token, 0x264))
            && Vulnerable(PhysicalSid(Field(token, 0x2a0)))
            && Ready(token, 0x5e0) && !HasFlying(token))
        {
            // 현재 후보 목록을 원래 순서로 방문해 첫 생산 보유 비행 타입에서 끝낸다.
            for (int index = 0; index < Signed(Field(token, 0x28c)); index++)
            {
                uint type = Item(token, 0x284, index);
                if (Need(_hooks.Product)(token, type) != 0 && (_types[(int)type].Flags2 & 0x20000u) != 0)
                {
                    PlaceInHome(token, type);
                    return;
                }
            }
        }

        if (Field(token, 0x2cc) != 0)
        {
            return;
        }

        int picked = Signed(_random.Next(Field(token, 0x28c)));
        uint pickedType = Item(token, 0x284, picked);
        if (Need(_hooks.Product)(token, pickedType) == 0)
        {
            pickedType = Item(token, 0x284, 0);
        }

        if (Need(_hooks.Product)(token, pickedType) != 0 && Ready(token, 0x5a8)
            && AvailableProduct(token, pickedType) != 0 && Need(_hooks.CanAfford)(token, pickedType))
        {
            PlaceInHome(token, pickedType);
        }
    }

    /// <summary>00415920: 자기 경계에서 y→x 난수로 최대 20회 후보를 찾는다. 0.655에서 추가했다.</summary>
    public void PlaceInHome(uint token, uint type)
    {
        // 전체 MayPlace가 허용한 첫 후보만 배치하고 최대 20회 뒤에는 아무 조각도 만들지 않는다.
        for (int remaining = Attempts; remaining != 0; remaining--)
        {
            float left = BitConverter.UInt32BitsToSingle(Field(token, 0x270));
            float top = BitConverter.UInt32BitsToSingle(Field(token, 0x274));
            float right = BitConverter.UInt32BitsToSingle(Field(token, 0x278));
            float bottom = BitConverter.UInt32BitsToSingle(Field(token, 0x27c));
            float y = (float)(Signed(_random.Next(Truncate(bottom - top))) + top);
            float x = (float)(Signed(_random.Next(Truncate(right - left))) + left);
            var query = new CanonPlacementQuery(type, 0, x, y, 0, Field(token, 0x260) & 255u, 1);
            Type(type);
            if (Need(_hooks.MayPlace)(query))
            {
                DeployWithNotice(token, type, x, y, 0, 0);
                return;
            }
        }
    }

    /// <summary>004108f0의 실제 배치→생각 기한/로그→로컬 소리다. 0.655에서 추가했다.</summary>
    public void DeployWithNotice(uint token, uint type, float x, float y, uint argument, uint direction)
    {
        Need(_hooks.Deploy)(token, type, x, y, argument, direction);
        Notice(token, type, x, y);
        if (Field(token, 0x260) == Need(_globals.LocalOwner)())
        {
            Need(_hooks.DropSound)();
        }
    }

    /// <summary>004115e0의 현재 수집자 목록 genus20000 검사다. 0.655에서 추가했다.</summary>
    public bool HasFlying(uint token)
    {
        // 하위가 현재 목록을 바꾸면 다음 원문 읽기에서 반영한다.
        for (int index = 0; index < Signed(Field(token, 0x2e4)); index++)
        {
            if ((_types[(int)_pool.Slot(PhysicalSid(Item(token, 0x2dc, index)))[10]].Flags2 & 0x20000u) != 0)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>00411ca0의 현재HP 선저장→최대HP/2 signed 비교다. 0.655에서 추가했다.</summary>
    public bool Vulnerable(int priest)
    {
        int hp = Signed(BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(priest).Slice(26, 4)));
        int maximum = Need(_hooks.MaxHitPoints)(priest);
        return hp < maximum / 2;
    }

    /// <summary>실제 같은 월드 연결 확인용 풀이다. 0.655에서 추가했다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>원본 QWORD 비교의 unordered/기한 전은 모두 거짓이다. 0.655에서 추가했다.</summary>
    private bool Ready(uint token, uint offset)
    {
        double due = BitConverter.UInt64BitsToDouble(((ulong)Field(token, offset + 4) << 32) | Field(token, offset));
        return Need(_globals.Now)() >= due;
    }

    /// <summary>같은 원본 생산 조회를 다시 호출한 뒤 현재 조각의 pending bit100만 제외한다. 0.655에서 추가했다.</summary>
    private int AvailableProduct(uint token, uint type)
    {
        int form = Need(_hooks.Product)(token, type);
        return form != 0 && (_pool.Slot(form)[21] & 1) != 0 ? 0 : form;
    }

    /// <summary>원본 로그 좌표는 y→x 절삭이며 생각 기한은 float rate/QWORD now를 사용한다. 0.655에서 추가했다.</summary>
    private void Notice(uint token, uint type, float x, float y)
    {
        int iy = Signed(Truncate(y)), ix = Signed(Truncate(x));
        Type(type);
        double rate = BitConverter.UInt32BitsToSingle(Field(token, 0x228));
        double now = Need(_globals.Now)();
        ulong first = BitConverter.DoubleToUInt64Bits(rate + now);
        Field(token, 0x5a8, unchecked((uint)first));
        Field(token, 0x5a8 + 4, unchecked((uint)(first >> 32)));
        ulong second = BitConverter.DoubleToUInt64Bits(rate + 1.0 + now);
        Field(token, 0x5b0, unchecked((uint)second));
        Field(token, 0x5b0 + 4, unchecked((uint)(second >> 32)));
        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.Log)(token, type, ix, iy);
        }
    }

    /// <summary>현재 살아 있는 0xb08 본체를 다시 조회한다. 0.655에서 추가했다.</summary>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 수집자 실제0xb08 본체 누락", nameof(token));
        }

        return raw;
    }

    /// <summary>외부 효과 뒤에도 현재 본체를 다시 읽는다. 0.655에서 추가했다.</summary>
    private uint Field(uint token, uint offset) => BinaryPrimitives.ReadUInt32LittleEndian(Body(token).AsSpan((int)offset, 4));

    /// <summary>현재 본체의 해당 DWORD만 쓴다. 0.655에서 추가했다.</summary>
    private void Field(uint token, uint offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Body(token).AsSpan((int)offset, 4), value);

    /// <summary>목록의 개수와 별개로 현재 backing 안의 실제 칸만 읽는다. 0.655에서 추가했다.</summary>
    private uint Item(uint token, uint pointer, int index)
    {
        if (index < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI 수집자 음수 목록 인덱스");
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(_memory.Bytes(Field(token, pointer)).AsSpan(unchecked((int)((uint)index * 4u)), 4));
    }

    /// <summary>현재 TYPE0도 허용하되 표 밖 접근은 거부한다. 0.655에서 추가했다.</summary>
    private OriginalTypeRecord Type(uint type)
    {
        if (type >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 수집자 현재 TYPE 범위");
        }

        return _types[(int)type];
    }

    /// <summary>원본 적 유효성은 별도로 0을 제외하며 이 물리 getter는 0도 보존한다. 0.655에서 추가했다.</summary>
    private int PhysicalSid(uint value)
    {
        if (value >= (uint)_pool.Capacity || value > ushort.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 수집자 현재 SID 범위");
        }

        return unchecked((int)value);
    }

    /// <summary>DWORD의 signed 분기를 보존한다. 0.655에서 추가했다.</summary>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>CRTftol의 signed64 절삭 후 하위 DWORD다. 무효 FISTP의 하위 DWORD는 0이다. 0.655에서 추가했다.</summary>
    private static uint Truncate(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)(long)value);
    }

    /// <summary>선택된 하위의 누락을 명시 오류로 남긴다. 0.655에서 추가했다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 수집자 실제 하위 미연결");
}
