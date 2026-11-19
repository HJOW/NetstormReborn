// 원본 AI 전략 말미의 같은 지갑·SP 정리004138c0을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 경제 모듈을 담는 네임스페이스다. 2026-11-19 0.658에서 R4-5 AI 경제용으로 추가했다.

// 원본이 각 분기에서 현재 다시 조회하는 미션/사제 상태다.
public sealed class AiEconomyHooks
{
    /// <summary>현재 데모 여부다. 0.658에서 추가했다.</summary>
    public Func<bool>? Demo { get; set; }

    /// <summary>현재 참여 플래그와 사제 ownerState1이다. 0.658에서 추가했다.</summary>
    public Func<uint, bool>? Captured { get; set; }
}

/// <summary>별도 현금 사본이나 보상 가산 없이 같은 본체+2d8과 원본 분산 SP 저장소를 동기화한다.</summary>
/// <remarks>2026-11-19 0.658에서 추가했다. 근거: cpppj <c>o/RawAiEconomy.h/.cpp</c>의 <c>RawAiEconomy</c>다.</remarks>
public sealed class RawAiEconomy
{
    /// <summary>0x898 예산이다. 0.658에서 추가했다.</summary>
    private const int Budget = 2200;

    /// <summary>0x4a6 데모 하한이다. 0.658에서 추가했다.</summary>
    private const int Floor = 1190;

    /// <summary>지원 간격이다. 0.658에서 추가했다.</summary>
    private const double Period = 1.0;

    /// <summary>생성/통지/보상과 같은 본체 저장소다. 0.658에서 추가했다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 게임 난수/추적 상태를 사용하는 실제 SP 저장소다. 0.658에서 추가했다.</summary>
    private readonly RawSpStore _store;

    /// <summary>현재 QWORD 시각이다. 0.658에서 추가했다.</summary>
    private readonly Func<double> _now;

    /// <summary>각 원문 위치에서 호출하는 현재 미션/사제 getter다. 0.658에서 추가했다.</summary>
    private readonly AiEconomyHooks _hooks;

    /// <summary>AI 경제 본체를 만든다. 같은 실제 저장소와 현재 시각을 빌린다.</summary>
    public RawAiEconomy(RawAiBrainMemory memory, RawSpStore store, Func<double> now, AiEconomyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(store);
        ArgumentNullException.ThrowIfNull(now);
        ArgumentNullException.ThrowIfNull(hooks);
        _memory = memory;
        _store = store;
        _now = now;
        _hooks = hooks;
    }

    /// <summary>004138c0: 1초 지원/포획 예산·데모 하한 뒤 매번 실제 SP Set이다. 0.658에서 추가했다.</summary>
    public void Reconcile(uint token)
    {
        double due = BitConverter.UInt64BitsToDouble(((ulong)Field(token, 0x5dc) << 32) | Field(token, 0x5d8));
        if (Need(_now)() >= due)
        {
            ulong next = BitConverter.DoubleToUInt64Bits(Need(_now)() + Period);
            Field(token, 0x5d8, unchecked((uint)next));
            Field(token, 0x5dc, unchecked((uint)(next >> 32)));
            if (Signed(Field(token, 0x2d8)) < Signed(Field(token, 0x290) + unchecked((uint)Budget)))
            {
                Field(token, 0x2d8, unchecked(Field(token, 0x24c) + Field(token, 0x2d8)));
                DemoFloor(token);
            }
        }

        CaptureBudget(token, Field(token, 0x264));
        CaptureBudget(token, Field(token, 0x260));
        float wallet = Signed(Field(token, 0x2d8));
        _store.Set(Field(token, 0x260), BitConverter.SingleToUInt32Bits(wallet));
    }

    /// <summary>후속 실제 월드 조립 확인용 동일 풀이다. 0.658에서 추가했다.</summary>
    public RawSpStore Store => _store;

    /// <summary>getter의 효과가 바꾼 현재 지갑을 원본 저장 포인터에서 다시 읽는다. 0.658에서 추가했다.</summary>
    private void DemoFloor(uint token)
    {
        if (Need(_hooks.Demo)())
        {
            uint wallet = Field(token, 0x2d8);
            Field(token, 0x2d8, Signed(wallet) > Floor ? wallet : unchecked((uint)Floor));
        }
    }

    /// <summary>포획 플래그만 먼저 판정하고 반환 뒤 비용/지갑을 현재 다시 읽는다. 0.658에서 추가했다.</summary>
    private void CaptureBudget(uint token, uint owner)
    {
        if (Need(_hooks.Captured)(owner))
        {
            uint target = Field(token, 0x290) + unchecked((uint)Budget);
            if (Signed(Field(token, 0x2d8)) < Signed(target))
            {
                Field(token, 0x2d8, target);
                DemoFloor(token);
            }
        }
    }

    /// <summary>가짜 핸들이나 작은 본체를 다른 저장소로 대신하지 않는다. 0.658에서 추가했다.</summary>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 경제 실제0xb08 본체 누락", nameof(token));
        }

        return raw;
    }

    /// <summary>현재 본체의 지정 필드를 다시 읽는다. 0.658에서 추가했다.</summary>
    private uint Field(uint token, uint offset) => BinaryPrimitives.ReadUInt32LittleEndian(Body(token).AsSpan((int)offset, 4));

    /// <summary>현재 본체의 지정 필드만 쓴다. 0.658에서 추가했다.</summary>
    private void Field(uint token, uint offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Body(token).AsSpan((int)offset, 4), value);

    /// <summary>원본 signed 비교를 구현 의존 변환 없이 적용한다. 0.658에서 추가했다.</summary>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>선택되지 않은 하위는 요구하지 않고 선택된 누락은 명시 오류로 남긴다. 0.658에서 추가했다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 경제 실제 하위 미연결");
}
