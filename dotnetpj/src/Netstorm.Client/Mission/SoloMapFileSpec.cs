using System.Text;

namespace Netstorm.Client.Mission;

/// <summary>
/// 원본 005327ac의 같은 9바이트를 덮어쓰는 파일 명세 키 저장소다. Client가 하나를 소유하면 모든 목록·이름 경계가 같은 키를 빌린다.
/// 키는 기본 "fortSpec"이고 단독 시나리오 목록일 때 "SOLOSpec"으로 바뀐다. 키를 바꿔도 목록 캐시를 자동으로 무효화하지 않는다(dirty는 별도 원본 호출자가 설정한다).
/// cpppj <c>client/SoloMapFileSpec</c>(SoloMapCatalog.h)을 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 추가했다. cpppj의 <c>Current()</c>는 같은 버퍼를 가리키는 <c>string_view</c>라 키를 바꾸면 이미 받은 뷰의 내용도 바뀌지만,
/// C#의 문자열은 복사본이므로 같은 물리 버퍼는 <see cref="Buffer"/>로 따로 노출한다.
/// </remarks>
public sealed class SoloMapFileSpec
{
    /// <summary>기본 원본 키와 마지막 NUL을 같은 고정 할당에 둔다(9바이트).</summary>
    private readonly byte[] _key = Encoding.ASCII.GetBytes("fortSpec\0");

    /// <summary>현재 8문자 키를 문자열로 읽는다. 읽은 시점의 내용이며 이후 키 변경은 반영되지 않는다.</summary>
    public string Current() => Encoding.ASCII.GetString(_key, 0, _key.Length - 1);

    /// <summary>키가 사는 9바이트 물리 버퍼다. 키를 바꿔도 같은 버퍼이며 마지막 바이트는 항상 NUL이다.</summary>
    public ReadOnlyMemory<byte> Buffer => _key;

    /// <summary>같은 버퍼를 "fortSpec"과 NUL로 복구한다. 목록 dirty는 별도 원본 호출자가 설정한다.</summary>
    public void SelectFort() => Encoding.ASCII.GetBytes("fortSpec\0", _key);

    /// <summary>같은 버퍼를 "SOLOSpec"과 NUL로 바꾼다. 캐시를 자동으로 무효화하지 않는다.</summary>
    public void SelectSolo() => Encoding.ASCII.GetBytes("SOLOSpec\0", _key);
}
