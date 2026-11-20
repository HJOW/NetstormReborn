using System.Buffers.Binary;

// 연구·업그레이드·주문 효과의 원본 플레어 생성과 정기 수명을 처리한다.
namespace Netstorm.Core.Original;

/// <summary>같은 월드의 현재 억제 시각·로딩 깊이·효과 TYPE을 조회한다.</summary>
public sealed class FlareState
{
    /// <summary>생성 관문에서만 읽는 현재 억제 시각이다.</summary>
    public Func<double>? SuppressedTime { get; set; }
    /// <summary>현재 로딩 깊이다.</summary>
    public Func<uint>? LoadingDepth { get; set; }
    /// <summary>기본 생성 wrapper의 현재 flare TYPE다.</summary>
    public Func<uint>? Type { get; set; }
    /// <summary>현재 Native 입력은 원문에 없는 할당·가상표 사전 검사를 생략한다.</summary>
    public bool Native { get; set; }
}

/// <summary>실제 생성·표시·수명·예약·소리를 같은 월드에 연결한다.</summary>
public sealed class FlareHooks
{
    /// <summary>현재 signed Kernel 프로세스 수다.</summary>
    public Func<int>? ProcessCount { get; set; }
    /// <summary>현재 TYPE·클라이언트 flags로 객체를 생성한다.</summary>
    public Func<uint, uint, int>? Create { get; set; }
    /// <summary>부모 SHP 추가 헤더의 실제 signed 기준점 두 값이다. 폭/높이와 구별한다.</summary>
    public Func<int, (short X, short Y)>? SourceHotspot { get; set; }
    /// <summary>현재 객체의 실제 소유자를 지정한다.</summary>
    public Action<int, uint>? Owner { get; set; }
    /// <summary>같은 월드의 공간·표시에 등록한다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>각 Regular 예약의 독립 성공 여부다.</summary>
    public Func<bool>? ReserveRegular { get; set; }
    /// <summary>성공한 예약에 원본 사건·payload를 주어 부착한다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }
    /// <summary>실제 프레임·표시를 갱신한다.</summary>
    public Action<int, int, uint>? Frame { get; set; }
    /// <summary>마지막 그림을 실제 갱신한다.</summary>
    public Action<int>? Update { get; set; }
    /// <summary>같은 프로세스·공간·풀 수명으로 실제 삭제한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>B구간 중간의 powerup.wav 위치 소리를 재생한다.</summary>
    public Action<float, float, string, uint>? Sound { get; set; }
}

/// <summary>0045a6c0→0045a480 생성과 0045a250 정기를 같은 raw 슬롯·TYPE·난수에서 실행한다.</summary>
/// <remarks>근거: cpppj/src/o/RawFlare.cpp·h. 2026-11-20 0.669에서 실제 업그레이드 효과의 선행 본체로 이식했다.</remarks>
public sealed class RawFlare
{
    /// <summary>flare106 생성자의 실제 가상표다.</summary>
    private const uint FlareVtable = 0x5096a0;
    /// <summary>현재 PATCH TYPE·프레임 표 폭이다.</summary>
    private const int TypeCount = 188;
    /// <summary>원본 PE00504194의 정확한 float Y 배율이다.</summary>
    private const float YScale = 0.09090909361839294f;
    /// <summary>현재 효과·부모가 공유하는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>부모의 현재 genus 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 frame code·first/count 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>난수를 실제 소비할 때 조회하는 같은 장기 owner다.</summary>
    private readonly Func<GameRandom>? _random;
    /// <summary>현재 전역의 조회 장치다.</summary>
    private readonly FlareState _state;
    /// <summary>선택된 실제 하위 효과다.</summary>
    private readonly FlareHooks _hooks;

    /// <summary>다른 게임 계산과 같은 난수 객체를 빌린다. 구성 자체는 난수를 소비하지 않는다.</summary>
    public RawFlare(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames,
        GameRandom random, FlareState state, FlareHooks hooks) : this(pool, types, frames, () => random, state, hooks)
    {
        ArgumentNullException.ThrowIfNull(random);
    }

