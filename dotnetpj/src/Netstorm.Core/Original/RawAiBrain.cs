using System.Buffers.Binary;
using Netstorm.Core.Simulation;

// 실제 AI 본체와 목록의 소유 메모리·생성·소멸을 공유하는 원본 코어다.
namespace Netstorm.Core.Original;

/// <summary>AI 할당 직후 아직 생성자가 쓰지 않은 바이트를 공급한다.</summary>
public delegate void AiBrainMemoryFill(Span<byte> bytes);

/// <summary>실제 소유 블록의 불투명32비트 토큰과 요청 크기다. 원본 주소와 같음을 뜻하지 않는다.</summary>
public readonly record struct AiBrainAllocation(uint Token, int Size);

/// <summary>AI의32비트 포인터 필드를 실제 .NET 소유 배열에 연결한다.
/// 근거: cpppj/src/o/RawAiBrain.h/.cpp의 RawAiBrainMemory. 호스트 포인터 대신 불투명 토큰을 쓴다.</summary>
public sealed class RawAiBrainMemory
{
    /// <summary>다음 블록에 부여할8바이트 정렬 토큰이다.</summary>
    private uint _next;
    /// <summary>각 할당의 미초기화 바이트 공급자다. 임의의0초기화를 원본으로 간주하지 않는다.</summary>
    private readonly AiBrainMemoryFill _fill;
    /// <summary>살아 있는 실제 블록의 유일한 소유자다.</summary>
    private readonly Dictionary<uint, byte[]> _blocks = [];
    /// <summary>성공한 할당의 순서와 크기다.</summary>
    private readonly List<AiBrainAllocation> _allocations = [];
    /// <summary>유효 블록의 실제 해제 순서다.</summary>
    private readonly List<uint> _frees = [];
    /// <summary>실제 할당 이력을 읽는다.</summary>
    public IReadOnlyList<AiBrainAllocation> Allocations => _allocations;
    /// <summary>실제 해제 이력을 읽는다.</summary>
    public IReadOnlyList<uint> Frees => _frees;

    /// <summary>비제로 첫 토큰과 각 할당의 바이트 공급자를 지정한다.</summary>
    public RawAiBrainMemory(uint firstToken, AiBrainMemoryFill fill)
    {
        if (firstToken == 0) throw new ArgumentOutOfRangeException(nameof(firstToken));
        ArgumentNullException.ThrowIfNull(fill);
        _next = firstToken;
        _fill = fill;
    }

    /// <summary>요청 크기의 실제 배열을 확보한다. 실패하면 다음 토큰과 할당 이력을 보존한다.</summary>
    public uint Allocate(int size)
    {
        ulong step = size > 0 ? ((ulong)size + 7) & ~7UL : 0;
        if (size <= 0 || step > uint.MaxValue - _next)
            throw new ArgumentOutOfRangeException(nameof(size), "AI 실제 소유 메모리 주소 공간 소진");
        var block = new byte[size];
        _fill(block);
        uint token = _next;
        if (!_blocks.TryAdd(token, block)) throw new InvalidOperationException("AI 메모리 핸들 중복");
        _next += (uint)step;
        _allocations.Add(new(token, size));
        return token;
    }

    /// <summary>0은 무동작이고 살아 있는 블록만 해제한다. 이중 해제와 가짜 토큰은 거부한다.</summary>
    public void Free(uint token)
    {
        if (token == 0) return;
        if (!_blocks.Remove(token)) throw new InvalidOperationException("AI 실제 소유자 없는 메모리 해제");
        _frees.Add(token);
    }

    /// <summary>현재 살아 있는 블록 전체를 빌린다. 해제된 토큰은 다시 조회할 수 없다.</summary>
    public byte[] Bytes(uint token) => _blocks.TryGetValue(token, out byte[]? raw)
        ? raw : throw new ArgumentOutOfRangeException(nameof(token), "AI 살아 있는 소유 메모리 누락");

    /// <summary>해당 실제 블록이 현재 살아 있는지 조회한다.</summary>
    public bool Contains(uint token) => _blocks.ContainsKey(token);
}

/// <summary>생성자가 각 원문 읽기 위치에서 조회하는 현재 장기 전역이다.</summary>
public sealed class AiBrainConstructorGlobals
{
    /// <summary>현재 로컬 소유자00540c70을 조회한다.</summary>
    public required Func<uint> LocalOwner { get; init; }
    /// <summary>현재 첫 지역 없음0050c038을 조회한다.</summary>
    public required Func<int> NoRegion { get; init; }
    /// <summary>같은 장부의 현재 무효 그래프00540410을 각 쓰기 위치에서 조회한다.</summary>
    public Func<uint>? InvalidGraph { get; init; }
}

