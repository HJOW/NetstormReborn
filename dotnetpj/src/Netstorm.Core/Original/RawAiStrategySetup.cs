// 원본 AI의 첫 영역·다리·생산 준비와 전략 목표 선택을 같은 본체에서 실행한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 전략 설정 모듈을 담는 네임스페이스다. 2026-11-19 0.646에서 R4-5 AI 초기화용으로 추가했다.

// 실제 원본 전역의 현재 저장소를 빌린다. 생성 시점의 값을 복제하지 않는다.
public sealed class AiStrategySetupGlobals
{
    /// <summary>현재 사람/데모 로컬 소유자다. 0.646에서 추가했다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>지역 없음 번호다. 0.646에서 추가했다.</summary>
    public Func<int>? NoRegion { get; set; }

    /// <summary>첫 신전 생산 TYPE이다. 0.646에서 추가했다.</summary>
    public Func<uint>? TempleProduct { get; set; }

    /// <summary>다리 내용물 TYPE이다. 0.646에서 추가했다.</summary>
    public Func<uint>? BridgeType { get; set; }

    /// <summary>signed 지도 좌표 상한이다. 0.646에서 추가했다.</summary>
    public Func<int>? MapLimit { get; set; }

    /// <summary>현재 게임 시각이다. 0.646에서 추가했다.</summary>
    public Func<double>? Now { get; set; }
}

// 월드가 이미 소유한 지역/삭제와 원본 진단 장치만 연결한다.
public sealed class AiStrategySetupHooks
{
    /// <summary>실제 자기 청크 경계다. 0.646에서 추가했다.</summary>
    public Func<int, float[]>? RegionBounds { get; set; }

    /// <summary>현재 지역 신전 조회다. 0.646에서 추가했다.</summary>
    public Func<int, uint>? Temple { get; set; }

    /// <summary>가상+10의 실제 내용물 삭제다. 0.646에서 추가했다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>원본 assert 이후 계속하는 진단이며 미공급이면 명시 실패한다. 0.646에서 추가했다.</summary>
    public Action<string>? Assertion { get; set; }

    /// <summary>기한 전 로컬 재고를 보충할 때만 현재 데모 여부를 읽는다. 0.646에서 추가했다.</summary>
    public Func<bool>? Demo { get; set; }
}

/// <summary>초기화 258과 최초 월드 스캔 25c를 분리하고 실제 Player 기술부모/내용물/게임 난수를 공유한다.
/// 00414af0의 양쪽 지역 확인 뒤 자기 영역·첫 재고·수집자 생산과 258 기록, 004139f0의 목표 1~4 선택을 순서대로 실행한다. 258 완료 판정의 본체 부분이다.</summary>
/// <remarks>2026-11-19 0.646에서 추가했다. 근거: cpppj <c>o/RawAiStrategySetup.h/.cpp</c>의 <c>RawAiStrategySetup</c>이다. 다리 생성에서 같은 게임 난수를 소비한다.</remarks>
public sealed class RawAiStrategySetup
{
    /// <summary>Player 한 기록의 바이트 수다. 0.646에서 추가했다.</summary>
    private const int PlayerBytes = 180;

    /// <summary>다리 재고의 실제 패턴 칸 수다. 0.646에서 추가했다.</summary>
    private const int BridgeSlots = 8;

    /// <summary>원본 미정 좌표 -10000.0f의 비트다. 0.646에서 추가했다.</summary>
    private const uint UnsetBits = 0xc61c4000u;

    /// <summary>목표 선택의 비용 여유다. 0.646에서 추가했다.</summary>
    private const int Reserve = 1200;

    /// <summary>기존 생성/삭제/지갑과 같은 본체 저장소다. 0.646에서 추가했다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 9Player의 현재 기술부모와 지역을 읽는다. 0.646에서 추가했다.</summary>
    private readonly RawPlayerWorld _player;

    /// <summary>공유 적 선택 몸체다. 0.646에서 추가했다.</summary>
    private readonly RawAiPreparation _preparation;