    /// <summary>같은 PATCH 표 계약만 확인하며 선택되지 않은 난수·전역은 요구하지 않는다.</summary>
    public RawFlare(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames,
        Func<GameRandom>? random, FlareState state, FlareHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != TypeCount || frames.Count != TypeCount)
        {
            throw new ArgumentException("플레어 PATCH 풀/TYPE/프레임 오류");
        }
        _pool = pool;
        _types = types;
        _frames = frames;
        _random = random;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>실제 flare 가상표만 처리한다.</summary>
    public bool Handles(int sid) => Read(sid, 0) == FlareVtable;
    /// <summary>현재 기본 TYPE의 A구간으로 지정한 개수의 효과를 생성한다.</summary>
    public void Spawn(int parent, int count, uint ev) => SpawnDirection(parent, count, ev, Need(_state.Type)(), 0);

    /// <summary>같은 생성 본체의 지정 TYPE·방향을 사용한다. C/D 주문 방향도 이 경로다.</summary>
    public void SpawnDirection(int parent, int count, uint ev, uint effectType, uint direction)
    {
        if (Need(_state.SuppressedTime)() != 0 || Need(_state.LoadingDepth)() != 0)
        {
            return;
        }
        if (direction >= 16)
        {
            throw new ArgumentOutOfRangeException(nameof(direction), "플레어 생성 방향 범위");
        }
        byte depth = _pool.Slot(parent)[35];
        int size = Need(_hooks.ProcessCount)();
        // 현재 Kernel 개수의 로컬 사본을 하나씩 증가시키며 음수·0개는 생성하지 않는다.
        for (int index = 0; index < count; index++)
        {
            if (size > 30000) return;
            int child = Need(_hooks.Create)(effectType, 2);
            float x = Float(parent, 14), y = Float(parent, 18);
            (short hx, short hy) = Need(_hooks.SourceHotspot)(parent);
            uint type = _pool.Slot(parent)[10];
            Type(type);
            bool shifted = (_types[(int)type].Flags2 & 0x30000u) != 0;
            float offsetX = shifted ? -0.7f : 0f, offsetY = shifted ? -0.1f : 0f;
            float highX = (float)(hx * 0.0625 + (double)offsetX);
            float highY = (float)(hy * (double)YScale + offsetY);
            double randomX = Random(offsetX, highX), randomY = Random(offsetY, highY);
            uint childType = _pool.Slot(child)[10];
            Type(childType);
            Put(child, 36, unchecked((uint)_frames[(int)childType].Run((byte)('A' + direction)).First));
            Need(_hooks.Owner)(child, 0);
            Span<byte> raw = Bytes(child);
            raw[40] |= 8;
            raw[35] = unchecked((byte)(depth - 1));
            float spawnX = Clamp((float)(x - randomX)), spawnY = Clamp((float)(y - randomY));
            Need(_hooks.Pop)(child, spawnX, spawnY, 0);
            if (Need(_hooks.ReserveRegular)())
            {
                Need(_hooks.AddRegular)(child, ev, (float)(Random(0, 0.07f) + 0.05));
            }
            size++;
        }
    }

