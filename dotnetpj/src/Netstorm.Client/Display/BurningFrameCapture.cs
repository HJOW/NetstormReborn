using Netstorm.Core.Original;

// 연소 프레임은 기존 실제SHP/plain RLE 캡처를 같은 풀에서 재사용한다.
namespace Netstorm.Client.Display;

/// <summary>cpppj MakeBurningFrameCapture의 함수사본/동일풀 연결기다.</summary>
public static class BurningFrameCapture
{
    /// <summary>입력의 소리장치는 보존하며 실제 현재프레임 캡처만 같은 ShapeToBuffer에 연결한다.</summary>
    public static BurningFrameHooks Make(RawShapeToBuffer capture, RawSidPool pool, BurningFrameHooks? hooks = null)
    {
        ArgumentNullException.ThrowIfNull(capture); ArgumentNullException.ThrowIfNull(pool);
        if (!ReferenceEquals(capture.Pool(), pool)) throw new ArgumentException("Burning frame/ShapeToBuffer SID풀 불일치");
        return new() { Capture = sid => capture.Capture(sid), StopSound = hooks?.StopSound };
    }
}
