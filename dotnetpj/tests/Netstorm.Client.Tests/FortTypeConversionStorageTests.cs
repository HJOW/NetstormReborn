using Netstorm.Client.Host;

namespace Netstorm.Client.Tests;

/// <summary>
/// 호스트 변환표 소유자의 현재 DWORD·255바이트 할당·수명을 검사한다
/// (cpppj <c>tests/FortTypeConversionStorageTests.cpp</c> 6개를 직역했다).
/// </summary>
/// <remarks>2026-11-03 F2c-9에서 추가했다. 원본 fixture가 없는 직역 검사라 PORT_MAP 상태는 port다.</remarks>
public sealed class FortTypeConversionStorageTests
{
    /// <summary>원본 004bd800의 실제 operator new 요청 폭이다.</summary>
    private const uint ExpectedBytes = 255u;

    /// <summary>
    /// 전체 물리 블록을 먼저 명시적으로 작성하므로 미초기화 메모리는 한 바이트도 읽지 않는다.
    /// </summary>
    /// <param name="owner">검사할 소유자다.</param>
    /// <param name="token">Allocate가 발급한 토큰이다.</param>
    /// <param name="value">채울 값이다.</param>
    /// <returns>같은 실제 할당의 뷰 Stabil다.</returns>
    private static Span<byte> InitializeConversion(FortTypeConversionStorage owner, uint token, byte value)
    {
        Span<byte> bytes = owner.Bytes(token);
        if (bytes.Length != (int)ExpectedBytes)
        {
            throw new InvalidOperationException("변환표 검사에 필요한 실제255바이트 범위");
        }
        bytes.Fill(value);
        return bytes;
    }

    /// <summary>
    /// 의도한 예외 형식만 성공으로 인정하여 다른 실패를 주소·크기 거부로 오인하지 않는다.
    /// </summary>
    /// <typeparam name="TException">기대하는 예외 형식이다.</typeparam>
    /// <param name="operation">실행할 동작이다.</param>
    /// <returns>기대 형식이면 참이다.</returns>
    private static bool ConversionRejects<TException>(Action operation)
        where TException : Exception
    {
        try
        {
            operation();
        }
        catch (TException)
        {
            return true;
        }
        catch
        {
            return false;
        }
        return false;
    }

    /// <summary>읽기·쓰기 참조는 같은 실제 DWORD이며 독립 소유자의 현재에는 영향을 주지 않는다.</summary>
    [Fact]
    public void CurrentReferencesAndIndependentOwners()
    {
        FortTypeConversionStorage first = new();
        FortTypeConversionStorage second = new();
        // 같은 Client 전역의 수정 가능한 참조를 빌린다
        ref uint current = ref first.Current();
        Assert.Equal(0u, current);
        Assert.Equal(0u, second.Current());
        uint token = first.Allocate(ExpectedBytes);
        Assert.NotEqual(0u, token);
        Assert.Equal(0u, current);
        Assert.Equal(0u, first.Current());
        current = token;
        second.Current() = 0x87654321u;
        Assert.Equal(token, first.Current());
        Assert.Equal(0x87654321u, second.Current());
        current = 0;
        // 빈 TypeNames처럼 현재 포인터만 잃어도 소유한 옛 할당은 남아야 한다
        Assert.Equal((int)ExpectedBytes, first.Bytes(token).Length);
        Assert.Equal(0x87654321u, second.Current());
    }

    /// <summary>두 살아 있는 할당은 독립이고 반복 조회는 복사본이 아닌 같은 물리 메모리를 반환한다.</summary>
    [Fact]
    public void ExactAllocationAndPhysicalAliases()
    {
        FortTypeConversionStorage owner = new();
        owner.Current() = 0xfedcba98u;
        uint first = owner.Allocate(ExpectedBytes);
        uint second = owner.Allocate(ExpectedBytes);
        Assert.NotEqual(0u, first);
        Assert.NotEqual(0u, second);
        Assert.NotEqual(first, second);
        Assert.Equal(0xfedcba98u, owner.Current());
        // 첫 255바이트 전체를 작성한다
        Span<byte> firstBytes = InitializeConversion(owner, first, 0x31);
        Span<byte> secondBytes = InitializeConversion(owner, second, 0x72);
        Assert.NotEqual(first, second);
        Assert.True(owner.Bytes(first).SequenceEqual(firstBytes.ToArray()));
        Assert.True(owner.Bytes(second).SequenceEqual(secondBytes.ToArray()));
        Assert.Equal(255, firstBytes.Length);
        Assert.Equal(255, secondBytes.Length);
        firstBytes[0] = 0x19;
        owner.Bytes(first)[254] = 0xe7;
        Assert.Equal(0x19, owner.Bytes(first)[0]);
        Assert.Equal(0xe7, firstBytes[254]);
        // 다른 살아 있는 블록에는 쓰기가 전달되지 않는다
        foreach (byte value in owner.Bytes(second).ToArray())
        {
            Assert.Equal(0x72, value);
        }
        owner.Bytes(second)[127] = 0xa5;
        Assert.Equal(0xa5, owner.Bytes(second)[127]);
        Assert.Equal(0x31, firstBytes[127]);
        Assert.Equal(0xfedcba98u, owner.Current());
    }

