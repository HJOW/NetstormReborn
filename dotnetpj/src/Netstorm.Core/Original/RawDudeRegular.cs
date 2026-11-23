using System.Buffers.Binary;

// Dude의 실제 정기 분배·보행·복귀·드롭·낙하를 같은 현재 월드에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawDudeRegular.cpp의 10.78 450fa0/451190/451210/451270/451340/4508b0 몸체다.</summary>
public sealed class RawDudeRegular
{
    /// <summary>10.78의 push1e 실제 capacity다.</summary>
    public const int PopulationCapacity = 30;
    /// <summary>부모·목표·실제 종속의 유일한 저장소다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 타입의 코드/Run 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>게임 전역의 같은 32비트 난수를 소비 위치에서 조회한다.</summary>
    private readonly Func<GameRandom>? _random;
    /// <summary>같은 목록/순환/진단 전역이다.</summary>
    private readonly DudeRegularState _state;
    /// <summary>실제 선택한 하위 효과다.</summary>
    private readonly DudeRegularHooks _hooks;
    /// <summary>누락된 원본 효과는 선택한 시점에만 진단한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Dude 정기 실제 효과 미연결");
    /// <summary>비정렬 raw의 지정 폭을 little endian으로 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 주변 필드를 합치거나 호스트 포인터로 해석하지 않는다.
        for (int index = 0; index < width; index++) value |= (uint)raw[offset + index] << (8 * index);
        return value;
    }
    /// <summary>원본 초기 생성자와 같은 capacity/count이며 이 소유권은 Session 수명에 묶인다. 배열의 미사용 heap 값은 게임 상태가 아니다.</summary>
    public static void InitializeDudePopulation(RawPostPopList population)
    {
        ArgumentNullException.ThrowIfNull(population);
        if (population.Entries.Length != 0 || population.Count != 0) throw new InvalidOperationException("Dude population 중복 초기화");
        population.Entries = new uint[PopulationCapacity];
    }
    /// <summary>같은 프레임/난수/현재 목록을 빌린다. Native는 장치/목록을 첫 선택 전 조회하지 않는다.</summary>
    public RawDudeRegular(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, GameRandom random, DudeRegularState state, DudeRegularHooks hooks)
        : this(pool, frames, () => random, state, hooks)
    {
    }
    /// <summary>실제 Native 난수 owner를 소비 위치에서만 조회한다.</summary>
    public RawDudeRegular(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, Func<GameRandom>? random, DudeRegularState state, DudeRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || frames.Count != 188 || (random is null && !state.Native)) throw new ArgumentException("Dude 정기 판본/프레임 연결 오류", nameof(frames));
        if (!state.Native) _ = state.Providers.Items();
        _pool = pool; _frames = frames; _random = random; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>현재 실제 난수를 쓰고 0범위는 원본 나눗셈 거부로 드러낸다.</summary>
    private uint RandomNext(uint limit)
    {
        GameRandom random = Need(_random)(); if (limit == 0 && _state.Native) { _ = random.Next(1); throw new DivideByZeroException("Dude 원문 난수0범위 나눗셈"); }
        return random.Next(limit);
    }
    /// <summary>가상+5c=450fa0인 실제 부모만 이 분배에 들어온다.</summary>
    public bool Handles(ushort sid) { ReadOnlySpan<byte> raw = _pool.Slot(sid); return raw[10] == 70 && Read(raw, 0) == 0x508cc8U; }
    /// <summary>free는 Handle의 원래 fallback 관문이 읽으므로 여기서 제거하지 않는다.</summary>
    private ReadOnlySpan<byte> Raw(ushort sid)
    {
        if (_state.Native) return _pool.Slot(sid);
        if (sid < 5 || sid >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 정기 SID 범위");
        ReadOnlySpan<byte> raw = _pool.Slot(sid); if (raw[10] >= _frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 정기 현재 타입 범위");
        return raw;
    }
    /// <summary>효과 호출 뒤 현재 위치를 읽을 때도 같은 raw만 사용한다.</summary>
    private RawPathPoint Position(ushort sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        return new(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]));
    }
    /// <summary>signed side의 의미와 범위 진단은 기존 원본 프레임 조회를 재사용한다.</summary>
    private int Stage(ushort sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        if (raw[10] >= _frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 현재 단계 TYPE 물리 범위");
        return RawPathAnimation.Direction(_frames[raw[10]], BinaryPrimitives.ReadInt32LittleEndian(raw[36..]));
    }
    /// <summary>WORD task와 BYTE35 및 frame DWORD의 이웃 바이트를 보존한다.</summary>
    private void Write(ushort sid, int offset, uint value, int width)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        // 원본 폭만 낮은 바이트부터 쓴다.
        for (int index = 0; index < width; index++) raw[offset + index] = (byte)(value >> (8 * index));
    }
    /// <summary>진단 callback이 복귀하면 원문처럼 목표 진입점과 이동 호출을 계속한다.</summary>
    public bool MoveTo(ushort sid, ushort target, bool quickly)
    {
        if (target == 0) return false;
        if ((Raw(target)[11] & 3U) != 0) Need(_hooks.Assertion)(0xde);
        return Need(_hooks.Move)(sid, Need(_hooks.Entrance)(target), quickly);
    }
    /// <summary>난수 호출 순서를 바꾸지 않고 원문 공급자 active 목록의 현재 항목을 선택한다.</summary>
    public float Wander(ushort sid, int count, bool quickly)
    {
        if (_state.Native)
        {
            if (count == 0 || RandomNext(100) >= 10)
            {
                if (_state.Providers.Count != 0)
                {
                    uint step = RandomNext(1) + 1U; int next = unchecked((int)(unchecked((uint)_state.ProviderIndex) + step)); int divisor = unchecked((int)_state.Providers.Count);
                    if (divisor == 0 || (next == int.MinValue && divisor == -1)) throw new DivideByZeroException("Dude 원문 공급원 IDIV 범위");
                    _state.ProviderIndex = next % divisor; Span<uint> storage = _state.Providers.Storage;
                    if (_state.ProviderIndex < 0 || _state.ProviderIndex >= storage.Length) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 현재 공급원 색인 물리 범위");
                    uint value = storage[_state.ProviderIndex]; if (value >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 현재 공급원 SID 물리 범위");
                    if (MoveTo(sid, (ushort)value, quickly)) return 10.0F;
                }
            }
            Need(_hooks.Task)(sid, 0x5103, 0); return 0.1F;
        }
        if (count == 0 || RandomNext(100) >= 10)
        {
            ReadOnlySpan<uint> providers = _state.Providers.Items();
            if (!providers.IsEmpty)
            {
                // 원문4558f0(1,2)는 상한 제외라 간격은1이며 난수는 한 번 소비한다.
                int next = unchecked((int)(unchecked((uint)_state.ProviderIndex) + RandomNext(1) + 1U));
                _state.ProviderIndex = next % providers.Length;
                if (_state.ProviderIndex < 0) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 공급자 순환 음수 색인");
                uint number = providers[_state.ProviderIndex];
                if (number > 0xffffU) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 공급자 SID 폭");
                if (MoveTo(sid, (ushort)number, quickly)) return 10.0F;
            }
        }
        Need(_hooks.Task)(sid, 0x5103, 0); return 0.1F;
    }
    /// <summary>path 인자는 원래 전달되지만 451210 몸체에서는 읽지 않는다.</summary>
    public float Return(ushort sid, ushort path, int count)
    {
        if (count > 4) { Need(_hooks.Destroy)(sid, 0); return -1.0F; }
        return MoveTo(sid, Need(_hooks.NearestResidence)(sid), false) ? 10.0F : 2.0F;
    }
    /// <summary>원래 위치는 공간 효과 전에 보존하고 이후 현재 프레임/가상 Pop은 기존 월드가 처리한다.</summary>
    public float Drop(ushort sid, ushort path, int count)
    {
        if (count != 0) { if (_state.Checking && path == 0) Need(_hooks.Assertion)(0x127); return 1.0F; }
        RawPathPoint origin = Position(sid); ushort residence = Need(_hooks.NearestResidence)(sid);
        if (residence != 0) { Need(_hooks.Unpop)(sid, 0); RawPathPoint point = Need(_hooks.Entrance)(residence); Need(_hooks.Pop)(sid, point, 0); if (Need(_hooks.Move)(sid, origin, false)) return 1.0F; }
        Need(_hooks.Destroy)(sid, 0); return -1.0F;
    }
    /// <summary>원본의 fild→곱/합→float 저장과 unordered y 상한을 같은 순서로 처리한다.</summary>
    public float Fall(ushort sid, int count)
    {
        if ((Raw(sid)[11] & 2U) != 0) return 0; if (count > 100) { Need(_hooks.Destroy)(sid, 0); return -1.0F; }
        if (count == 10) Write(sid, 35, 20, 1);
        if (Stage(sid) == 8) { byte type = Raw(sid)[10]; Need(_hooks.SetFrame)(sid, _frames[type].Run((byte)'A').First, 0); }
        RawPathPoint point = Position(sid); float y = (float)((count * 0.5 + 1.0) * 0.0625 + point.Y); if (254.0F < y) y = 254.0F;
        Need(_hooks.Unpop)(sid, 0); int direction = Stage(sid); if (direction < 0 || direction > 15) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 낙하 단계 범위");
        RiftTypeFrames.LetterRun run = _frames[Raw(sid)[10]].Run((byte)('A' + direction));
        if (!_state.Native && run.Count <= 0) throw new ArgumentOutOfRangeException(nameof(sid), "Dude 낙하 현재 Run0");
        uint frame = unchecked((uint)run.First) + RandomNext((uint)run.Count); Write(sid, 36, frame, 4); Need(_hooks.Pop)(sid, new(point.X, y), 0x40); return 0.05F;
    }
    /// <summary>원본의 poll 우선·free/마스크 fallback·낙하 우선·경로 대기·task 쓰기 순서다.</summary>
    public float Handle(ushort sid, uint @event, uint count, float payload)
    {
        if (!_state.Native && !Handles(sid)) throw new InvalidOperationException("Dude 정기 실제 타입/가상 표 오류");
        if (@event == _state.CurrentPollEvent()) return 0;
        if ((Raw(sid)[11] & 1U) != 0 || (@event & 0xfff0U) != 0x5100U) { if (_state.Checking) Need(_hooks.Assertion)(399); return payload; }
        int signedCount = unchecked((int)count);
        if (@event != 0x5105 && Need(_hooks.CheckSpace)(sid)) return Fall(sid, signedCount);
        ushort path = Need(_hooks.FindPath)(sid, _state.CurrentPathType());
        if (path != 0 && count != 0 && @event != 0x5107) return payload;
        if (_state.Native) Need(_hooks.SetWord)(sid, @event); else Write(sid, 12, @event, 2);
        switch (@event)
        {
            case 0x5101: Need(_hooks.Assertion)(0x179); return Wander(sid, signedCount, false);
            case 0x5102: return Wander(sid, signedCount, false);
            case 0x5103: return Return(sid, path, signedCount);
            case 0x5104: return Drop(sid, path, signedCount);
            case 0x5105: return Fall(sid, signedCount);
            case 0x5106:
            case 0x5107:
                if (signedCount > 10) { Need(_hooks.Destroy)(sid, 0); return -1.0F; }
                if (@event == 0x5107 && path == 0) _ = Wander(sid, 0, true);
                return 0.2F;
            default: Need(_hooks.Assertion)(0x185); return payload;
        }
    }
    /// <summary>composition root의 같은 풀 연결을 확인한다.</summary>
    public RawSidPool Pool => _pool;
}
