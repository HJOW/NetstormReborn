using Netstorm.Client.Host;
using Xunit;

namespace Netstorm.Client.Tests;

/// <summary>Save as 시나리오 확정·취소·오류 문맥의 분기 계약을 확인한다.</summary>
/// <remarks>2026-11-11 0.365에서 cpppj HEAD의 RawMenuSaveFort.inc S1 분기를 반영하며 추가했다.</remarks>
public sealed class SaveFortDialogScenarioTests
{
    /// <summary>확정 장치가 참이면 1을 돌려주고 처리했음을 알린다.</summary>
    [Fact]
    public void AcceptScenarioTrueReturnsOne()
    {
        var context = new SaveFortDialogContext { EditorAccept = _ => true };
        Assert.True(SaveFortDialogContext.TryAcceptScenario(context, "값", out uint result));
        Assert.Equal(1u, result);
    }

    /// <summary>확정 장치가 거짓이면 0을 돌려주고 같은 편집창을 유지한다.</summary>
    [Fact]
    public void AcceptScenarioFalseReturnsZero()
    {
        string seen = "";
        var context = new SaveFortDialogContext { EditorAccept = text => { seen = text; return false; } };
        Assert.True(SaveFortDialogContext.TryAcceptScenario(context, "본문", out uint result));
        Assert.Equal(0u, result);
        Assert.Equal("본문", seen);
    }

    /// <summary>확정 장치가 없으면 일반 경로로 돌려보낸다.</summary>
    [Fact]
    public void AcceptScenarioWithoutDeviceReturnsFalse()
    {
        Assert.False(SaveFortDialogContext.TryAcceptScenario(new SaveFortDialogContext(), "값", out uint result));
        Assert.Equal(0u, result);
        Assert.False(SaveFortDialogContext.TryAcceptScenario(null, "값", out result));
        Assert.Equal(0u, result);
    }

    /// <summary>취소 장치가 있으면 예약하고 1을 돌려준다.</summary>
    [Fact]
    public void CancelScenarioCallsDeviceAndReturnsOne()
    {
        int calls = 0;
        var context = new SaveFortDialogContext { EditorCancel = () => calls++ };
        Assert.True(SaveFortDialogContext.TryCancelScenario(context, out uint result));
        Assert.Equal(1u, result);
        Assert.Equal(1, calls);
    }

    /// <summary>취소 장치가 없으면 일반 경로로 돌려보낸다.</summary>
    [Fact]
    public void CancelScenarioWithoutDeviceReturnsFalse()
    {
        Assert.False(SaveFortDialogContext.TryCancelScenario(new SaveFortDialogContext(), out uint result));
        Assert.Equal(0u, result);
        Assert.False(SaveFortDialogContext.TryCancelScenario(null, out result));
        Assert.Equal(0u, result);
    }

    /// <summary>일반 확인이 성공하면 결과를 그대로 돌려준다.</summary>
    [Fact]
    public void AcceptWithReportPassesThroughNormalResult()
    {
        var context = new SaveFortDialogContext { ReportError = _ => throw new InvalidOperationException("호출되면 안 된다") };
        Assert.Equal(1u, SaveFortDialogContext.AcceptWithReport(context, () => 1u));
        Assert.Equal(1u, SaveFortDialogContext.AcceptWithReport(null, () => 1u));
    }

    /// <summary>일반 확인이 실패하고 안내 장치가 있으면 같은 창에 띄우고 0을 돌려준다.</summary>
    [Fact]
    public void AcceptWithReportNotifiesAndReturnsZeroOnError()
    {
        string seen = "";
        var context = new SaveFortDialogContext { ReportError = message => seen = message };
        Assert.Equal(0u, SaveFortDialogContext.AcceptWithReport(context, () => throw new InvalidOperationException("측면 충돌")));
        Assert.Equal("측면 충돌", seen);
    }

    /// <summary>안내 장치가 없으면 원문처럼 예외를 그대로 던진다.</summary>
    [Fact]
    public void AcceptWithReportRethrowsWithoutReporter()
    {
        Assert.Throws<InvalidOperationException>(() =>
            SaveFortDialogContext.AcceptWithReport(new SaveFortDialogContext(), () => throw new InvalidOperationException("원문 계약")));
        Assert.Throws<InvalidOperationException>(() =>
            SaveFortDialogContext.AcceptWithReport(null, () => throw new InvalidOperationException("원문 계약")));
    }
}