    /// <summary>공유 내용물 조건 검색이다. 0.646에서 추가했다.</summary>
    private readonly RawCarrierPreDestroy _finder;

    /// <summary>공유 실제 생산 내용물 생성이다. 0.646에서 추가했다.</summary>
    private readonly RawContainedCreate _create;

    /// <summary>순회의 현재 권한/내용물 TYPE다. 0.646에서 추가했다.</summary>
    private readonly ContainedFinderState _finderState;

    /// <summary>신전의 현재 BYTE TYPE에서 theme을 읽는다. 0.646에서 추가했다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>다른 게임 계산과 같은 원본 난수 스트림이다. 0.646에서 추가했다.</summary>
    private readonly GameRandom _random;

    /// <summary>방향별 현재 9×9 동맹표다. 0.646에서 추가했다.</summary>
    private readonly uint[] _allies;

    /// <summary>현재 원본 전역의 참조다. 0.646에서 추가했다.</summary>
    private readonly AiStrategySetupGlobals _globals;

    /// <summary>선택된 실제 월드/장치 효과다. 0.646에서 추가했다.</summary>
    private readonly AiStrategySetupHooks _hooks;

    /// <summary>AI 전략 설정 본체를 만든다. 같은 PATCH 풀과 Player/동맹 물리 계약을 확인한다.</summary>
    public RawAiStrategySetup(RawAiBrainMemory memory, RawPlayerWorld player, RawAiPreparation preparation,
        RawCarrierPreDestroy finder, RawContainedCreate create, ContainedFinderState finderState,
        IReadOnlyList<OriginalTypeRecord> types, GameRandom random, uint[] allies,
        AiStrategySetupGlobals globals, AiStrategySetupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(player);
        ArgumentNullException.ThrowIfNull(preparation);
        ArgumentNullException.ThrowIfNull(finder);
        ArgumentNullException.ThrowIfNull(create);
        ArgumentNullException.ThrowIfNull(finderState);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(allies);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (player.Pool().Edition != OriginalEdition.Patch1078
            || !ReferenceEquals(player.Pool(), preparation.Pool())
            || !ReferenceEquals(player.Pool(), finder.Pool)
            || !ReferenceEquals(player.Pool(), create.Pool)
            || player.Records().Length != 9 * PlayerBytes
            || allies.Length != 81
            || types.Count == 0)
        {
            throw new ArgumentException("AI 전략 설정 같은 PATCH·Player9·동맹81·TYPE 계약 누락", nameof(player));
        }

        _memory = memory;
        _player = player;
        _preparation = preparation;
        _finder = finder;
        _create = create;
        _finderState = finderState;
        _types = types;
        _random = random;
        _allies = allies;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>00414af0: 양쪽 지역 확인 뒤 자기 영역·첫 재고·수집자 생산과 258을 기록한다. 0.646에서 추가했다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void Initialize(uint token)
    {
        uint enemy = Field(token, 0x264);
        if (enemy == 0)
        {
            _preparation.SelectEnemy(token);
        }
        else
        {
            uint owner = Field(token, 0x260);
            if (owner >= 9 || enemy >= 9)
            {
                throw new ArgumentOutOfRangeException(nameof(token), "AI 전략 설정 방향별 동맹 범위");
            }

            if (_allies[owner * 9 + enemy] != 0)
            {
                _preparation.SelectEnemy(token);
            }
        }

        if (Field(token, 0x264) == 0 || Region(Field(token, 0x260)) == Need(_globals.NoRegion)() || Region(Field(token, 0x264)) == Need(_globals.NoRegion)())
        {
            return;
        }

        // float 경계 네 값의 비트 표현을 같은 본체에 저장한다.
        float[] bounds = Need(_hooks.RegionBounds)(Region(Field(token, 0x260)));
        for (int index = 0; index < bounds.Length; index++)
        {
            Field(token, 0x270u + unchecked((uint)index) * 4u, BitConverter.SingleToUInt32Bits(bounds[index]));
        }

        Field(token, 0x268, Field(token, 0x270));
        Field(token, 0x26c, Field(token, 0x274));
        float centerX = (float)((BitConverter.UInt32BitsToSingle(Field(token, 0x278)) - BitConverter.UInt32BitsToSingle(Field(token, 0x270))) * 0.5 + BitConverter.UInt32BitsToSingle(Field(token, 0x268)));
        Field(token, 0x268, BitConverter.SingleToUInt32Bits(centerX));
        float centerY = (float)((BitConverter.UInt32BitsToSingle(Field(token, 0x27c)) - BitConverter.UInt32BitsToSingle(Field(token, 0x274))) * 0.5 + BitConverter.UInt32BitsToSingle(Field(token, 0x26c)));
        Field(token, 0x26c, BitConverter.SingleToUInt32Bits(centerY));
        int technology = PhysicalSid(_player.Technology(Signed(Field(token, 0x260))));
        SeedBridges(token, technology, Field(token, 0x238));
        uint temple = Need(_hooks.Temple)(Region(Field(token, 0x260)));
        if (temple != 0 && FindProduct(token, Need(_globals.TempleProduct)()) == 0)
        {
            int sid = PhysicalSid(temple);
            AddProduct(token, sid, Need(_globals.TempleProduct)());
            int type = _player.Pool().Slot(sid)[10];
            if (type >= _types.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(token), "AI 초기 신전 BYTE TYPE 범위");
            }

            Field(token, 0x390, unchecked((uint)_types[type].Theme));
        }

        Field(token, 0x258, 1);
    }

