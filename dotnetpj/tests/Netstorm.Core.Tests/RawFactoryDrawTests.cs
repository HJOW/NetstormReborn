using System.Buffers.Binary;
using Netstorm.Core.Original;

// 원본 캡처의 소비자 결선과 별도로 파생 분배·생략·오류 경계를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>Factory의 실제 가상 표·상태/레벨·현재 그림 계약을 검사한다.</summary>
public sealed class RawFactoryDrawTests
{
    /// <summary>두 글자만 가진 TYPE85의 현재 프레임 표를 만든다.</summary>
    private static RiftTypeFrames[] Frames() => Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames(
        [new((byte)'A', (byte)'0', 0, 0), new((byte)'B', (byte)'0', 0, 0)])).ToArray();

    /// <summary>모든 레벨·두 가상 표와 죽은/빈/void 상태의 원문 분기를 확인한다.</summary>
    [Fact]
    public void LevelsAndAdditionalSuppressionReadCurrentRaw()
    {
        var pool = new RawSidPool(32768); int sid = pool.Allocate(); Span<byte> raw = pool.AllocatedBytes(sid); raw.Clear(); raw[10] = 85;
        var draw = new RawFactoryDraw(pool, Frames()); var calls = new List<(int, int, uint, int)>();
        // 두 파생은 같은 가상 몸체를 쓰되 타입 이름으로 분배하지 않는다.
        foreach (uint table in new uint[] { 0x509148, 0x509210 })
        {
            BinaryPrimitives.WriteUInt32LittleEndian(raw, table); Assert.True(draw.Handles(sid));
            // 없는C~H 글자는-1이고 상위3비트를 빠짐없이 사용한다.
            for (int level = 0; level < 8; level++)
            {
                raw[11] = (byte)(level << 5); Assert.Equal(level < 2 ? level : -1, draw.MainFrame(sid));
            }
            raw[11] = 1 | 4; raw[40] = 2; BinaryPrimitives.WriteInt32LittleEndian(raw.Slice(36, 4), -7);
            draw.Additional(sid, -11, 23, (x, y, type, frame) => calls.Add((x, y, type, frame)));
            Assert.Equal((-11, 23, 85u, -7), calls[^1]);
            Assert.Throws<InvalidOperationException>(() => draw.Additional(sid, 0, 0, null));
            // 각 억제 원인은 실제 그리기 소비자가 없어도 조기 복귀한다.
            foreach (byte extra in new byte[] { 1, 8, 9 }) { raw[40] = extra; draw.Additional(sid, 0, 0, null); }
            raw[40] = 2; raw[11] = 2; draw.Additional(sid, 0, 0, null);
            raw[11] = 0; BinaryPrimitives.WriteInt32LittleEndian(raw.Slice(36, 4), 0); draw.Additional(sid, 0, 0, null);
        }
        Assert.Equal(2, calls.Count);
    }

    /// <summary>미복원 파생/TYPE·다른 판본·표 폭을 명시 거부한다.</summary>
    [Fact]
    public void UnrestoredVtableTypeAndEditionFailExplicitly()
    {
        var pool = new RawSidPool(32768); int sid = pool.Allocate(); Span<byte> raw = pool.AllocatedBytes(sid); raw.Clear(); raw[10] = 85;
        var draw = new RawFactoryDraw(pool, Frames()); Assert.False(draw.Handles(sid));
        string error = Assert.Throws<InvalidOperationException>(() => draw.MainFrame(sid)).Message;
        Assert.Contains($"SID={sid} TYPE=85 VT=0", error);
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x509148); raw[10] = 188;
        Assert.Throws<InvalidOperationException>(() => draw.Additional(sid, 0, 0, null));
        Assert.Throws<ArgumentException>(() => new RawFactoryDraw(pool, Frames()[..187]));
        Assert.Throws<ArgumentException>(() => new RawFactoryDraw(new RawSidPool(32768, edition: OriginalEdition.Cd1072), Frames()));
    }
}
