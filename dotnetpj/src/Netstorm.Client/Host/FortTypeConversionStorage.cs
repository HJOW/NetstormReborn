namespace Netstorm.Client.Host;

/// <summary>
/// 원본 현재 conversion DWORD와 실제 255바이트 할당을 Client 수명에 유지하는 호스트 소유자다
/// (cpppj <c>client/FortTypeConversionStorage.h</c>의 <c>FortTypeConversionStorage</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-9에서 추가했다. 원본 부모가 현재 포인터 대입·해제 순서를 소유하고 이 계층은 주소와 할당 수명만 제공한다.</remarks>
public sealed class FortTypeConversionStorage
{
    /// <summary>원본 004bd800의 실제 operator new 요청 폭이다.</summary>
    public const uint ConversionBytes = 255u;

    /// <summary>최초 BSS 0인 현재 conversion DWORD다. 부모가 직접 대입한다.</summary>
    private uint _current;

    /// <summary>호스트 토큰 발급 상태다. 서로 다른 값이며 0은 미발급이다.</summary>
    private uint _nextToken;

    /// <summary>발급한 한 토큰의 실제 힙 주소와 물리 폭을 함께 보존한다.</summary>
    private readonly Dictionary<uint, byte[]> _allocations = new();

    /// <summary>
    /// 같은 Client 전역의 수정 가능한 참조다.
    /// </summary>
    /// <returns>부모가 실제 대입할 현재 DWORD 참조다.</returns>
    /// <remarks>원본의 const 겹침은 C#에서 이름이 겹치므로 이 하나로 둔다. 읽기 소비자도 같은 주소를 빌린다.</remarks>
    public ref uint Current()
    {
        return ref _current;
    }

    /// <summary>
    /// 실제 255바이트를 할당만 하고 본문의 zero나 현재 대입을 앞당기지 않는다.
    /// </summary>
    /// <param name="bytes">요청 폭이다. 원본과 같이 255만 받는다.</param>
    /// <returns>해제되지 않은 실제 할당의 비영 토큰이다.</returns>
    /// <exception cref="ArgumentException">Fort 변환표의 실제 255바이트 할당 폭이 아니다.</exception>
    /// <exception cref="OverflowException">Fort 변환표 수명 토큰이 소진됐다.</exception>
    /// <remarks>원본은 미초기화 블록을 만들지만 C#의 새 배열은 0으로 채워진다. 로더가 실제 DWORD/WORD/BYTE zero를 수행하므로 관측이 같다.</remarks>
    public uint Allocate(uint bytes)
    {
        if (bytes != ConversionBytes)
        {
            throw new ArgumentException("Fort 변환표의 실제255바이트 할당 폭", nameof(bytes));
        }
        if (_nextToken == uint.MaxValue)
        {
            throw new OverflowException("Fort 변환표 수명 토큰 소진");
        }
        uint token = unchecked(_nextToken + 1u);
        // 할당 성공 전에는 현재 전역이나 발급 상태를 바꾸지 않는다
        byte[] allocation = new byte[bytes];
        _allocations.Add(token, allocation);
        _nextToken = token;
        return token;
    }

    /// <summary>
    /// 해제되지 않은 같은 물리 할당을 빌리며 없는 주소는 명시 거부한다.
    /// </summary>
    /// <param name="token">Allocate가 발급한 토큰이다.</param>
    /// <returns>같은 실제 버퍼의 가변 뷰 Stabil다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Fort 변환표의 해제되거나 없는 실제 주소다.</exception>
    public Span<byte> Bytes(uint token)
    {
        if (token == 0 || !_allocations.TryGetValue(token, out byte[]? allocation))
        {
            throw new ArgumentOutOfRangeException(nameof(token), token, "Fort 변환표의 해제되거나 없는 실제 주소");
        }
        return allocation.AsSpan();
    }

    /// <summary>
    /// 지정 할당만 반납하고 현재 0 쓰기를 대신 수행하지 않는다. free(nullptr)는 허용한다.
    /// </summary>
    /// <param name="token">반납할 토큰이다. 0이면 아무 것도 하지 않는다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort 변환표의 이미 반납한 실제 주소다.</exception>
    public void Release(uint token)
    {
        if (token == 0)
        {
            return;
        }
        if (!_allocations.Remove(token))
        {
            throw new ArgumentOutOfRangeException(nameof(token), token, "Fort 변환표의 이미 반납한 실제 주소");
        }
    }
}
