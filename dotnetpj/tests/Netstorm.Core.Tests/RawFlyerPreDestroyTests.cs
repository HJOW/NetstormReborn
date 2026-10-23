using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 실제 플라이어 사전 파괴(<see cref="RawFlyerPreDestroy"/>)의 경계 계약을 cpppj FlyerPreDestroyTests.cpp 의
/// 변형과 원본 순서로 확인한다. +8 을 WORD 로만 읽어 대상이 타입·상태 바이트까지 번지지 않는지,
/// 대상이 0 이면 가상 삭제를 하지 않는지, callback 이 raw 를 바꿔도 Damageable 에 원래 this·flags 를 넘기는지 본다.
/// 기준 판본은 10.78 이다 (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class RawFlyerPreDestroyTests
{
    /// <summary>검사 대상 플라이어 번호.</summary>
    private const int Source = 50;

    /// <summary>10.78 플라이어 클래스의 가상 표 값(원본 관찰 입력).</summary>
    private const uint PatchFlyerVtable = 0x12400000;

    /// <summary>원본 관찰이 플라이어에 넣는 타입 번호.</summary>
    private const byte FlyerType = 144;

    /// <summary>+8 WORD 만 지우고 나머지는 그대로 두는 훅으로 순서를 확인한다.</summary>
    [Fact]
    public void ReadsOnlyWordAndKeepsOriginalArguments()
    {
        var pool = new RawSidPool(32768, false);
        Prepare(pool, 0x0102);
        var parts = new List<string>();
        var body = new RawFlyerPreDestroy(pool, new FlyerPreDestroyHooks
        {
            Destroy = (sid, flags) =>
            {
                // 4바이트로 읽으면 타입·상태가 섞여 0x02019002 가 된다
                Assert.Equal(0x0102, sid);
                Assert.Equal(0u, flags);
                parts.Add("D:" + sid);
                // callback 이 대상 WORD 와 상태를 바꿔도 하위 호출 인자는 원본 그대로다
                PutWord(pool.RawSlot(Source), 8, 61);
                pool.RawSlot(Source)[11] = 1;
                parts.Add(";");
            },
            Damageable = (sid, flags) =>
            {
                Assert.Equal(Source, sid);
                Assert.Equal(0x200000u, flags);
                parts.Add("P:" + sid + ":" + flags);
            },
        });
        body.PreDestroy(Source, 0x200000);
        Assert.Equal("D:258;P:50:2097152", string.Concat(parts));
    }

    /// <summary>대상이 0 이면 가상 삭제 없이 Damageable 만 부른다.</summary>
    [Fact]
    public void SkipsDestroyWhenTargetIsZero()
    {
        var pool = new RawSidPool(32768, false);
        Prepare(pool, 0);
        var parts = new List<string>();
        var body = new RawFlyerPreDestroy(pool, new FlyerPreDestroyHooks
        {
            Destroy = (sid, _) => parts.Add("D:" + sid),
            Damageable = (sid, flags) => parts.Add("P:" + sid + ":" + flags),
        });
        body.PreDestroy(Source, 7);
        Assert.Equal("P:50:7", string.Concat(parts));
    }

    /// <summary>두 하위 경계가 모두 없으면 첫 삭제 전에 거부한다.</summary>
    [Fact]
    public void RejectsMissingHooks()
    {
        var pool = new RawSidPool(32768, false);
        Prepare(pool, 60);
        Assert.Throws<ArgumentException>(() => new RawFlyerPreDestroy(pool, new FlyerPreDestroyHooks
        {
            Damageable = (_, _) => { },
        }));
        Assert.Throws<ArgumentException>(() => new RawFlyerPreDestroy(pool, new FlyerPreDestroyHooks
        {
            Destroy = (_, _) => { },
        }));
    }

    /// <summary>슬롯을 원본 관찰과 같은 초기값으로 되돌린다.</summary>
    /// <param name="pool">대상 풀.</param>
    /// <param name="target">+8 에 넣을 대상 WORD.</param>
    private static void Prepare(RawSidPool pool, uint target)
    {
        Span<byte> raw = pool.RawSlot(Source);
        raw.Clear();
        Put(raw, 0, PatchFlyerVtable);
        raw[10] = FlyerType;
        raw[11] = 2;
        PutWord(raw, 8, target);
        Put(raw, 14, BitConverter.SingleToUInt32Bits(2.125f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(3.25f));
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>16비트 WORD 를 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutWord(Span<byte> bytes, int offset, uint value)
    {
        // 두 바이트만 낮은 바이트부터 쓴다
        bytes[offset] = (byte)value;
        bytes[offset + 1] = (byte)(value >> 8);
    }
}
