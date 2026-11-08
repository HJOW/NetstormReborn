// 표시 부모와 SHP 접근의 계약을 확인한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 표시 부모 모듈을 검증하는 테스트 클래스다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.
public sealed class RawSquidDisplayParentTests
{
    /// <summary>50바이트 슬롯을 만든다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    private static byte[] Slot(uint type, int frame, float x, float y, byte extra = 0)
    {
        var raw = new byte[50];
        raw[RawSidPool.TypeOffset] = (byte)type;
        Write(raw, 36, unchecked((uint)frame), 4);
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x), 4);
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y), 4);
        raw[40] = extra;
        return raw;
    }

    /// <summary>TYPE 뷰를 만든다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    private static RawSquidDisplayTypeView View(uint token, uint flags1, int frameCount, bool loaded, int header) =>
        new(token, flags1, frameCount, loaded, header);

    /// <summary>물리 크기가 맞지 않으면 거부하는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRejectsBadSize()
    {
        var hooks = new RawSquidDisplayParentHooks();
        Assert.Throws<ArgumentException>(() => RawSquidDisplayParent.Run(new byte[49], new RawCheckingFlag(false), hooks));
    }

    /// <summary>유효 프레임이 표시 요청으로 이어지는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void RunMarksValidFrame()
    {
        var marks = new List<(uint Type, int Frame, uint X, uint Y, uint Marker)>();
        var hooks = new RawSquidDisplayParentHooks
        {
            Type = _ => View(80u, 0u, 10, true, 0),
            TypeNumber = view => view.Token,
            FrameError = (_, _, _) => { },
            Assertion = (_, _, _) => { },
            Mark = (type, frame, x, y, marker) => marks.Add((type, frame, x, y, marker)),
        };
        RawSquidDisplayParent.Run(Slot(80u, 3, 10f, 20f, 4), new RawCheckingFlag(false), hooks);
        uint xbits = BitConverter.SingleToUInt32Bits(10f);
        uint ybits = BitConverter.SingleToUInt32Bits(20f);
        Assert.Equal([(80u, 3, xbits, ybits, 16u)], marks);
    }

    /// <summary>범위 밖 프레임이 표시 요청 없이 끝나는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void RunSkipsMarkWhenOutOfRange()
    {
        var marks = new List<(uint Type, int Frame, uint X, uint Y, uint Marker)>();
        var hooks = new RawSquidDisplayParentHooks
        {
            Type = _ => View(80u, 0u, 10, true, 0),
            TypeNumber = view => view.Token,
            FrameError = (_, _, _) => { },
            Assertion = (_, _, _) => { },
            Mark = (type, frame, x, y, marker) => marks.Add((type, frame, x, y, marker)),
        };
        RawSquidDisplayParent.Run(Slot(80u, 99, 10f, 20f), new RawCheckingFlag(false), hooks);
        Assert.Empty(marks);
    }

    /// <summary>진단 중 범위 밖 프레임이 보고·단언되는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void RunReportsAndAssertsWhenChecking()
    {
        var errors = new List<(uint Number, int Frame, uint Abstract)>();
        var assertions = new List<(string Expression, string File, uint Line)>();
        var marks = new List<(uint Type, int Frame, uint X, uint Y, uint Marker)>();
        var hooks = new RawSquidDisplayParentHooks
        {
            Type = _ => View(80u, 0u, 10, true, 0),
            TypeNumber = _ => 80u,
            FrameError = (number, frame, abstractValue) => errors.Add((number, frame, abstractValue)),
            Assertion = (expression, file, line) => assertions.Add((expression, file, line)),
            Mark = (type, frame, x, y, marker) => marks.Add((type, frame, x, y, marker)),
        };
        RawSquidDisplayParent.Run(Slot(80u, 99, 10f, 20f, 1), new RawCheckingFlag(true), hooks);
        Assert.Equal([(80u, 99, 1u), (80u, 99, 1u)], errors);
        Assert.Single(assertions);
        Assert.Equal("getTypeStruct()->frameCheck( isAbstract(), theFrame, true )", assertions[0].Expression);
        Assert.Equal("\\Ns\\O\\Squid.cpp", assertions[0].File);
        Assert.Equal(0x846u, assertions[0].Line);
        Assert.Empty(marks);
    }

    /// <summary>그림이 없으면 표시 요청 없이 단언하는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void RunAssertsWhenShapeMissing()
    {
        var assertions = new List<(string Expression, string File, uint Line)>();
        var marks = new List<(uint Type, int Frame, uint X, uint Y, uint Marker)>();
        var hooks = new RawSquidDisplayParentHooks
        {
            Type = _ => View(80u, 0u, 10, false, 0),
            TypeNumber = view => view.Token,
            FrameError = (_, _, _) => { },
            Assertion = (expression, file, line) => assertions.Add((expression, file, line)),
            Mark = (type, frame, x, y, marker) => marks.Add((type, frame, x, y, marker)),
        };
        RawSquidDisplayParent.Run(Slot(80u, 0, 10f, 20f), new RawCheckingFlag(true), hooks);
        Assert.Single(assertions);
        Assert.Empty(marks);
    }

    /// <summary>빠진 장치를 명시 오류로 알리는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void RunRejectsMissingHook()
    {
        var hooks = new RawSquidDisplayParentHooks();
        Assert.Throws<InvalidOperationException>(() => RawSquidDisplayParent.Run(Slot(80u, 0, 10f, 20f), new RawCheckingFlag(false), hooks));
    }

    /// <summary>범위 안 SHP를 그대로 읽는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void ReadFrameReturnsShape()
    {
        var hooks = new RiftShapeAccessHooks
        {
            TypeNumber = _ => 80u,
            FrameError = (_, _, _) => { },
            Assertion = (_, _, _) => { },
            Shape = (token, frame) => [1.5f, 2.5f],
        };
        float[] size = RawRiftShapeAccess.ReadFrame(View(80u, 0u, 10, true, 0), 3, 0u, hooks);
        Assert.Equal([1.5f, 2.5f], size);
    }

    /// <summary>범위 밖 프레임이 오류·단언되는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void ReadFrameReportsAndAssertsOutOfRange()
    {
        var errors = new List<(uint Number, int Frame, uint Abstract)>();
        var hooks = new RiftShapeAccessHooks
        {
            TypeNumber = _ => 80u,
            FrameError = (number, frame, abstractValue) => errors.Add((number, frame, abstractValue)),
            Assertion = (_, _, _) => throw new InvalidOperationException("단언 복귀"),
            Shape = (_, _) => [0f, 0f],
        };
        Assert.Throws<InvalidOperationException>(() => RawRiftShapeAccess.ReadFrame(View(80u, 0u, 10, true, 0), 99, 1u, hooks));
        Assert.Equal([(80u, 99, 1u)], errors);
    }

    /// <summary>그림이 없어도 단언 뒤 현재 SHP를 읽는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void ReadFrameReadsShapeAfterAssertion()
    {
        var assertions = new List<(string Expression, string File, uint Line)>();
        bool shaped = false;
        var hooks = new RiftShapeAccessHooks
        {
            TypeNumber = _ => 80u,
            FrameError = (_, _, _) => { },
            Assertion = (expression, file, line) => assertions.Add((expression, file, line)),
            Shape = (token, frame) => { shaped = true; return [3f, 4f]; },
        };
        float[] size = RawRiftShapeAccess.ReadFrame(View(80u, 0u, 10, false, 0), 3, 0u, hooks);
        Assert.Single(assertions);
        Assert.Equal("frameCheck( abstract, frame, false )", assertions[0].Expression);
        Assert.True(shaped);
        Assert.Equal([3f, 4f], size);
    }

    /// <summary>빠진 SHP 장치를 명시 오류로 알리는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void ReadFrameRejectsMissingShape()
    {
        var hooks = new RiftShapeAccessHooks
        {
            TypeNumber = _ => 80u,
            FrameError = (_, _, _) => { },
            Assertion = (_, _, _) => { },
        };
        Assert.Throws<InvalidOperationException>(() => RawRiftShapeAccess.ReadFrame(View(80u, 0u, 10, true, 0), 3, 0u, hooks));
    }

    /// <summary>두 배 개수를 적용하는지 검증한다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void ReadFrameDoublesCount()
    {
        int calls = 0;
        var hooks = new RiftShapeAccessHooks
        {
            TypeNumber = _ => 80u,
            FrameError = (_, _, _) => { },
            Assertion = (_, _, _) => { },
            Shape = (_, _) => { calls++; return [0f, 0f]; },
        };
        RawRiftShapeAccess.ReadFrame(View(80u, 0x440000u, 10, true, 0), 15, 0u, hooks);
        Assert.Equal(1, calls);
        Assert.Throws<InvalidOperationException>(() => RawRiftShapeAccess.ReadFrame(View(80u, 0x440000u, 10, true, 0), 20, 0u, new RiftShapeAccessHooks
        {
            TypeNumber = _ => 80u,
            FrameError = (_, _, _) => { },
            Assertion = (_, _, _) => throw new InvalidOperationException("단언 복귀"),
            Shape = (_, _) => [0f, 0f],
        }));
    }

    /// <summary>슬롯 바이트를 리틀 엔디언으로 쓰는 검사 보조다. 2026-11-07 F2d 표시 부모 이식에서 추가됐다.</summary>
    private static void Write(Span<byte> raw, int offset, uint value, int width)
    {
        for (int i = 0; i < width; i++)
        {
            raw[offset + i] = (byte)(value >> (8 * i));
        }
    }
}