    /// <summary>004139f0: 자금/기능/그래프/특정 좌표로 목표 1~4를 고른다. 0.646에서 추가했다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void SelectGoal(uint token)
    {
        uint attached = Field(token, 0x2d0);
        uint goal;
        if (attached != 0 && Signed(Field(token, 0x2d8)) > Signed(Field(token, 0x290) + unchecked((uint)Reserve)) && (Field(token, 0x248) & 4) != 0 && Field(token, 0x298) != Field(token, 0x2ac))
        {
            goal = 1;
        }
        else if (attached == 0 && (Field(token, 0x248) & 0x10) != 0)
        {
            goal = 2;
        }
        else
        {
            float x = BitConverter.UInt32BitsToSingle(Field(token, 0xaf0));
            float y = BitConverter.UInt32BitsToSingle(Field(token, 0xaf4));
            float limit = Need(_globals.MapLimit)();
            goal = x > 0 && x < limit && y > 0 && y < limit
                ? 4
                : attached != 0 && Signed(Field(token, 0x2d8)) > Signed(Field(token, 0x290) + unchecked((uint)Reserve))
                    ? 3u
                    : (Field(token, 0x248) & 0x10) != 0 ? 2u : 3u;
        }

        if (goal == Field(token, 0x2d4))
        {
            return;
        }

        Field(token, 0x3cc, 0);
        Field(token, 0x3d4, UnsetBits);
        Field(token, 0x3d0, UnsetBits);
        Field(token, 0x3d8, 1);
        Field(token, 0x5f0, 0);
        Field(token, 0x2d4, goal);
    }

    /// <summary>00410360과 004103c0은 모두 같은 mask8 조회다. 0.646에서 추가했다.</summary>
    public int FindProduct(uint token, uint type)
    {
        AssertType(type);
        int parent = PhysicalSid(_player.Technology(Signed(Field(token, 0x260))));
        return _finder.FindContained(parent, 8, type, 0);
    }

