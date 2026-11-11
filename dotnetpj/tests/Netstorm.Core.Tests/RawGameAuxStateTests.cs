using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 직접 Join 생성자 헤더의 초기값과 쓰기를 검사한다
/// (cpppj <c>o/RawGameAuxState.h</c>의 `battleStartPeerHeader_` 계약을 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.355에서 JoinTestBattle 생성자 헤더 재대조와 함께 추가했다. 원본 x86 fixture로 대조하지 않으므로 PORT_MAP 상태는 port다.</remarks>
public sealed class RawGameAuxStateTests
{
    /// <summary>생성자 헤더는 원문 생성 순간의 1bh 바이트·바이트·워드다.</summary>
    [Fact]
    public void BattleStartPeerHeaderHasOriginalConstructorBytes()
    {
        var state = new RawGameAuxState();
        Assert.Equal(new FormPacketHeader(0x1b, 1, 38), state.BattleStartPeerHeader);
        state.BattleStartPeerHeader = new FormPacketHeader(0x1c, 2, 40);
        Assert.Equal(new FormPacketHeader(0x1c, 2, 40), state.BattleStartPeerHeader);
    }
}
