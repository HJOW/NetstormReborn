using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 숫자 저장 카메라10칸의 초기화·포착·판정·숫자 분배를 대조한다.
// 2026-11-26 1.049에서 DI-4c 저장 카메라용으로 추가했다.
public sealed class SavedCameraKeyTests
{
    /// <summary>정지·언어·소리·중심·즉시 카메라를 기록 갈고리로 묶는다.</summary>
    /// <returns>갈고리와 호출 기록이다.</returns>
    private static (SavedCameraKeyHooks Hooks, List<string> Calls) Scene(bool paused, int language, ConstructionNoticePoint captured)
    {
        var calls = new List<string>();
        var hooks = new SavedCameraKeyHooks
        {
            Paused = () => paused,
            Language = () => language,
            Sound = () => calls.Add("S"),
            Capture = () => { calls.Add("P"); return captured; },
            Center = (x, y) => calls.Add($"C{x},{y}"),
        };
        return (hooks, calls);
    }

    /// <summary>명시 초기화는 열 좌표를 모두 같은 무효 Point로 쓴다.</summary>
    [Fact]
    public void InitializeWritesInvalidPoints()
    {
        var saved = new SavedCameraPositions();
        RawSavedCameraKey.InitializeSavedCameraPositions(saved);
        Assert.All(saved.Points, point => Assert.Equal(new ConstructionNoticePoint(-10000f, -10000f), point));
        Assert.All(saved.Points, point => Assert.False(RawSavedCameraKey.IsSavedCameraPositionValid(point)));
    }

    /// <summary>현재 Renderer 중심을 단정도 지도 좌표로 포착한다.</summary>
    [Fact]
    public void CaptureComputesFloatCenter()
    {
        var view = new RawCameraViewState(); RawCameraView.Initialize(view);
        RawCameraView.SetPixels(view, 64, 44);
        // x=(0+1024)/2+64=576을 16으로, y=(0+768)/2+44=428을 11로 나눈다.
        ConstructionNoticePoint point = RawSavedCameraKey.CaptureSavedCameraCenter(view, [0, 0, 1024, 768]);
        Assert.Equal(36f, point.X);
        Assert.Equal(428f / 11f, point.Y);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSavedCameraKey.CaptureSavedCameraCenter(view, [0, 0, 1024]));
    }

    /// <summary>두 축 0 이상 256 미만이 불러오기 범위다.</summary>
    [Fact]
    public void ValidityBoundsMatchOriginRange()
    {
        Assert.True(RawSavedCameraKey.IsSavedCameraPositionValid(new(0f, 0f)));
        Assert.True(RawSavedCameraKey.IsSavedCameraPositionValid(new(255.999f, 255.999f)));
        Assert.False(RawSavedCameraKey.IsSavedCameraPositionValid(new(256f, 0f)));
        Assert.False(RawSavedCameraKey.IsSavedCameraPositionValid(new(0f, 256f)));
        Assert.False(RawSavedCameraKey.IsSavedCameraPositionValid(new(-0.1f, 5f)));
    }

    /// <summary>Shift 실제 기호는 저장하고 숫자는 불러오며 표는 언어를 따른다.</summary>
    [Fact]
    public void DispatchShiftSymbolSavesAndDigitRecalls()
    {
        var saved = new SavedCameraPositions();
        RawSavedCameraKey.InitializeSavedCameraPositions(saved);
        (SavedCameraKeyHooks hooks, List<string> calls) = Scene(false, 0, new(12.5f, 30.25f));
        // 기본 표 "!@#$%^&*()"에서 '%'는 4번 칸이다.
        Assert.True(RawSavedCameraKey.DispatchSavedCameraKey(new('%' | InputCode.Shift, 0, 0), saved, hooks));
        Assert.Equal(new ConstructionNoticePoint(12.5f, 30.25f), saved.Points[4]);
        Assert.Equal(new[] { "S", "P" }, calls);
        // '5'는 4번 칸이며 저장된 좌표가 유효하므로 즉시 카메라에 전달된다.
        calls.Clear();
        Assert.True(RawSavedCameraKey.DispatchSavedCameraKey(new('5', 0, 0), saved, hooks));
        Assert.Equal(["C12.5,30.25"], calls);
        // 독일어 표 "!\"§$%&/()="에서 '='는 9번 칸이다.
        var german = Scene(false, 3, new(1f, 2f));
        Assert.True(RawSavedCameraKey.DispatchSavedCameraKey(new('=' | InputCode.Shift, 0, 0), saved, german.Hooks));
        Assert.Equal(new ConstructionNoticePoint(1f, 2f), saved.Points[9]);
        Assert.Equal(new[] { "S", "P" }, german.Calls);
    }

    /// <summary>뗌·후보 아님·정지·무효 불러오기는 저장·이동을 실행하지 않는다.</summary>
    [Fact]
    public void DispatchIgnoresReleaseForeignPausedAndInvalidRecall()
    {
        var saved = new SavedCameraPositions();
        RawSavedCameraKey.InitializeSavedCameraPositions(saved);
        (SavedCameraKeyHooks hooks, List<string> calls) = Scene(false, 0, new(1f, 2f));
        Assert.False(RawSavedCameraKey.DispatchSavedCameraKey(new('5' | InputCode.Release, 0, 0), saved, hooks));
        Assert.False(RawSavedCameraKey.DispatchSavedCameraKey(new('a', 0, 0), saved, hooks));
        Assert.Empty(calls);
        // 정지 중이면 숫자도 소비만 하고 갈고리를 부르지 않는다.
        var paused = Scene(true, 0, new(1f, 2f));
        Assert.True(RawSavedCameraKey.DispatchSavedCameraKey(new('5', 0, 0), saved, paused.Hooks));
        Assert.Empty(paused.Calls);
        // 무효 칸의 불러오기는 소비만 한다.
        Assert.True(RawSavedCameraKey.DispatchSavedCameraKey(new('5', 0, 0), saved, hooks));
        Assert.Empty(calls);
    }
}