    /// <summary>00410420의 로컬/원격 생산 내용물 생성이다. 0.646에서 추가했다.</summary>
    public void AddProduct(uint token, int parent, uint type)
    {
        AssertType(type);
        if (FindProduct(token, type) != 0)
        {
            Assert("!haveProd( typeNum )");
        }

        if (FindProduct(token, type) != 0)
        {
            Assert("!haveTeleportable( typeNum )");
        }

        if (Field(token, 0x260) != Need(_globals.LocalOwner)())
        {
            int existing = FindProduct(token, type);
            if (existing != 0)
            {
                Need(_hooks.Destroy)(existing, 0);
            }

            int technology = PhysicalSid(_player.Technology(Signed(Field(token, 0x260))));
            int duplicate = _finder.FindContained(technology, 8, type, 0);
            if (duplicate != 0 && (_player.Pool().Slot(duplicate)[18] | ((uint)_player.Pool().Slot(duplicate)[19] << 8)) != 0)
            {
                Assert("!s.findContainedType( CFF_TELEPORT, typeNum )");
            }

            _create.Create(technology, type, 0, 8, 0x10, 0);
        }

        _create.Create(parent, type, 0, 0x10, 0x10, 0);
    }

    /// <summary>0040fc60의 기존 다리 스캔과 부족분 생성이다. 0.646에서 추가했다.</summary>
    public void SeedBridges(uint token, int parent, uint count)
    {
        Field(token, 0x604, count);
        uint total = ScanBridges(token, parent);
        // 하위가 현재 목표 개수를 바꾸면 다음 비교에서 다시 읽으며 unsigned 감김 뒤 signed 비교한다.
        for (uint made = total; Signed(made) < Signed(Field(token, 0x604)); made++)
        {
            AddBridge(token, parent);
        }
    }

    /// <summary>0040fcb0: 기존 재고/신전/기한·로컬 데모 관문 뒤 한 조각만 생성한다. 0.646에서 추가했다.</summary>
    public void RefreshBridges(uint token, int parent)
    {
        uint total = ScanBridges(token, parent);
        int owner = _player.Pool().Slot(parent)[34];
        if (owner >= 9)
        {
            throw new ArgumentOutOfRangeException(nameof(parent), "AI 다리 보충 현재 Player 범위");
        }

        if (ReadPlayer(unchecked((uint)owner), 0x7c) == 0 || Signed(total) >= Signed(Field(token, 0x604)))
        {
            return;
        }

        double due = BitConverter.UInt64BitsToDouble(((ulong)Field(token, 0x634) << 32) | Field(token, 0x630));
        if (!(Need(_globals.Now)() >= due))
        {
            if (unchecked((uint)owner) != Need(_globals.LocalOwner)() || !Need(_hooks.Demo)())
            {
                return;
            }
        }

        AddBridge(token, parent);
    }

    /// <summary>후속 실제 월드 조립 확인용 동일 풀이다. 0.646에서 추가했다.</summary>
    public RawSidPool Pool() => _player.Pool();