    /// <summary>주기·후속 플레어·마지막 갱신/삭제·현재 프레임·B구간 소리를 원문 순서로 실행한다.</summary>
    public float Handle(int sid, uint ev, uint count, float payload)
    {
        if (!_state.Native && !Handles(sid))
        {
            throw new ArgumentException("플레어 현재 가상 표 오류", nameof(sid));
        }
        RiftTypeFrames.LetterRun run = Run(sid);
        int step = unchecked((int)count), kind = unchecked((int)ev);
        if (step == 0) payload = (float)Random(0.05f, 0.13f);
        if (kind > 1 && step == unchecked(run.Count - 1))
        {
            if (Need(_hooks.ProcessCount)() > 30000) return 0f;
            int child = Need(_hooks.Create)(_pool.Slot(sid)[10], 2);
            float x = Integer(Float(sid, 14)), y = Integer(Float(sid, 18));
            double randomX = Random(-0.4f, 0.4f), randomY = Random(-0.4f, 0.4f);
            float childY = (float)(randomY + y);
            Put(child, 36, unchecked((uint)run.First));
            Need(_hooks.Pop)(child, (float)(randomX + x), childY, 0);
            if (Need(_hooks.ReserveRegular)()) Need(_hooks.AddRegular)(child, unchecked(ev - 1), 0);
        }
        if (step >= run.Count)
        {
            Need(_hooks.Update)(sid);
            Need(_hooks.Destroy)(sid, 0);
            return -1f;
        }
        Need(_hooks.Frame)(sid, unchecked((int)((uint)run.First + count)), 0);
        if (step == run.Count / 2 && RawPathAnimation.Direction(_frames[_pool.Slot(sid)[10]], unchecked((int)Read(sid, 36))) == 1)
        {
            Need(_hooks.Sound)(Float(sid, 14), Float(sid, 18), "powerup.wav", 0);
        }
        return payload;
    }

    /// <summary>다른 실제 부모는 이전 처리기에 넘기며 누락은 명시 오류다.</summary>
    public RegularHandler Handler(RegularHandler? fallback = null) => (sid, ev, count, payload) =>
        Handles(sid) ? Handle(sid, ev, count, payload) : Need(fallback)(sid, ev, count, payload);
    /// <summary>실제 월드 조립 시 같은 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>현재 프레임의 방향 글자에서 실제 first/count를 조회한다.</summary>
    private RiftTypeFrames.LetterRun Run(int sid)
    {
        uint type = _pool.Slot(sid)[10];
        Type(type);
        int direction = RawPathAnimation.Direction(_frames[(int)type], unchecked((int)Read(sid, 36)));
        if (direction < 0 || direction >= 16) throw new ArgumentOutOfRangeException(nameof(sid), "플레어 현재 프레임 방향 범위");
        return _frames[(int)type].Run((byte)('A' + direction));
    }
    /// <summary>입력 float 차이를 넓혀 절댓값과 현재 난수 상위16비트를 원본 순서로 곱한다.</summary>
    private double Random(float low, float high) => Need(_random)().Next(65536) * (1.0 / 65536.0) * Math.Abs((double)high - low) + low;
    /// <summary>typed의 할당 검사와 Native의 물리 슬롯 직접 쓰기를 구별한다.</summary>
    private Span<byte> Bytes(int sid) => _state.Native ? _pool.RawSlot(sid) : _pool.AllocatedBytes(sid);
    /// <summary>원본 비정렬 DWORD만 읽는다.</summary>
    private uint Read(int sid, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid).Slice(offset, 4));
    /// <summary>원본 직접 프레임 쓰기는 별도 SquidFrame 통지를 호출하지 않는다.</summary>
    private void Put(int sid, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Bytes(sid).Slice(offset, 4), value);
    /// <summary>저장된 float의 비트를 그대로 읽는다.</summary>
    private float Float(int sid, int offset) => BitConverter.UInt32BitsToSingle(Read(sid, offset));
    /// <summary>현재 TYPE 범위를 확인하며 TYPE0도 허용한다.</summary>
    private void Type(uint type)
    {
        if (type >= (uint)_frames.Count) throw new ArgumentOutOfRangeException(nameof(type), "플레어 현재 TYPE 범위");
    }
    /// <summary>실제 fadd0.5→signed64 ftol→signed DWORD→float 저장 순서다.</summary>
    private static float Integer(float input)
    {
        double value = (double)input + 0.5;
        return !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0
            ? 0f : unchecked((int)(uint)(long)value);
    }
    /// <summary>NaN은 유지하고 유한 좌표에만 원문 1..255 경계를 적용한다.</summary>
    private static float Clamp(float value)
    {
        if (value < 1f) value = 1f;
        if (255f < value) value = 255f;
        return value;
    }
    /// <summary>선택된 미연결만 실제 호출 위치에서 거부한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("플레어 실제 하위 미연결");
}