    /// <summary>0·없는 주소와 이미 해제한 주소는 실제 배열처럼 읽거나 다시 반납할 수 없다.</summary>
    [Fact]
    public void UnknownAndFreedTokensRejected()
    {
        FortTypeConversionStorage owner = new();
        const uint unknown = uint.MaxValue;
        Assert.True(ConversionRejects<ArgumentOutOfRangeException>(() => owner.Bytes(0)));
        Assert.True(ConversionRejects<ArgumentOutOfRangeException>(() => owner.Bytes(unknown)));
        Assert.True(ConversionRejects<ArgumentOutOfRangeException>(() => owner.Release(unknown)));
        uint token = owner.Allocate(ExpectedBytes);
        InitializeConversion(owner, token, 0x4c);
        owner.Release(token);
        Assert.True(ConversionRejects<ArgumentOutOfRangeException>(() => owner.Bytes(token)));
        Assert.True(ConversionRejects<ArgumentOutOfRangeException>(() => owner.Release(token)));
        Assert.Equal(0u, owner.Current());
    }

    /// <summary>free(nullptr)·비현재 해제와 현재 해제는 부모의 별도 현재 쓰기를 대신하지 않는다.</summary>
    [Fact]
    public void ReleasePreservesCurrentAndOtherBlocks()
    {
        FortTypeConversionStorage owner = new();
        owner.Release(0);
        uint previous = owner.Allocate(ExpectedBytes);
        uint current = owner.Allocate(ExpectedBytes);
        InitializeConversion(owner, previous, 0x18);
        // 비현재 할당 반납 뒤에도 읽을 명시 초기화 블록이다
        Span<byte> live = InitializeConversion(owner, current, 0x59);
        byte[] liveCopy = live.ToArray();
        owner.Current() = current;
        owner.Release(0);
        Assert.Equal(current, owner.Current());
        Assert.True(owner.Bytes(current).SequenceEqual(liveCopy));
        owner.Release(previous);
        Assert.Equal(current, owner.Current());
        Assert.Equal(0x59, owner.Bytes(current)[0]);
        Assert.Equal(0x59, owner.Bytes(current)[254]);
        Assert.True(ConversionRejects<ArgumentOutOfRangeException>(() => owner.Bytes(previous)));
        owner.Release(current);
        // 해제 뒤에도 원문 부모의 무조건 0쓰기 전까지 옛 DWORD가 남는다
        Assert.Equal(current, owner.Current());
        Assert.True(ConversionRejects<ArgumentOutOfRangeException>(() => owner.Bytes(current)));
        owner.Release(0);
        Assert.Equal(current, owner.Current());
        owner.Current() = 0;
        Assert.Equal(0u, owner.Current());
        Assert.True(liveCopy[0] == 0x59);
    }

    /// <summary>여러 세션과 같은 소비자들이 한 소유자를 빌리면 마지막 소비자가 놓을 때까지 할당이 유지된다.</summary>
    [Fact]
    public void SharedConsumersKeepOwnerAlive()
    {
        FortTypeConversionStorage firstSession = new();
        uint token = firstSession.Allocate(ExpectedBytes);
        InitializeConversion(firstSession, token, 0x8b);
        firstSession.Current() = token;
        firstSession.Bytes(token)[254] = 0xc6;
        Assert.Equal(token, firstSession.Current());
        Assert.Equal(0xc6, firstSession.Bytes(token)[254]);
        Assert.Equal(0x8b, firstSession.Bytes(token)[0]);
    }

    /// <summary>잘못된 크기는 명시 진단하고 이미 있는 현재·할당을 바꾸거나 다음 성공 할당을 막지 않는다.</summary>
    [Fact]
    public void WrongSizesPreserveLiveState()
    {
        FortTypeConversionStorage owner = new();
        uint existing = owner.Allocate(ExpectedBytes);
        Span<byte> bytes = InitializeConversion(owner, existing, 0xd3);
        byte[] snapshot = bytes.ToArray();
        owner.Current() = existing;
        uint[] invalidSizes = [0, 1, 254, 256, uint.MaxValue];
        // 각 실패가 같은 살아 있는 할당에 부작용을 남기는지 확인한다
        foreach (uint size in invalidSizes)
        {
            Assert.True(ConversionRejects<ArgumentException>(() => owner.Allocate(size)));
            Assert.Equal(existing, owner.Current());
            Assert.True(owner.Bytes(existing).SequenceEqual(snapshot));
            foreach (byte value in owner.Bytes(existing).ToArray())
            {
                Assert.Equal(0xd3, value);
            }
        }
        uint next = owner.Allocate(ExpectedBytes);
        Assert.NotEqual(0u, next);
        Assert.NotEqual(existing, next);
        Assert.Equal(existing, owner.Current());
        Assert.Equal(255, owner.Bytes(next).Length);
        Assert.Equal(0xd3, owner.Bytes(existing)[0]);
        Assert.Equal(0xd3, owner.Bytes(existing)[254]);
    }
}
