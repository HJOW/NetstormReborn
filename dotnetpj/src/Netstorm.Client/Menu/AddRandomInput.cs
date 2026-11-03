using Netstorm.Client.Input;

namespace Netstorm.Client.Menu;

/// <summary>
/// 대본 AddRandom의 실제 타입 해석과 일반 입력 관문을 제공한다
/// (cpppj <c>client/AddRandomInput.h</c>의 <c>DispatchAddRandomInput</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-10에서 추가했다. 별도 메뉴 수명·소유자 조건을 추가하지 않는다.</remarks>
public static class AddRandomInput
{
    /// <summary>원본 00542954의 명령 번호다.</summary>
    public const uint AddRandomCommand = 0x02000098u;

    /// <summary>명령 코드에서 글자 부분을 가려내는 마스크다.</summary>
    private const uint CommandMask = 0x03ffffffu;

    /// <summary>
    /// 키 뗌과 일반 모드는 소비만 하고, signed y가 255를 넘을 때만 문자열 타입 조회를 한다.
    /// </summary>
    /// <param name="evt">입력 사건이다.</param>
    /// <param name="editor">편집 모드면 참이다.</param>
    /// <param name="mission">미션 중이면 참이다.</param>
    /// <param name="lookup">문자열 타입 조회다. y가 255 이하이면 부르지 않는다.</param>
    /// <param name="add">실제 현재 배치 부모다.</param>
    /// <returns>명령을 소비했으면 참이다.</returns>
    /// <exception cref="InvalidOperationException">AddRandom의 실제 타입 이름 조회·배치 부모가 연결되지 않았다.</exception>
    public static bool DispatchAddRandomInput(InputEvent evt, bool editor, bool mission, Func<uint, int>? lookup, Action<uint, uint>? add)
    {
        if ((evt.Code & CommandMask) != AddRandomCommand)
        {
            return false;
        }
        if ((evt.Code & InputCode.Release) != 0 || (!editor && !mission))
        {
            return true;
        }
        uint type = unchecked((uint)evt.Y);
        if (evt.Y > 255)
        {
            if (lookup is null)
            {
                throw new InvalidOperationException("AddRandom의 실제 타입 이름 조회 미연결");
            }
            type = unchecked((uint)lookup(type));
        }
        if (add is null)
        {
            throw new InvalidOperationException("AddRandom의 실제 현재 배치 부모 미연결");
        }
        // 원문처럼 lookup의 -1·0도 배치 부모로 전달하고 잘못된 타입 진단은 하위에 맡긴다
        add(type, 1);
        return true;
    }
}