    /// <summary>다른 모듈과 같은 살아 있는 본체만 허용한다. 0.646에서 추가했다.</summary>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 전략 설정 실제0xb08 본체 누락", nameof(token));
        }

        return raw;
    }

    /// <summary>하위 효과 이후에도 본체를 다시 조회한다. 0.646에서 추가했다.</summary>
    private uint Field(uint token, uint offset) => BinaryPrimitives.ReadUInt32LittleEndian(Body(token).AsSpan((int)offset, 4));

    /// <summary>현재 본체의 선택된 필드만 갱신한다. 0.646에서 추가했다.</summary>
    private void Field(uint token, uint offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Body(token).AsSpan((int)offset, 4), value);

    /// <summary>축소 이전에 실제 풀 경계를 검사하며 0도 원본 getter 입력으로 보존한다. 0.646에서 추가했다.</summary>
    private int PhysicalSid(uint value)
    {
        if (value >= (uint)_player.Pool().Capacity || value > ushort.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 전략 설정 SID 범위");
        }

        return unchecked((int)value);
    }

    /// <summary>별도 지역 사본을 만들지 않는다. 0.646에서 추가했다.</summary>
    private int Region(uint owner)
    {
        if (owner >= 9)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 전략 설정 현재 Player 범위");
        }

        return Signed(ReadPlayer(owner, 0x78));
    }

    /// <summary>같은 현재 Player 기록 DWORD를 읽는다. 0.646에서 추가했다.</summary>
    private uint ReadPlayer(uint owner, uint offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(_player.Records().AsSpan((int)owner * PlayerBytes + (int)offset, 4));

    /// <summary>원본 진단이 반환하면 다음 원문 연산으로 진행한다. 0.646에서 추가했다.</summary>
    private void Assert(string text) => Need(_hooks.Assertion)(text);

    /// <summary>범위가 잘못된 타입도 진단 장치 호출 자체는 먼저 보존한다. 0.646에서 추가했다.</summary>
    private void AssertType(uint type)
    {
        if (Signed(type) < 1 || _types.Count <= type)
        {
            Assert("typeNum > 0 && typeNum < numTypes");
        }
    }

    /// <summary>손상된 재고가 원본 고정 배열 뒤의 상태를 덮기 전에 물리 계약 오류로 알린다. 0.646에서 추가했다.</summary>
    private void StoreBridge(uint token, uint pattern)
    {
        int index = Signed(Field(token, 0x628));
        if (index < 0 || index >= BridgeSlots)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "AI 실제 다리 재고8칸 초과");
        }

        Field(token, 0x608u + unchecked((uint)index) * 4u, pattern);
    }

    /// <summary>원본은 직선 조각을 강제할 때에도 난수/가중 추첨을 먼저 소비한다. 0.646에서 추가했다.</summary>
    private void AddBridge(uint token, int parent)
    {
        uint pattern = unchecked((uint)CanonDecoder.SelectBridgePattern(unchecked((int)_random.Next(10000)), OriginalEdition.Patch1078));
        StoreBridge(token, pattern);
        uint serial = Field(token, 0x5f4);
        Field(token, 0x5f4, serial + 1u);
        if (Signed(serial) % 4 == 0)
        {
            StoreBridge(token, 0);
        }

        int index = Signed(Field(token, 0x628));
        _create.Create(parent, Need(_globals.BridgeType)(), Field(token, 0x608u + unchecked((uint)index) * 4u), 0x40, 0x10, 0);
        Field(token, 0x628, Field(token, 0x628) + 1u);
        double deadline = BitConverter.UInt32BitsToSingle(Field(token, 0x600)) + Need(_globals.Now)();
        ulong bits = BitConverter.DoubleToUInt64Bits(deadline);
        Field(token, 0x630, unchecked((uint)bits));
        Field(token, 0x634, unchecked((uint)(bits >> 32)));
    }

    /// <summary>사용 중인 다리도 총수에는 포함하고, 배치 가능한 패턴만 8칸 재고에 모은다. 0.646에서 추가했다.</summary>
    private uint ScanBridges(uint token, int parent)
    {
        Field(token, 0x628, 0);
        uint total = 0;
        var cursor = new RawContainedFinder(_player.Pool(), _finderState);
        // 저장된 next를 이용하는 같은 원본 내용물 순회로 현재 다리 상태를 읽는다.
        for (int sid = cursor.Begin(parent); sid != 0; sid = cursor.Next())
        {
            ReadOnlySpan<byte> raw = _player.Pool().Slot(sid);
            uint flags = raw[20] | ((uint)raw[21] << 8);
            if ((flags & 0x40) == 0)
            {
                continue;
            }

            total++;
            if ((flags & 0x140) == 0x40)
            {
                StoreBridge(token, raw[12] | ((uint)raw[13] << 8));
                Field(token, 0x628, Field(token, 0x628) + 1u);
            }
        }

        return total;
    }

    /// <summary>DWORD의 signed 분기에서 감김을 정의된 비트 해석으로 보존한다. 0.646에서 추가했다.</summary>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>선택된 하위만 필요로 하며 누락된 효과를 빈 성공으로 처리하지 않는다. 0.646에서 추가했다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 전략 설정의 선택된 실제 하위 미연결");
}
