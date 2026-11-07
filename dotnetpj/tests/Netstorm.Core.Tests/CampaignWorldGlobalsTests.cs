// 실제 캠페인 부모 전역의 빌림·복제·대입 수명 규칙을 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// CampaignWorldGlobals의 빌림·복제·대입을 검증하는 테스트 클래스다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.
public sealed class CampaignWorldGlobalsTests
{
    /// <summary>기본값은 독립 0·0·0·true다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void DefaultHasIndependentZerosAndPostTrue()
    {
        var first = new CampaignWorldGlobals();
        var second = new CampaignWorldGlobals();
        Assert.Equal(0u, first.MySubscriber);
        Assert.Equal(0u, first.LoginSubscriber);
        Assert.Equal(0u, first.Editor);
        Assert.True(first.MissionPost);
        first.MySubscriber = 7u;
        first.LoginSubscriber = 8u;
        first.Editor = 9u;
        first.MissionPost = false;
        Assert.Equal(0u, second.MySubscriber);
        Assert.Equal(0u, second.LoginSubscriber);
        Assert.Equal(0u, second.Editor);
        Assert.True(second.MissionPost);
    }

    /// <summary>값 생성은 자체 fallback을 소유한다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void ValueConstructorOwnsFallbacks()
    {
        var globals = new CampaignWorldGlobals(0xabcdef01u, 5u, 2u, false);
        Assert.Equal(0xabcdef01u, globals.MySubscriber);
        Assert.Equal(5u, globals.LoginSubscriber);
        Assert.Equal(2u, globals.Editor);
        Assert.False(globals.MissionPost);
        var defaults = new CampaignWorldGlobals(3u);
        Assert.Equal(3u, defaults.MySubscriber);
        Assert.Equal(0u, defaults.LoginSubscriber);
        Assert.Equal(0u, defaults.Editor);
        Assert.True(defaults.MissionPost);
    }

    /// <summary>빌린 쪽은 외부 상자를 읽고 쓰고, null 슬롯은 자체 저장소를 쓴다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void BindingBorrowsBoxesAndFallsBackToLocal()
    {
        var my = new UIntBox { Value = 11u };
        var globals = new CampaignWorldGlobals(my, null);
        Assert.Equal(11u, globals.MySubscriber);
        Assert.Equal(0u, globals.LoginSubscriber);
        globals.MySubscriber = 22u;
        globals.LoginSubscriber = 33u;
        Assert.Equal(22u, my.Value);
        Assert.Equal(33u, globals.LoginSubscriber);
        my.Value = 44u;
        Assert.Equal(44u, globals.MySubscriber);
    }

    /// <summary>복사는 값을 복제하며 외부 참조를 전파하지 않는다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void CopyReplicatesValuesWithoutPropagatingRefs()
    {
        var my = new UIntBox { Value = 1u };
        var login = new UIntBox { Value = 2u };
        var borrowed = CampaignWorldGlobals.BorrowSubscribers(my, login);
        borrowed.Editor = 3u;
        borrowed.MissionPost = false;
        var copy = new CampaignWorldGlobals(borrowed);
        Assert.Equal(1u, copy.MySubscriber);
        Assert.Equal(2u, copy.LoginSubscriber);
        Assert.Equal(3u, copy.Editor);
        Assert.False(copy.MissionPost);
        my.Value = 10u;
        login.Value = 20u;
        Assert.Equal(1u, copy.MySubscriber);
        Assert.Equal(2u, copy.LoginSubscriber);
        copy.MySubscriber = 30u;
        copy.LoginSubscriber = 40u;
        Assert.Equal(10u, my.Value);
        Assert.Equal(20u, login.Value);
    }

    /// <summary>대입은 목적지 바인딩을 유지하고 원본 값을 먼저 캡처한 뒤 쓴다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void AssignKeepsDestinationBindingAndCopiesValues()
    {
        var destinationMy = new UIntBox { Value = 100u };
        var destinationLogin = new UIntBox { Value = 200u };
        var destination = new CampaignWorldGlobals(destinationMy, destinationLogin);
        var source = new CampaignWorldGlobals(1u, 2u, 3u, false);
        destination.AssignFrom(source);
        Assert.Equal(1u, destinationMy.Value);
        Assert.Equal(2u, destinationLogin.Value);
        Assert.Equal(1u, destination.MySubscriber);
        Assert.Equal(2u, destination.LoginSubscriber);
        Assert.Equal(3u, destination.Editor);
        Assert.False(destination.MissionPost);
        // 같은 상자를 공유해도 캡처 뒤 쓰기라 값이 보존된다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.
        var shared = new UIntBox { Value = 9u };
        var aliasDestination = new CampaignWorldGlobals(shared, null);
        var aliasSource = new CampaignWorldGlobals(shared, null);
        aliasSource.MySubscriber = 5u;
        aliasDestination.AssignFrom(aliasSource);
        Assert.Equal(5u, shared.Value);
        Assert.Equal(5u, aliasDestination.MySubscriber);
        // 자기 대입도 값을 유지한다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.
        aliasDestination.AssignFrom(aliasDestination);
        Assert.Equal(5u, aliasDestination.MySubscriber);
    }

    /// <summary>두 주소를 빌린 전역은 외부 상자를 공유한다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void BorrowSubscribersSharesBothBoxes()
    {
        var my = new UIntBox { Value = 55u };
        var login = new UIntBox { Value = 66u };
        var globals = CampaignWorldGlobals.BorrowSubscribers(my, login);
        Assert.Equal(55u, globals.MySubscriber);
        Assert.Equal(66u, globals.LoginSubscriber);
        globals.MySubscriber = 77u;
        globals.LoginSubscriber = 88u;
        Assert.Equal(77u, my.Value);
        Assert.Equal(88u, login.Value);
    }
}