/// <summary>생성자가 실제로 선택한 사제·좌표 지역·TYPE 요구 에너지 조회다.</summary>
public sealed class AiBrainConstructorHooks
{
    /// <summary>같은 실제 풀의 사제 SID를 조회한다(004921f0).</summary>
    public Func<uint, uint>? Priest { get; init; }
    /// <summary>현재 hash0의 지면 지역을 조회한다(0046f530).</summary>
    public Func<float, float, int>? RegionAt { get; init; }
    /// <summary>TYPE+a0의 실제 요구 에너지 문자열이다. 타입 이름으로 대체하지 않는다.</summary>
    public Func<uint, string>? RequiredEnergy { get; init; }
}

/// <summary>PATCH00416860의 실제 AI 생성자와00410100의 역순 목록 소멸자다.
/// 근거: cpppj/src/o/RawAiBrain.h/.cpp. 전략 갱신과 초기 갱신은 별도 몸체다.</summary>
public sealed class RawAiBrainConstructor
{
    /// <summary>00417e50이 요청하는 실제 본체 크기다.</summary>
    public const int BytesCount = 0xb08;
    /// <summary>원본 미지정 좌표-10000.0f의 비트다.</summary>
    private const uint Unset = 0xc61c4000;
    /// <summary>본체 안16개 목록의 포인터 위치와 DWORD 용량이다. 생성·소멸 순서를 공유한다.</summary>
    private static readonly (int Offset, uint Capacity)[] Lists =
    [
        (0x284, 10), (0x2dc, 500), (0x2e8, 500), (0x2f4, 500), (0x300, 4),
        (0x30c, 5), (0x318, 5), (0x324, 40), (0x330, 500), (0x33c, 500),
        (0x348, 500), (0x354, 500), (0x360, 500), (0x36c, 500), (0x378, 40), (0x384, 2),
    ];
    /// <summary>ae4할당 전에 원문 순서대로 기록하는 고정 DWORD다.</summary>
    private static readonly (int Offset, uint Value)[] FirstFields =
    [
        (0x3cc, 0), (0x3d4, Unset), (0x3d0, Unset), (0x3d8, 1), (0x3dc, 0),
        (0x3f8, 0), (0x404, 0), (0x3e0, 0), (0x3e4, 6), (0x3f0, 1), (0x3f4, 0),
        (0x3fc, 0), (0x400, 0), (0x3ec, 0), (0x574, 0), (0x5a4, 0), (0x58c, 0),
        (0x590, 0), (0x5a0, 0), (0x59c, 0), (0x598, 0), (0x594, 0), (0x628, 0),
        (0x5f4, 0), (0x5f0, 0), (0xadc, 10), (0xae0, 0),
    ];
    /// <summary>실제 double0을 쓰는 위치다. 주변 미초기화 바이트는 보존한다.</summary>
    private static readonly int[] Doubles = [0x5a8, 0x5b0, 0x5b8, 0x5c0, 0x5c8, 0x5d0, 0x5d8, 0x5e0, 0x5e8, 0x630];
    /// <summary>첫 지역·메타데이터 조회 전에 기록하는 마지막 고정 DWORD다.</summary>
    private static readonly (int Offset, uint Value)[] LastFields =
    [
        (0x2d4, 0), (0x264, 0), (0x2a4, 0), (0x258, 0), (0x25c, 0), (0x6d0, 0),
        (0xad8, 2), (0x390, 3), (0xad4, Unset), (0xad0, Unset), (0xaf4, Unset), (0xaf0, Unset),
        (0x3e0, 0), (0x3e4, 6), (0x3f0, 1), (0x3f4, 0), (0x3fc, 0), (0x400, 0),
        (0x3ec, 0), (0x3b0, 0), (0x394, 0), (0x2cc, 0), (0x2c8, 0), (0x2d0, 0),
        (0x290, 0), (0x3c8, 3), (0xaf8, 10), (0xafc, 10), (0xb00, 0), (0x294, 0),
    ];
    /// <summary>본체와 각 목록의 실제 소유자다.</summary>
    private readonly RawAiBrainMemory _memory;
    /// <summary>같은 Player9·첫 지역·실제 풀을 빌리는 부모다.</summary>
    private readonly RawPlayerWorld _player;
    /// <summary>같은 방향별 동맹·Chat·brain 수명 부모다.</summary>
    private readonly PlayerParticipation _participation;
    /// <summary>같은 섬 존재·소유·Player 첫 지역 몸체다.</summary>
    private readonly RawIslandRuntime _islands;
    /// <summary>현재 방향별81DWORD 표다.</summary>
    private readonly uint[] _allies;
    /// <summary>현재 TYPE번호 순flags2표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 세 전역의 조회기다. C++ 장기 참조를 대리자로 빌린다.</summary>
    private readonly AiBrainConstructorGlobals _globals;
    /// <summary>선택된 실제 외부 조회다.</summary>
    private readonly AiBrainConstructorHooks _hooks;
    /// <summary>실제 초기화를 완료한 본체다.</summary>
    private readonly HashSet<uint> _live = [];
    /// <summary>목록 소멸을 마쳐 본체 해제만 남은 토큰이다.</summary>
    private readonly HashSet<uint> _destructed = [];

