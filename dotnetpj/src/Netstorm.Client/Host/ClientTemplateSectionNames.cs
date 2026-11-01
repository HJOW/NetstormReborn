using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>
/// 실제 512바이트 이름표 할당을 요청한다. 할당에 실패하면 null을 돌려준다(원본 <c>operator new(nothrow)</c>의 NULL).
/// 실제 호스트는 기본 할당을 쓰고, 독립 검사는 알려진 바이트로 채운 할당을 공급한다.
/// </summary>
/// <param name="size">요청 크기(항상 512).</param>
/// <remarks>2026-11-01 0.129에서 추가했다.</remarks>
public delegate byte[]? TemplateSectionNamesAllocator(int size);

/// <summary>
/// Client 수명의 전역 005c8aa4 표 한 개를 파일 reader와 신규 manager 초기화가 함께 빌리는 원문 lazy owner다.
/// 최초 <see cref="Names"/>에서만 원문 부모(<see cref="RawTemplateSectionNames.Get"/>)를 실행해 실제 512바이트를 할당하고 277바이트를 쓴다.
/// 표 끝의 미정 235바이트는 게임 규칙으로 읽지 않는다. Session/Storage 교체와 독립이며 이동·복제하지 않는다.
/// cpppj <c>client/ClientTemplateSectionNames</c>를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-01 0.129에서 추가했다. C#의 새 배열은 0으로 채워지지만 이 클래스는 앞 277바이트 밖을 읽거나 쓰지 않는다.
/// 이름표 포인터(<see cref="Names"/>가 돌려주는 같은 배열)와 SNART 변환 상태는 생성한 뒤 이 owner가 끝까지 보존한다.
/// </remarks>
public sealed class ClientTemplateSectionNames
{
    /// <summary>"Terr%02d" 서식의 정의된 출력 길이(NUL 포함 7바이트)다.</summary>
    private const int FormattedBytes = 7;

    /// <summary>영역 이름 번호의 개수(Terr00~Terr19)다.</summary>
    private const uint TerritoryNames = 20;

    /// <summary>최초 선택 시에만 실행하는 실제 또는 검사 allocator다.</summary>
    private readonly TemplateSectionNamesAllocator _allocate;

    /// <summary>원문이 쓴 277바이트만 정의된 실제 512바이트 표다.</summary>
    private byte[]? _bytes;

    /// <summary>새 Client의 BSS 0이며 Session 정리에서 reset하지 않는다. 최초 0, 성공 1인 호스트 불투명 token이다.</summary>
    private uint _current;

    /// <summary>할당은 아직 하지 않으며 최초 <see cref="Names"/>에서만 원문 부모를 실행한다.</summary>
    /// <param name="allocate">표 할당 장치. 생략하면 새 배열을 할당한다.</param>
    public ClientTemplateSectionNames(TemplateSectionNamesAllocator? allocate = null)
    {
        _allocate = allocate ?? (size => new byte[size]);
    }

    /// <summary>
    /// 실제 512바이트의 동일한 배열을 lazy 생성해 돌려준다. 할당이 NULL이면 원문 004bcced의 NULL 이름표 복사에 해당하는 물리 실패로 남기고 현재 token을 0으로 보존한다.
    /// </summary>
    public Span<byte> Names()
    {
        // 원문 서식의 정의된 7바이트만 출력하며 미정 local256 끝을 빌리지 않는다
        byte[] formatted = new byte[FormattedBytes];
        TemplateSectionNamesHooks hooks = new()
        {
            Allocate = size =>
            {
                if (size != RawTemplateSectionNames.Capacity)
                {
                    throw new InvalidOperationException("Client 이름표 실제512할당계약");
                }
                _bytes = _allocate((int)size);
                return _bytes is null ? 0u : 1u;
            },
            // 0은 정상 표로 만들지 않는다
            Buffer = token =>
            {
                if (token != 1 || _bytes is null)
                {
                    throw new ArgumentOutOfRangeException(null, "Client 이름표 실제NULL할당접근");
                }
                return _bytes.AsSpan(0, (int)RawTemplateSectionNames.Capacity);
            },
            // 실제 선택 범위에서는 Terr%02d의 ASCII/NUL과 동일하다
            Format = index =>
            {
                if (index >= TerritoryNames)
                {
                    throw new ArgumentOutOfRangeException(null, "Template Terr원문서식0..19범위");
                }
                formatted[0] = (byte)'T';
                formatted[1] = (byte)'e';
                formatted[2] = (byte)'r';
                formatted[3] = (byte)'r';
                formatted[4] = (byte)('0' + index / 10);
                formatted[5] = (byte)('0' + index % 10);
                formatted[6] = 0;
                return formatted;
            },
        };
        uint token = RawTemplateSectionNames.Get(ref _current, hooks);
        return hooks.Buffer!(token);
    }

    /// <summary>최초 0, 성공 1인 호스트 불투명 token만 읽는다. 실제 주소를 DWORD로 자르지 않는다.</summary>
    public uint CurrentToken => _current;
}
