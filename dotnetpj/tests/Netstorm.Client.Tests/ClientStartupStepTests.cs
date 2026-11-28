using Netstorm.Client.Host;

namespace Netstorm.Client.Tests; // 초기화의 실제 부분 효과와 재진입/재시도 경계를 검사한다.

/// <summary>완료 접두·최초 예외·잡힌 재진입 오류를 성공이나 재실행으로 바꾸지 않는지 확인한다.</summary>
public sealed class ClientStartupStepTests
{
    /// <summary>앞 단계 성공 뒤 다음 단계가 실패해도 앞선 효과를 반복하거나 실패 단계를 다시 실행하지 않는다.</summary>
    [Fact]
    public void FailurePreservesSuccessfulPrefixAndNeverRepeatsPartialEffects()
    {
        var first = new ClientStartupStep("Clock"); var second = new ClientStartupStep("Player");
        var events = new List<string>(); var failure = new IOException("부분 Player 할당");
        first.Run(() => events.Add("clock"));
        Assert.Same(failure, Assert.Throws<IOException>(() => second.Run(() => { events.Add("allocated"); throw failure; })));
        first.Run(() => throw new InvalidOperationException("성공한 Clock 반복"));
        Assert.Same(failure, Assert.Throws<IOException>(() => second.Run(() => events.Add("retry"))));
        Assert.Equal(["clock", "allocated"], events); Assert.True(first.Completed); Assert.False(second.Completed);
    }

    /// <summary>내부 action이 재진입 오류를 잡아도 그 단계를 완료로 기록하지 않으며 첫 오류를 계속 전달한다.</summary>
    [Fact]
    public void CaughtReentryStillPoisonsTheStageWithoutRepeatingItsAction()
    {
        var step = new ClientStartupStep("Scene Start"); Exception? original = null; int effects = 0;
        var outer = Assert.Throws<InvalidOperationException>(() => step.Run(() =>
        {
            effects++; original = Assert.Throws<InvalidOperationException>(() => step.Run(() => effects++));
        }));
        Assert.Same(original, outer); Assert.False(step.Completed); Assert.Equal(1, effects);
        Assert.Same(outer, Assert.Throws<InvalidOperationException>(() => step.Run(() => effects++))); Assert.Equal(1, effects);
    }

    /// <summary>전체 domain 실패는 이미 성공한 하위의 효과와 실패한 하위의 원래 예외를 보존한다.</summary>
    [Fact]
    public void DomainFailureRetainsInnerFailureAndOriginalThrowSite()
    {
        var domain = new ClientStartupStep("domain"); var issue = new ClientStartupStep("Issue"); var chat = new ClientStartupStep("Chat");
        int registrations = 0; var failure = new IOException("manager");
        // 실제 실패 위치가 재호출 뒤에도 스택에 남는지 확인한다.
        void AllocateManager() => throw failure;
        Assert.Same(failure, Assert.Throws<IOException>(() => domain.Run(() =>
        { issue.Run(() => registrations++); chat.Run(AllocateManager); })));
        Assert.Same(failure, Assert.Throws<IOException>(() => domain.Run(() => registrations++)));
        Assert.Contains(nameof(AllocateManager), failure.StackTrace!); Assert.Equal(1, registrations);
        Assert.True(issue.Completed); Assert.False(chat.Completed); Assert.False(domain.Completed);
    }

    /// <summary>잘못된 action 인자는 단계 실행이나 완료로 기록하지 않는다.</summary>
    [Fact]
    public void InvalidActionDoesNotPreventFollowingValidInitialization()
    {
        var step = new ClientStartupStep("Intro"); Assert.Throws<ArgumentNullException>(() => step.Run(null!));
        Assert.False(step.Completed); int calls = 0; step.Run(() => calls++); step.Run(() => calls++);
        Assert.True(step.Completed); Assert.Equal(1, calls);
    }
}