    /// <summary>같은 PATCH세계·Player9·방향별 동맹을 연결한다. 생성자 완료 전 brain을 부착하지 않는다.</summary>
    public RawAiBrainConstructor(RawAiBrainMemory memory, RawPlayerWorld player, PlayerParticipation participation,
        RawIslandRuntime islands, uint[] allies, IReadOnlyList<OriginalTypeRecord> types,
        AiBrainConstructorGlobals globals, AiBrainConstructorHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(player);
        ArgumentNullException.ThrowIfNull(participation);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(allies);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (player.Pool().Edition != OriginalEdition.Patch1078 || !ReferenceEquals(player.Pool(), islands.Pool())
            || player.Records().Length != 1620 || allies.Length != 81 || types.Count == 0
            || globals.InvalidGraph is null || globals.LocalOwner is null || globals.NoRegion is null)
            throw new ArgumentException("AI 생성자 같은 PATCH세계/Player9/동맹표/현재 그래프 조회 누락");
        _memory = memory; _player = player; _participation = participation; _islands = islands;
        _allies = allies; _types = types; _globals = globals; _hooks = hooks;
    }

    /// <summary>정렬되지 않은 현재 DWORD를 읽는다.</summary>
    private static uint Read(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));
    /// <summary>주변 바이트를 보존하고 정확히4바이트만 쓴다.</summary>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset, 4), value);
    /// <summary>도달한 실제 조회기가 없으면 가짜 결과를 만들지 않고 거부한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("AI 생성자 실제 조회 미연결");
    /// <summary>첫 NUL 전의 요구 에너지ss를 ASCII대소문자 없이 비교한다.</summary>
    private static bool DoubleSun(string value)
    {
        int nul = value.IndexOf('\0');
        ReadOnlySpan<char> text = value.AsSpan(0, nul < 0 ? value.Length : nul);
        return text.Length == 2 && text[0] is 's' or 'S' && text[1] is 's' or 'S';
    }

    /// <summary>본체를 할당하고 실제 생성자를 실행한다. 중간 실패에는 구성한 목록과 본체를 정리한다.</summary>
    public uint Construct(uint owner, ReadOnlySpan<byte> settings, int bytes = BytesCount)
    {
        if (bytes != BytesCount) throw new ArgumentException("AI 실제 본체 크기는0xb08입니다", nameof(bytes));
        uint token = _memory.Allocate(BytesCount);
        try { Initialize(token, owner, settings); }
        catch { _memory.Free(token); throw; }
        return token;
    }

    /// <summary>이미 확보한0xb08블록에 원문 쓰기만 수행한다. 원본이 남긴 미초기화 바이트는 보존한다.</summary>
    public void Initialize(uint token, uint owner, ReadOnlySpan<byte> settings)
    {
        if (settings.Length != RawMissionAiSettings.Size || owner >= 9 || _live.Contains(token) || _destructed.Contains(token))
            throw new ArgumentException("AI 생성자 실제 설정/owner/새 본체 계약 오류");
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != BytesCount) throw new ArgumentException("AI 생성자 실제0xb08소유 메모리 누락");
        var made = new List<uint>();
        // 생성 중 구성한 실제 목록만 예외 시 역순 정리하도록 등록한다.
        uint Allocate(int bytes)
        {
            uint block = _memory.Allocate(bytes);
            made.Add(block);
            return block;
        }
        // 외부 조회가 brain owner를 바꾼 경우에도 원문 다음 읽기에서 현재 값을 사용한다.
        int PlayerOffset()
        {
            uint current = Read(raw, 0x260);
            if (current >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "AI 현재owner 실제9기록 범위 오류");
            return (int)current * 180;
        }
        try
        {
            settings.CopyTo(raw);
            Put(raw, 0x274, Unset); Put(raw, 0x270, Unset); Put(raw, 0x27c, Unset); Put(raw, 0x278, Unset);
            Put(raw, 0x284, Allocate(40)); Put(raw, 0x288, 10); Put(raw, 0x28c, 0);
            Put(raw, 0x2bc, Unset); Put(raw, 0x2b8, Unset); Put(raw, 0x2c4, Unset); Put(raw, 0x2c0, Unset);
            // 첫 목록 뒤15개를 포인터·용량·개수 순서로 구성한다.
            for (int index = 1; index < Lists.Length; index++)
            {
                (int offset, uint capacity) = Lists[index];
                Put(raw, offset, Allocate((int)capacity * 4)); Put(raw, offset + 4, capacity); Put(raw, offset + 8, 0);
            }
            // 원문 상수와0필드만 실제 저장소에 순서대로 쓴다.
            foreach ((int offset, uint value) in FirstFields) Put(raw, offset, value);
            // 실제 double0의 두DWORD를 쓴다.
            foreach (int offset in Doubles) { Put(raw, offset, 0); Put(raw, offset + 4, 0); }
            Put(raw, 0xaec, owner); Put(raw, 0xae8, 0); Put(raw, 0xae4, Allocate(120));
            Put(raw, 0x2d8, Read(raw, 0x22c)); Put(raw, 0x260, owner);
            // 원본 고정 배열255DWORD만0으로 만든다.
            for (int index = 0; index < 255; index++) Put(raw, 0x6d4 + index * 4, 0);
            // 주변 미초기화 영역을 보존하고 마지막 고정 필드를 쓴다.
            foreach ((int offset, uint value) in LastFields) Put(raw, offset, value);
            Put(raw, 0x298, _globals.InvalidGraph!()); Put(raw, 0x2a8, 0); Put(raw, 0x2ac, _globals.InvalidGraph());
            byte[] records = _player.Records();
            if (unchecked((int)Read(records, PlayerOffset() + 0x78)) == _globals.NoRegion())
            {
                uint priest = Need(_hooks.Priest)(Read(raw, 0x260));
                if (priest > 0 && priest < _player.Pool().Capacity)
                {
                    ReadOnlySpan<byte> piece = _player.Pool().Slot((int)priest);
                    int region = Need(_hooks.RegionAt)(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(piece.Slice(14, 4))),
                        BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(piece.Slice(18, 4))));
                    if (_islands.Exists(region) != 0)
                    {
                        _islands.Owner(region, Read(raw, 0x260));
                        Put(records, PlayerOffset() + 0x78, unchecked((uint)region));
                    }
                }
            }
            Put(raw, 0x280, 0);
            // 실제flags40800과 요구 에너지ss를 모두 만족하는 첫 타입만 쓴다.
            for (int type = 0; type < _types.Count; type++)
                if ((_types[type].Flags2 & 0x40800u) != 0 && DoubleSun(Need(_hooks.RequiredEnergy)((uint)type)))
                { Put(raw, 0x280, (uint)type); break; }
            if ((raw[0x248] & 1) == 0) { Put(raw, 0x370, 0); Put(raw, 0x37c, 0); }
            _participation.SetAlliance(Read(raw, 0x260), Read(raw, 0x23c));
            uint current = Read(raw, 0x260);
            if (current >= 9 || _globals.LocalOwner() >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "AI 현재 방향별 동맹 범위 오류");
            if (_allies[current * 9 + _globals.LocalOwner()] != 0) _participation.AddAlly(_globals.LocalOwner(), Read(raw, 0x260));
            _live.Add(token);
        }
        catch
        {
            // 실제 구성에 성공한 목록만 역순 해제한다. caller가 소유한 본체는 남긴다.
            for (int index = made.Count - 1; index >= 0; index--) _memory.Free(made[index]);
            throw;
        }
    }

    /// <summary>ae4조건부 해제 뒤16개 목록을 현재 포인터로 역순 해제한다. 본체는 아직 살아 있다.</summary>
    public void Destruct(uint token)
    {
        if (!_live.Contains(token)) throw new InvalidOperationException("AI 실제 생성자 완료 없는 소멸 요청");
        byte[] raw = _memory.Bytes(token);
        if (Read(raw, 0xae4) != 0) _memory.Free(Read(raw, 0xae4));
        // 용량이0으로 바뀌었어도 현재 포인터를 원문 역순으로 해제한다.
        for (int index = Lists.Length - 1; index >= 0; index--) _memory.Free(Read(raw, Lists[index].Offset));
        _live.Remove(token);
        _destructed.Add(token);
    }

    /// <summary>Participation의 목록 소멸 뒤 본체 해제 단계에 연결한다.</summary>
    public void FreeBody(uint token)
    {
        if (!_destructed.Remove(token)) throw new InvalidOperationException("AI 실제 목록 소멸 전 본체 해제 요청");
        _memory.Free(token);
    }

    /// <summary>초기 갱신·전략이 읽고 쓰는 동일한 살아 있는 본체를 빌린다.</summary>
    public byte[] Bytes(uint token) => _memory.Bytes(token);
    /// <summary>상위 바인딩에서 같은 실제 풀인지 확인할 때 쓴다.</summary>
    public RawSidPool Pool() => _player.Pool();
}
